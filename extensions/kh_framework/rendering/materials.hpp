#pragma once

// The feature half (rendering_integration_features.hpp): the material sets: hashing, the uv animation, the pool and
// interning. Never include it on its own.
namespace RenderIntegration {
static_assert(sizeof(KhUvAnim) == 15 * sizeof(float), "KhUvAnim is hashed as its bytes: floats only, no padding");

inline uint64_t kh_material_set_hash(const KhMaterialSet& khmh_s) {
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    auto mix = [&h](const void* p, size_t n) {
        h = CryptoGenerator::fnv1a64_update(h, static_cast<const char*>(p), n);
        h = CryptoGenerator::fnv1a64_update(h, "\xFF", 1);
    };
    auto mixs = [&mix](const std::string& s) { mix(s.data(), s.size()); };
    const uint32_t khmh_n = static_cast<uint32_t>(khmh_s.slots.size());
    mix(&khmh_n, sizeof(khmh_n));

    for (const KhMaterial& m : khmh_s.slots) {
        const uint8_t khmh_u = m.used ? 1 : 0;
        mix(&khmh_u, 1);
        if (!m.used) continue;
        mix(&m.shader, sizeof(m.shader));
        mixs(m.user_shader);
        for (int k = 0; k < KH_MAT_MAPS; ++k) {
            mixs(m.maps[k].path);
            // KH_USER_TEX: a user slot's colour space is part of its identity.
            if (k >= KH_MAT_MAPS_BUILTIN) {
                const uint8_t khmh_sg = m.maps[k].srgb ? 1 : 0;
                mix(&khmh_sg, 1);
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
        if (m.shader == 1) {   // KH_USER_SLOTS: what a .hlsl material's orm / specular slots hold. The other kinds mix nothing more.
            const uint8_t khmh_nm = static_cast<uint8_t>((m.map_as ? 1 : 0) | (m.map_smdi ? 2 : 0) |
                                                         (m.user_vertex ? 4 : 0) | (m.vertex_static ? 8 : 0));   // KH_USER_VS.
            mix(&khmh_nm, 1);
            mix(&m.vertex_bound, sizeof(m.vertex_bound));
        }
        // KH_UV_ANIM: mixed only when set (uv_anim follows from uv alone).
        if (m.uv_anim) {
            const uint8_t khmh_ua = 0xA7;
            mix(&khmh_ua, 1);
            mix(&m.uv, sizeof(m.uv));
        }
    }

    return h;
}

// KH_UV_ANIM: uv_anim / uv_anim_time from the parameters (the parse, once the slot's params are read).
inline void kh_uv_anim_derive(KhMaterial& m) {
    const KhUvAnim& a = m.uv;
    const bool khud_xf = a.scale[0] != 1.0f || a.scale[1] != 1.0f || a.offset[0] != 0.0f || a.offset[1] != 0.0f ||
                         a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot != 0.0f || a.rot_speed != 0.0f;
    const bool khud_wp = a.warp[0] > 0.0f;
    const bool khud_fb = a.flip[0] * a.flip[1] > 1.5f;
    m.uv_anim = khud_xf || khud_wp || khud_fb;
    m.uv_anim_time = a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot_speed != 0.0f ||
                     (khud_wp && a.warp[2] != 0.0f) || (khud_fb && a.flip[2] > 0.0f && a.flip[3] > 1.5f);
}
// The table's flag bits (p0.x, above the map bits 0 - 11): 12 the transform (or a warp, which works on its output),
// 13 the warp, 14 the flipbook (15: KH_SPEC_AA's Toksvig arm, kh_mat_gpu_fill).
inline int kh_uv_anim_bits(const KhMaterial& m) {
    if (!m.used || !m.uv_anim) return 0;
    const KhUvAnim& a = m.uv;
    const bool khub_wp = a.warp[0] > 0.0f;
    const bool khub_fb = a.flip[0] * a.flip[1] > 1.5f;
    const bool khub_xf = a.scale[0] != 1.0f || a.scale[1] != 1.0f || a.offset[0] != 0.0f || a.offset[1] != 0.0f ||
                         a.scroll[0] != 0.0f || a.scroll[1] != 0.0f || a.rot != 0.0f || a.rot_speed != 0.0f;
    return ((khub_xf || khub_wp) ? 4096 : 0) | (khub_wp ? 8192 : 0) | (khub_fb ? 16384 : 0);
}

// Material sets are interned by content hash and pool-owned, so a RenderObject carries a raw pointer and stays
// trivially copyable. A set lives while any object references it (noted each flush) plus KH_MAT_GC_IDLE_MS, then goes
// through a two-flush grave like the meshes. The pool and the grave are under g_mat_pool_mu. Sets are interned by
// kh_apply_material_update (game thread; the pinned default by its first user, either thread), noted by flush_frame's
// census and by kh_material_gc, collected by kh_material_gc in flush_locked's housekeeping (the render thread's scene
// flush, or the game thread under a park), and emptied, all but the pinned default, by kh_material_pool_reset at the
// session teardown.
static constexpr uint64_t KH_MAT_GC_IDLE_MS = 120000;
struct KhMatPoolEntry { std::unique_ptr<KhMaterialSet> set; uint64_t last_ref_ms; };
struct KhMatGrave { std::unique_ptr<KhMaterialSet> set; uint64_t flush_at; };
static std::unordered_map<uint64_t, KhMatPoolEntry> g_mat_pool;
static std::vector<KhMatGrave> g_mat_grave;
static std::mutex g_mat_pool_mu;

inline const KhMaterialSet* kh_material_intern(KhMaterialSet&& khmi_s) {
    if (khmi_s.hash == 0) khmi_s.hash = kh_material_set_hash(khmi_s);
    std::lock_guard<std::mutex> khmi_l(g_mat_pool_mu);
    auto khmi_it = g_mat_pool.find(khmi_s.hash);
    if (khmi_it == g_mat_pool.end()) {
        KhMatPoolEntry khmi_e;
        khmi_e.set.reset(new KhMaterialSet(std::move(khmi_s)));
        khmi_e.set->gpu = KhMaterialSet::Gpu();   // A copied-from set's GPU state is not this set's.
        khmi_e.last_ref_ms = GetTickCount64();
        khmi_it = g_mat_pool.emplace(khmi_e.set->hash, std::move(khmi_e)).first;
    } else {
        khmi_it->second.last_ref_ms = GetTickCount64();
    }
    return khmi_it->second.set.get();
}

// KH_MAT_TABLE: the one-slot default set every untextured / colour-only draw binds (flags 0, defaults). Pinned:
// kh_material_gc never collects it.
inline const KhMaterialSet* kh_default_material_set() {
    static const KhMaterialSet* khdm_p = []() {
        KhMaterialSet khdm_s;
        khdm_s.slots.resize(1);
        khdm_s.any = false;
        khdm_s.hash = kh_material_set_hash(khdm_s);
        return kh_material_intern(std::move(khdm_s));
    }();
    return khdm_p;
}

// KH_MAT_NOTE_ONCE: the census's notes, batched as its mesh notes are (KH_REF_NOTE_ONCE) - every set the walk saw,
// then the distinct ones stamped once under one lock at its end (still under g_draw_list_mutex: the census's lock
// order, draw list then pool). Game thread scratch, emptied at the walk's start (an exception may have left the last
// one's) as at its end.
static std::vector<const KhMaterialSet*> g_mat_note_cen;
inline void kh_material_note_end(std::vector<const KhMaterialSet*>& khme_v) {
    if (khme_v.empty()) return;
    std::sort(khme_v.begin(), khme_v.end());
    khme_v.erase(std::unique(khme_v.begin(), khme_v.end()), khme_v.end());
    std::lock_guard<std::mutex> khme_l(g_mat_pool_mu);
    const uint64_t khme_now = GetTickCount64();
    for (const KhMaterialSet* khme_s : khme_v) {
        auto khme_it = g_mat_pool.find(khme_s->hash);
        if (khme_it != g_mat_pool.end()) khme_it->second.last_ref_ms = khme_now;
    }
    khme_v.clear();
}

inline bool kh_mat_set_has_alpha(const KhMaterialSet* khma_s) {
    if (!khma_s || !khma_s->any) return false;
    for (const KhMaterial& m : khma_s->slots) if (m.used && m.alpha_mode != 0) return true;
    return false;
}
}