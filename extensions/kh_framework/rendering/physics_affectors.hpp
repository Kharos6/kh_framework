#pragma once

// The feature half (rendering_integration_features.hpp): AFFECTOR, the script-side physics affectors. Never include
// it on its own.
namespace RenderIntegration {

// Defined with the effect clock (rendering/effect_clock.hpp). The affectors' lifetimes and turbulence run on it, and
// so does the frame delta the simulation steps on (cloth_frame_dt, rendering/cloth_sync.hpp): steady wall time,
// so the cloth keeps moving in the pause menu and does not follow setAccTime. The first frame of a session reports
// the clock's age at that call (the clock starts at its first read by anything, so 0 only when this is it), a gap
// over 10 s reports 0; the core advances nothing for a zero dt (an unprimed instance places its rest pose on its
// first step whatever dt says) and clamps any other long frame to CLOTH_DT_MAX.
inline double effect_time_seconds_d();

// AFFECTOR - the script-side record (addPhysicsAffector, updatePhysicsAffector, removeRenderHandler). GAME
// THREAD ONLY, unlocked: the SQF commands, cloth_sync and the mission reset all run on it, and neither the render
// thread nor a worker reads this table - a worker sees only the views cloth_sync copies into its instance.
struct Affector {
    uint8_t  type = AFF_NONE;
    float    pos[3] = {};                      // SQF [x, y, zASL].
    float    dir[3] = { 0.0f, 1.0f, 0.0f };    // SQF vectorDir, unit; the unrotated types ignore it.
    // Half-extents (m) along the affector's right, forward and up - world east, north and up for a type without a
    // direction. All zero = unbounded.
    float    scale[3] = {};
    float    p[4] = {};                        // The type's parameters (ClothAffView::p).
    float    falloff = 0.25f;                  // Outer fraction of the region faded to zero (0..1).
    // The lifetime, [fadeIn, hold, fadeOut] on the process clock; timed false = permanent. birth re-arms when the
    // script sets a new lifetime; t0 never does, so turbulence keeps flowing.
    bool     timed = false;
    float    fade_in = 0.0f, hold = 0.0f, fade_out = 0.0f;
    double   birth = 0.0;
    double   t0 = 0.0;
    uint64_t seq = 0;                          // Creation stamp; turbulence's seed.
};
static std::unordered_map<std::string, Affector> g_affectors;
// Its size, stored by the game thread wherever the table changes: renderStats' physicsAffectors gauge (KH_AFF_GAUGE,
// read live by the command, not through the flush's stats copy).
static std::atomic<uint64_t> g_affectors_n{ 0 };
inline void aff_count_note() { g_affectors_n.store(g_affectors.size(), std::memory_order_relaxed); }
static uint64_t g_aff_next_seq = 0;
// This frame's affectors with a live envelope, world centre in double, built once per frame and copied into each
// instance by cloth_sync. reach is the region's bounding radius, negative for an unbounded one.
struct AffFrame { double c[3]; float reach; ClothAffView v; };
static std::vector<AffFrame> g_aff_frame;

inline std::string make_affector_uid() {
    return std::string("khpa_") + UIDGenerator::generate();
}

inline bool aff_rotated(uint8_t t) {
    return t == AFF_WIND || t == AFF_VORTEX || t == AFF_FORCE;
}

// The renderer's lifetime_envelope, over the same [fadeIn, hold, fadeOut].
inline float aff_envelope(const Affector& a, double now, bool& expired) {
    expired = false;
    if (!a.timed) return 1.0f;
    double age = now - a.birth;
    if (age < 0.0) age = 0.0;
    if (age < a.fade_in) return static_cast<float>(age / (a.fade_in > 0.001f ? a.fade_in : 0.001f));
    age -= a.fade_in;
    if (age < a.hold) return 1.0f;
    age -= a.hold;
    if (age < a.fade_out) return 1.0f - static_cast<float>(age / (a.fade_out > 0.001f ? a.fade_out : 0.001f));
    expired = true;
    return 0.0f;
}

// Every entry point that looks an affector up reaps first, so an expired handle reads as removed everywhere at once.
inline void aff_reap(double now) {
    for (auto it = g_affectors.begin(); it != g_affectors.end(); ) {
        bool x = false;
        aff_envelope(it->second, now, x);
        if (x) it = g_affectors.erase(it);
        else ++it;
    }
    aff_count_note();
}

inline std::string aff_add(Affector a) {
    if (!render_on()) return std::string();   // PLAYER_ONLY.
    a.seq = ++g_aff_next_seq;
    a.birth = a.t0 = effect_time_seconds_d();
    const std::string h = make_affector_uid();
    g_affectors[h] = a;
    aff_count_note();
    return h;
}

inline bool aff_remove(const std::string& h) {
    const bool r = g_affectors.erase(h) != 0;
    aff_count_note();
    return r;
}
inline void aff_clear() { g_affectors.clear(); aff_count_note(); }
// ALL_HANDLES: every affector handle, appended in creation order (seq). Game thread only; the caller reaps first
// if it wants only live ones.
inline void aff_handles(std::vector<std::string>& out) {
    std::vector<std::pair<uint64_t, std::string>> v;
    v.reserve(g_affectors.size());
    for (const auto& kv : g_affectors) v.emplace_back(kv.second.seq, kv.first);
    std::sort(v.begin(), v.end());
    for (auto& p : v) out.push_back(std::move(p.second));
}

// The affector's axes as engine-axes rows: right, forward, up - Arma's model frame (x right, y forward, z up) with
// vectorDir as forward and the world's up as the reference, built in SQF axes then swapped (east, up, north) like a
// position. A vertical vectorDir takes north as its up. A type without a direction keeps east, north, up.
inline void aff_basis(const Affector& a, float* ax) {
    float r[3] = { 1.0f, 0.0f, 0.0f }, f[3] = { 0.0f, 1.0f, 0.0f }, u[3] = { 0.0f, 0.0f, 1.0f };
    if (aff_rotated(a.type)) {
        for (int k = 0; k < 3; ++k) f[k] = a.dir[k];
        const float ref[3] = { 0.0f, fabsf(f[2]) > 0.999f ? 1.0f : 0.0f, fabsf(f[2]) > 0.999f ? 0.0f : 1.0f };
        r[0] = f[1] * ref[2] - f[2] * ref[1];
        r[1] = f[2] * ref[0] - f[0] * ref[2];
        r[2] = f[0] * ref[1] - f[1] * ref[0];
        const float rl = cloth_v3_len(r);
        for (int k = 0; k < 3; ++k) r[k] /= rl;
        u[0] = r[1] * f[2] - r[2] * f[1];
        u[1] = r[2] * f[0] - r[0] * f[2];
        u[2] = r[0] * f[1] - r[1] * f[0];
    }
    const float* rows[3] = { r, f, u };
    for (int w = 0; w < 3; ++w) {
        ax[w * 3 + 0] = rows[w][0];
        ax[w * 3 + 1] = rows[w][2];
        ax[w * 3 + 2] = rows[w][1];
    }
}

// splitmix64: turbulence's modes are a pure function of the affector's seed.
inline uint64_t aff_rng(uint64_t& s) {
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
inline float aff_u01(uint64_t& s) {
    return static_cast<float>(static_cast<double>(aff_rng(s) >> 40) * (1.0 / 16777216.0));
}

// Reaps, then builds this frame's view of every affector with a live envelope. Game thread, once per frame, from
// cloth_sync.
inline void aff_frame_build(double now) {
    g_aff_frame.clear();
    aff_reap(now);
    const double tau = 6.283185307179586;
    for (const auto& kv : g_affectors) {
        const Affector& a = kv.second;
        bool x = false;
        const float gain = aff_envelope(a, now, x);
        if (!(gain > 0.0f)) continue;
        AffFrame fr{};
        fr.c[0] = static_cast<double>(a.pos[0]);   // SQF [x, y, zASL] -> engine [x, zASL, y].
        fr.c[1] = static_cast<double>(a.pos[2]);
        fr.c[2] = static_cast<double>(a.pos[1]);
        ClothAffView& v = fr.v;
        v.type = a.type;
        aff_basis(a, v.ax);
        v.bounded = a.scale[0] > 0.0f && a.scale[1] > 0.0f && a.scale[2] > 0.0f;
        fr.reach = -1.0f;
        if (v.bounded) {
            for (int k = 0; k < 3; ++k) v.inv_r[k] = 1.0f / a.scale[k];
            fr.reach = fmaxf(a.scale[0], fmaxf(a.scale[1], a.scale[2]));
        }
        v.edge = a.falloff < 0.0f ? 0.0f : (a.falloff > 1.0f ? 1.0f : a.falloff);
        v.gain = gain;
        for (int k = 0; k < 4; ++k) v.p[k] = a.p[k];
        if (a.type == AFF_VORTEX) {
            // The core: as set, else a quarter of the region's narrower cross-section, else 1 m when unbounded.
            float core = a.p[3];
            if (!(core > 0.0f)) core = v.bounded ? 0.25f * fminf(a.scale[0], a.scale[2]) : 1.0f;
            v.p[3] = core > 0.01f ? core : 0.01f;
        }
        if (a.type == AFF_TURBULENCE) {
            // Eight modes: four at the eddy size and four an octave finer at half the amplitude, directions and
            // phases from the seed, scaled so the field's RMS speed is the script's strength (each unit mode is RMS
            // 1/sqrt(2); the weights' squares sum to 5).
            const float size = a.p[1] > 0.01f ? a.p[1] : 0.01f;
            const float rate = a.p[2] > 0.0f ? a.p[2] : 0.0f;
            const float amp = a.p[0] / sqrtf(2.5f);
            const double age = now - a.t0;
            uint64_t s = a.seq * 0xD1B54A32D192ED03ull + 0x5851F42D4C957F2Dull;
            for (uint32_t m = 0; m < AFF_MODES; ++m) {
                const float w = m < AFF_MODES / 2 ? 1.0f : 0.5f;
                const float z = 2.0f * aff_u01(s) - 1.0f;
                const float az = static_cast<float>(tau) * aff_u01(s);
                const float rz = sqrtf(fmaxf(0.0f, 1.0f - z * z));
                const float khat[3] = { rz * cosf(az), z, rz * sinf(az) };
                const float km = static_cast<float>(tau) / size *
                                 (m < AFF_MODES / 2 ? 1.0f : 2.0f) * (0.75f + 0.5f * aff_u01(s));
                // Any direction perpendicular to the wave: the cross with a random vector, retried when degenerate.
                float af_a[3] = { 0.0f, 0.0f, 0.0f };
                for (int t = 0; t < 4; ++t) {
                    const float q[3] = { 2.0f * aff_u01(s) - 1.0f, 2.0f * aff_u01(s) - 1.0f,
                                         2.0f * aff_u01(s) - 1.0f };
                    af_a[0] = khat[1] * q[2] - khat[2] * q[1];
                    af_a[1] = khat[2] * q[0] - khat[0] * q[2];
                    af_a[2] = khat[0] * q[1] - khat[1] * q[0];
                    if (cloth_v3_len(af_a) > 0.1f) break;
                }
                const float al = cloth_v3_len(af_a);
                for (int k = 0; k < 3; ++k) {
                    v.mk[m][k] = khat[k] * km;
                    v.ma[m][k] = al > 1.0e-6f ? af_a[k] / al * amp * w : 0.0f;
                }
                v.mw[m] = static_cast<float>(tau) * rate * (0.5f + aff_u01(s));
                const double ph0 = tau * static_cast<double>(aff_u01(s));
                v.mph[m] = static_cast<float>(fmod(static_cast<double>(v.mw[m]) * age + ph0, tau));
            }
        }
        g_aff_frame.push_back(fr);
    }
}
}