#pragma once

// The feature half (rendering_integration_features.hpp): the material sets: hashing, the uv animation, the pool and
// interning. Never include it on its own.
namespace RenderIntegration {
static_assert(sizeof(UvAnim) == 15 * sizeof(float), "UvAnim is hashed as its bytes: floats only, no padding");

inline uint64_t material_set_hash(const MaterialSet& mh_s) {
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    auto mix = [&h](const void* p, size_t n) {
        h = CryptoGenerator::fnv1a64_update(h, static_cast<const char*>(p), n);
        h = CryptoGenerator::fnv1a64_update(h, "\xFF", 1);
    };
    auto mixs = [&mix](const std::string& s) { mix(s.data(), s.size()); };
    const uint32_t mh_n = static_cast<uint32_t>(mh_s.slots.size());
    mix(&mh_n, sizeof(mh_n));

    for (const Material& m : mh_s.slots) {
        const uint8_t u = m.used ? 1 : 0;
        mix(&u, 1);
        if (!m.used) continue;
        mix(&m.shader, sizeof(m.shader));
        mixs(m.user_shader);
        for (int k = 0; k < MAT_MAPS; ++k) {
            mixs(m.maps[k].path);
            // USER_TEX: a user slot's colour space is part of its identity.
            if (k >= MAT_MAPS_BUILTIN) {
                const uint8_t sg = m.maps[k].srgb ? 1 : 0;
                mix(&sg, 1);
            }
        }
        mix(m.base_color, sizeof(m.base_color));
        mix(&m.roughness, sizeof(m.roughness));
        mix(&m.metalness, sizeof(m.metalness));
        mix(&m.emissive_intensity, sizeof(m.emissive_intensity));
        mix(&m.normal_strength, sizeof(m.normal_strength));
        mix(&m.cutoff, sizeof(m.cutoff));
        mix(&m.alpha_mode, sizeof(m.alpha_mode));
        mix(&m.route_occ, sizeof(m.route_occ));
        mix(&m.route_rough, sizeof(m.route_rough));
        mix(&m.route_metal, sizeof(m.route_metal));
        mix(&m.route_alpha, sizeof(m.route_alpha));
        mix(&m.route_gloss, sizeof(m.route_gloss));
        mix(&m.route_spec, sizeof(m.route_spec));
        mix(m.spec_color, sizeof(m.spec_color));
        mix(&m.glossiness, sizeof(m.glossiness));
        mix(&m.fresnel_n, sizeof(m.fresnel_n));
        mix(&m.fresnel_k, sizeof(m.fresnel_k));
        if (m.shader == 1) {   // USER_SLOTS: what a .hlsl material's orm / specular slots hold. The other kinds mix nothing more.
            const uint8_t nm = static_cast<uint8_t>((m.map_as ? 1 : 0) | (m.map_smdi ? 2 : 0) |
                                                    (m.user_vertex ? 4 : 0) | (m.vertex_static ? 8 : 0));   // KH_USER_VS.
            mix(&nm, 1);
            mix(&m.vertex_bound, sizeof(m.vertex_bound));
        }
        // UV_ANIM: mixed only when set (uv_anim follows from uv alone).
        if (m.uv_anim) {
            const uint8_t ua = 0xA7;
            mix(&ua, 1);
            mix(&m.uv, sizeof(m.uv));
        }
    }

    return h;
}

// UV_ANIM: uv_anim / uv_anim_time from the parameters (the parse, once the slot's params are read).
inline void uv_anim_derive(Material& m) {
    const UvAnim& a = m.uv;
    const bool xf = a.scale[0] != 1.0f || a.scale[1] != 1.0f || a.offset[0] != 0.0f || a.offset[1] != 0.0f ||
                    a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot != 0.0f || a.rot_speed != 0.0f;
    const bool wp = a.warp[0] > 0.0f;
    const bool fb = a.flip[0] * a.flip[1] > 1.5f;
    m.uv_anim = xf || wp || fb;
    m.uv_anim_time = a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot_speed != 0.0f ||
                     (wp && a.warp[2] != 0.0f) || (fb && a.flip[2] > 0.0f && a.flip[3] > 1.5f);
}
// The table's flag bits (p0.x, above the map bits 0 - 11): 12 the transform (or a warp, which works on its output),
// 13 the warp, 14 the flipbook (15: SPEC_AA's Toksvig arm, mat_gpu_fill).
inline int uv_anim_bits(const Material& m) {
    if (!m.used || !m.uv_anim) return 0;
    const UvAnim& a = m.uv;
    const bool wp = a.warp[0] > 0.0f;
    const bool fb = a.flip[0] * a.flip[1] > 1.5f;
    const bool xf = a.scale[0] != 1.0f || a.scale[1] != 1.0f || a.offset[0] != 0.0f || a.offset[1] != 0.0f ||
                    a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot != 0.0f || a.rot_speed != 0.0f;
    return ((xf || wp) ? 4096 : 0) | (wp ? 8192 : 0) | (fb ? 16384 : 0);
}

// Material sets are interned by content hash and pool-owned, so a RenderObject carries a raw pointer and stays
// trivially copyable. A set lives while any object references it (noted each flush) plus MAT_GC_IDLE_MS, then goes
// through a two-flush grave like the meshes. The pool and the grave are under g_mat_pool_mu. Sets are interned by
// apply_material_update (game thread; the pinned default by its first user, either thread), noted by flush_frame's
// census and by material_gc, collected by material_gc in flush_locked's housekeeping (the render thread's scene
// flush, or the game thread under a park), and emptied, all but the pinned default, by material_pool_reset at the
// session teardown.
static constexpr uint64_t MAT_GC_IDLE_MS = 120000;
struct MatPoolEntry { std::unique_ptr<MaterialSet> set; uint64_t last_ref_ms; };
struct MatGrave { std::unique_ptr<MaterialSet> set; uint64_t flush_at; };
static std::unordered_map<uint64_t, MatPoolEntry> g_mat_pool;
static std::vector<MatGrave> g_mat_grave;
static std::mutex g_mat_pool_mu;

inline const MaterialSet* material_intern(MaterialSet&& s) {
    if (s.hash == 0) s.hash = material_set_hash(s);
    std::lock_guard<std::mutex> l(g_mat_pool_mu);
    auto it = g_mat_pool.find(s.hash);
    if (it == g_mat_pool.end()) {
        MatPoolEntry e;
        e.set.reset(new MaterialSet(std::move(s)));
        e.set->gpu = MaterialSet::Gpu();   // A copied-from set's GPU state is not this set's.
        e.last_ref_ms = GetTickCount64();
        it = g_mat_pool.emplace(e.set->hash, std::move(e)).first;
    } else {
        it->second.last_ref_ms = GetTickCount64();
    }
    return it->second.set.get();
}

// MAT_TABLE: the one-slot default set every untextured / colour-only draw binds (flags 0, defaults). Pinned:
// material_gc never collects it.
inline const MaterialSet* default_material_set() {
    static const MaterialSet* p = []() {
        MaterialSet s;
        s.slots.resize(1);
        s.any = false;
        s.hash = material_set_hash(s);
        return material_intern(std::move(s));
    }();
    return p;
}

// MAT_NOTE_ONCE: the census's notes, batched as its mesh notes are (REF_NOTE_ONCE) - every set the walk saw,
// then the distinct ones stamped once under one lock at its end (still under g_draw_list_mutex: the census's lock
// order, draw list then pool). Game thread scratch, emptied at the walk's start (an exception may have left the last
// one's) as at its end.
static std::vector<const MaterialSet*> g_mat_note_cen;
inline void material_note_end(std::vector<const MaterialSet*>& v) {
    if (v.empty()) return;
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    std::lock_guard<std::mutex> l(g_mat_pool_mu);
    const uint64_t now = GetTickCount64();
    for (const MaterialSet* s : v) {
        auto it = g_mat_pool.find(s->hash);
        if (it != g_mat_pool.end()) it->second.last_ref_ms = now;
    }
    v.clear();
}

inline bool mat_set_has_alpha(const MaterialSet* s) {
    if (!s || !s->any) return false;
    for (const Material& m : s->slots) if (m.used && m.alpha_mode != 0) return true;
    return false;
}
}