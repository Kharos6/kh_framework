#pragma once

// The feature half (rendering_integration_features.hpp): the GPU material table, material animation, kh_bind_material
// and kh_draw_textured. Never include it on its own.
namespace RenderIntegration {

// C++ twin of KhGpuMat (cb.hlsl): the material lanes, the page layers of the six maps, and the arma model's lanes (p4 /
// p5: filled for the arma model and, KH_USER_SLOTS, for a .hlsl material - whose model lane p5[3] stays 0; zero for pbr
// but the specular route, which is -1). lay1.zw and lay2 are KH_USER_TEX's user0..user5 layers (zero for every material
// without them). an0 - an3 are KH_UV_ANIM's (kh_uv_anim_lanes; zero without an animation, whose flag bits 12 - 14 in
// p0.x are then clear).
struct KhGpuMat {
    float p0[4]; float p1[4]; float p2[4]; float p3[4]; float lay0[4]; float lay1[4]; float p4[4]; float p5[4];
    float lay2[4];
    float an0[4]; float an1[4]; float an2[4]; float an3[4];
};
static_assert(sizeof(KhGpuMat) == 13 * 16, "KhGpuMat is thirteen float4 (cb.hlsl twin, the table's stride)");
static std::vector<KhGpuMat> g_mat_gpu_cpu;   // The table's CPU shadow, one entry per set slot.
static bool g_mat_gpu_dirty = false;
static uint32_t g_mat_gpu_page_gen = 0;   // The g_tex_page_gen the table was built against.

// KH_UV_ANIM - the clock-driven entries, rewritten once a cycle. An entry is listed when kh_mat_gpu_ensure adds it
// (its index and a copy of its parameters) and the list is emptied wherever the table is; kh_mat_gpu_upload - the
// funnel every material bind reaches - rewrites the listed lanes at the first upload of each cycle
// (g_topo_cycles_pub: the render thread's counter, readable by the game thread under the park). One time per cycle
// (g_mat_anim_t: stamped at the main depth clear that opens the cycle - kh_mat_anim_stamp - or, before the first, at
// the first read), so every route and every caster that cycle reads one phase, a frame's period apart. Same
// serialized window as the table (the render thread, or the game thread under the park). Empty list = no work.
struct KhMatAnimEnt { uint32_t ix; KhUvAnim a; };
static std::vector<KhMatAnimEnt> g_mat_anim;
static uint64_t g_mat_anim_done = ~0ull;   // The cycle whose rewrite ran.
static uint64_t g_mat_anim_tcyc = ~0ull;   // The cycle g_mat_anim_t was taken in.
static double   g_mat_anim_t = 0.0;        // That cycle's session time (effect_time_seconds_d), s.
// At the main depth clear, right after the cycle counter moves: the cycle's time, while a clock-driven entry exists.
inline void kh_mat_anim_stamp(uint64_t khas_c) {
    if (g_mat_anim.empty()) return;
    g_mat_anim_tcyc = khas_c;
    g_mat_anim_t = effect_time_seconds_d();
}
// The casters' skip keys (sun union and tiers, DLS, seam) mix this while a caster's set has uv_anim_alpha: a new key
// each cycle - whether or not anything bound a material yet - so an animated cutout's shadow is redrawn at the cycle's
// phase (its draw binds the material, which runs the rewrite). An opaque animated material's keys do not mix it.
inline uint64_t kh_mat_anim_cycle_key() { return g_topo_cycles_pub.load(std::memory_order_relaxed); }
inline double kh_mat_anim_time() {
    const uint64_t khat_c = g_topo_cycles_pub.load(std::memory_order_relaxed);
    if (khat_c != g_mat_anim_tcyc) {
        g_mat_anim_tcyc = khat_c;
        g_mat_anim_t = effect_time_seconds_d();
    }
    return g_mat_anim_t;
}
// One slot's lanes at t (s). Everything that grows with time is reduced here, in double, to the small value the
// shader needs - the translation and the warp's phase to [0, 1), the angle to a turn, the frame to the sheet - so
// the shader needs no clock and loses no precision after hours.
//   an0 = A = S * R(theta), (a00, a01, a10, a11): the lookup uv' = A * uv + c.
//   an1 = (c, ph): c = 0.5 - A * (0.5, 0.5) + offset + scroll * t, wrapped to [0, 1) (the maps repeat); ph the
//         warp's phase per axis, measured against the unwrapped translation, so the wrap moves no ripple.
//   an2 = (amplitude, frequency, 0, 0): the warp adds amplitude * sin(2 pi (frequency * uv'.yx + ph)).
//   an3 = (columns, rows, frame, 0): the flipbook's sheet and this time's frame (0 - frames-1).
inline void kh_uv_anim_lanes(const KhUvAnim& a, double t, KhGpuMat& g) {
    const double khal_pi = 3.14159265358979323846;
    double khal_th = static_cast<double>(a.rot) + static_cast<double>(a.rot_speed) * t;
    khal_th = std::fmod(khal_th, 360.0) * (khal_pi / 180.0);
    const double khal_cs = std::cos(khal_th), khal_sn = std::sin(khal_th);
    const double khal_m[4] = { a.scale[0] * khal_cs, -a.scale[0] * khal_sn,
                               a.scale[1] * khal_sn, a.scale[1] * khal_cs };
    const double khal_c0[2] = { 0.5 - 0.5 * (khal_m[0] + khal_m[1]), 0.5 - 0.5 * (khal_m[2] + khal_m[3]) };
    double khal_c[2], khal_d[2];
    for (int k = 0; k < 2; ++k) {
        double khal_o = static_cast<double>(a.offset[k]) + static_cast<double>(a.scroll[k]) * t;
        khal_o -= std::floor(khal_o);
        const double khal_cf = khal_c0[k] + khal_o;
        khal_c[k] = khal_cf - std::floor(khal_cf);
        khal_d[k] = khal_c[k] - khal_c0[k];   // The translation the ripple is measured against: offset + whole uv.
    }
    double khal_ph[2] = { 0.0, 0.0 };   // Zero, as the warp's lanes, while it is off.
    if (a.warp[0] > 0.0f) {
        double khal_pt = static_cast<double>(a.warp[2]) * t;
        khal_pt -= std::floor(khal_pt);
        const double khal_fq = a.warp[1];
        khal_ph[0] = khal_pt - khal_fq * khal_d[1];   // .x reads uv'.y.
        khal_ph[1] = khal_pt - khal_fq * khal_d[0];
        for (int k = 0; k < 2; ++k) khal_ph[k] -= std::floor(khal_ph[k]);
    }
    double khal_fr = 0.0;
    if (a.flip[2] > 0.0f && a.flip[3] > 1.5f) {
        khal_fr = std::floor(std::fmod(static_cast<double>(a.flip[2]) * t, static_cast<double>(a.flip[3])));
    }
    for (int k = 0; k < 4; ++k) g.an0[k] = static_cast<float>(khal_m[k]);
    g.an1[0] = static_cast<float>(khal_c[0]);
    g.an1[1] = static_cast<float>(khal_c[1]);
    g.an1[2] = static_cast<float>(khal_ph[0]);
    g.an1[3] = static_cast<float>(khal_ph[1]);
    g.an2[0] = a.warp[0] > 0.0f ? a.warp[0] : 0.0f;
    g.an2[1] = a.warp[0] > 0.0f ? a.warp[1] : 0.0f;
    g.an2[2] = 0.0f;
    g.an2[3] = 0.0f;
    g.an3[0] = a.flip[0];
    g.an3[1] = a.flip[1];
    g.an3[2] = static_cast<float>(khal_fr);
    g.an3[3] = 0.0f;
}
// The once-a-cycle rewrite (kh_mat_gpu_upload).
inline void kh_mat_anim_refresh() {
    const uint64_t khar_c = g_topo_cycles_pub.load(std::memory_order_relaxed);
    if (khar_c == g_mat_anim_done) return;
    g_mat_anim_done = khar_c;
    const double khar_t = kh_mat_anim_time();
    for (const KhMatAnimEnt& khar_e : g_mat_anim) {
        if (khar_e.ix < g_mat_gpu_cpu.size()) kh_uv_anim_lanes(khar_e.a, khar_t, g_mat_gpu_cpu[khar_e.ix]);
    }
    g_mat_gpu_dirty = true;
}

// One slot's entry. Flags reflect what actually resolved (a missing texture drops its bit, and a route through it - the
// script's or the slot convention's - degrades to the shader's no-map default, KH_ROUTE_ABSENT's one exception below),
// so the shader never samples a null layer.
inline void kh_mat_gpu_fill(ID3D11DeviceContext* ctx, ID3D11Device* dev, const KhMaterial& m,
                            KhGpuMat& khmf_g, KhMaterialSet::Gpu::PageSet& khmf_pages) {
    int flags = 0;
    float khmf_lay[KH_MAT_MAPS] = {};
    for (int s = 0; s < KH_MAT_MAPS; ++s) {
        khmf_pages.p[s] = nullptr;
        if (!m.used || m.maps[s].path.empty()) continue;
        const KhTexRef khmf_r = kh_tex_resolve(dev, ctx, m.maps[s].path, kh_tex_map_srgb(m, s));
        if (!khmf_r.srv) continue;
        khmf_pages.p[s] = khmf_r.srv;
        khmf_lay[s] = static_cast<float>(khmf_r.layer);
        flags |= 1 << s;
    }
    // KH_ROUTE_ABSENT: a script's route whose map did not resolve (pending, failed) reads the shader's no-map default
    // (cb.hlsl's contract: "with no map behind a route"), not the slot convention's channel of another map; that
    // convention serves an unrouted input, and a routed one whose call sets khre_keep (its -1 reads a map: below).
    auto route_eff = [&](int user, int def_slot, int def_chan, bool khre_keep = false) -> float {
        if (user >= 0 && (flags & (1 << (user >> 2)))) return static_cast<float>(user);
        if (user >= 0 && !khre_keep) return -1.0f;
        if (def_slot >= 0 && (flags & (1 << def_slot))) return static_cast<float>(def_slot * 4 + def_chan);
        return -1.0f;
    };
    // The arma model reads its own conventions from the shared slots: AS.g is the occlusion, SMDI.b the gloss and
    // SMDI.g the specular intensity; it has no roughness or metal and does not take the pbr spec workflow.
    const bool khmf_arma = m.used && m.shader == 2;
    // KH_USER_SLOTS - a .hlsl material's orm / specular slot follows the map the script named: "as" defaults the
    // occlusion to .g and leaves roughness and metal unrouted (AS carries neither; an explicit route still stands);
    // "smdi" defaults the gloss to .b and the specular intensity to .g and leaves the pbr spec workflow off (SMDI's rgb
    // is not an F0). Its arma lanes are filled whichever it named; its model lane stays 0 - the shader's own choice
    // (KhMatSurf::model).
    const bool khmf_user = m.used && m.shader == 1;
    const bool khmf_as = khmf_arma || (khmf_user && m.map_as);
    const bool khmf_smdi = khmf_arma || (khmf_user && m.map_smdi);
    const bool khmf_lanes = khmf_arma || khmf_user;
    // KH_SPEC_AA: bit 15 arms the shader's Toksvig term (KhToksvigA2) for a normal map whose chain is the renderer's
    // own and lossless - the png / tga / bmp route (kh_tex_build_mips): 8-bit texels box-averaged, never
    // renormalised, so a level's shortened normals measure the spread it averaged. A .jpg / .jpeg's compression error
    // shortens the texels too (blotchy roughness along its blocks), and a .dds / .paa keeps its file's levels,
    // renormalised by some tools or block-compressed: there the length is no measure, and the term stays off - the
    // test is the extension alone, so also for the levels KH_DDS_MIP_FILL builds below a 32-bit file's last.
    if ((flags & 2) && (kh_ends_with_ci(m.maps[1].path, ".png") || kh_ends_with_ci(m.maps[1].path, ".tga") ||
                        kh_ends_with_ci(m.maps[1].path, ".bmp"))) flags |= 1 << 15;
    khmf_g.p0[0] = static_cast<float>(flags | kh_uv_anim_bits(m));   // KH_UV_ANIM: bits 12 - 14.
    khmf_g.p0[1] = static_cast<float>(m.alpha_mode);   // 0 opaque, 1 cutout, 2 blend (the draw may
                                                       // override to 3 through matCtl.w).
    khmf_g.p0[2] = m.cutoff;
    khmf_g.p0[3] = m.normal_strength;
    khmf_g.p1[0] = m.base_color[0];
    khmf_g.p1[1] = m.base_color[1];
    khmf_g.p1[2] = m.base_color[2];
    khmf_g.p1[3] = m.roughness;
    khmf_g.p2[0] = khmf_arma ? 0.0f : m.metalness;
    khmf_g.p2[1] = m.emissive_intensity;
    khmf_g.p2[2] = route_eff(m.route_occ,   2, khmf_as ? 1 : 0);
    khmf_g.p2[3] = khmf_arma ? -1.0f : route_eff(m.route_rough, khmf_as ? -1 : 2, 1);
    khmf_g.p3[0] = khmf_arma ? -1.0f : route_eff(m.route_metal, khmf_as ? -1 : 2, 2);
    khmf_g.p3[1] = route_eff(m.route_alpha, 0, 3);
    // KH_ROUTE_ABSENT: the shader's gloss at route -1 is slot 4's alpha (cb.hlsl KhSampleMatG; arma's: the
    // glossiness param) - under a .hlsl material's "smdi" that is no gloss, so its absent route keeps SMDI.b.
    khmf_g.p3[2] = route_eff(m.route_gloss, 4, khmf_smdi ? 2 : 3, khmf_user && m.map_smdi);
    khmf_g.p3[3] = (!khmf_smdi && (flags & 16)) ? 1.0f : 0.0f;
    khmf_g.lay0[0] = khmf_lay[0]; khmf_g.lay0[1] = khmf_lay[1]; khmf_g.lay0[2] = khmf_lay[2]; khmf_g.lay0[3] = khmf_lay[3];
    khmf_g.lay1[0] = khmf_lay[4]; khmf_g.lay1[1] = khmf_lay[5];
    khmf_g.lay1[2] = khmf_lay[6]; khmf_g.lay1[3] = khmf_lay[7];   // KH_USER_TEX: user0, user1.
    khmf_g.lay2[0] = khmf_lay[8]; khmf_g.lay2[1] = khmf_lay[9];   // user2..user5.
    khmf_g.lay2[2] = khmf_lay[10]; khmf_g.lay2[3] = khmf_lay[11];
    for (int k = 0; k < 3; ++k) khmf_g.p4[k] = khmf_lanes ? m.spec_color[k] : 0.0f;
    khmf_g.p4[3] = khmf_lanes ? m.glossiness : 0.0f;
    khmf_g.p5[0] = khmf_lanes ? m.fresnel_n : 0.0f;
    khmf_g.p5[1] = khmf_lanes ? m.fresnel_k : 0.0f;
    khmf_g.p5[2] = khmf_lanes ? route_eff(m.route_spec, khmf_smdi ? 4 : -1, 1) : -1.0f;
    khmf_g.p5[3] = khmf_arma ? 1.0f : 0.0f;
    // KH_UV_ANIM: a clock-driven one's lanes at this cycle's time (kh_mat_anim_refresh rewrites them each cycle).
    if (m.used && m.uv_anim) kh_uv_anim_lanes(m.uv, m.uv_anim_time ? kh_mat_anim_time() : 0.0, khmf_g);
}

// Uploads the set's slots into the table at first draw. Under the park invariant (render thread mid-frame or game
// thread under the lock).
inline void kh_mat_gpu_ensure(ID3D11DeviceContext* ctx, ID3D11Device* dev, const KhMaterialSet& khme_s) {
    static const KhMaterial khme_default;
    for (int khme_round = 0; khme_round < 8; ++khme_round) {   // Bounded: each round is a page grow.
        if (g_mat_gpu_page_gen != g_tex_page_gen) {   // A page moved: every entry's SRV/layer is stale.
            g_mat_gpu_cpu.clear();
            g_mat_anim.clear();   // KH_UV_ANIM: its indices are the table's.
            std::lock_guard<std::mutex> khme_l(g_mat_pool_mu);
            for (auto& khme_kv : g_mat_pool) khme_kv.second.set->gpu.base = -1;
            for (auto& khme_g : g_mat_grave) if (khme_g.set) khme_g.set->gpu.base = -1;
            g_mat_gpu_page_gen = g_tex_page_gen;
        }
        if (khme_s.gpu.base >= 0) return;
        const uint32_t khme_gen0 = g_tex_page_gen;
        const size_t khme_n = khme_s.slots.empty() ? 1 : khme_s.slots.size();
        khme_s.gpu.pages.assign(khme_n, KhMaterialSet::Gpu::PageSet());
        const int khme_base = static_cast<int>(g_mat_gpu_cpu.size());
        uint64_t khme_sig = CryptoGenerator::FNV1A64_OFFSET;
        for (size_t khme_i = 0; khme_i < khme_n; ++khme_i) {
            const KhMaterial& khme_m = khme_i < khme_s.slots.size() ? khme_s.slots[khme_i] : khme_default;
            KhGpuMat khme_g = {};
            kh_mat_gpu_fill(ctx, dev, khme_m, khme_g, khme_s.gpu.pages[khme_i]);
            g_mat_gpu_cpu.push_back(khme_g);
            if (khme_m.used && khme_m.uv_anim_time)   // KH_UV_ANIM: rewritten every cycle (kh_mat_anim_refresh).
                g_mat_anim.push_back(KhMatAnimEnt{ static_cast<uint32_t>(g_mat_gpu_cpu.size() - 1), khme_m.uv });
            // Batch identity: the pages bound per slot, the alpha mode (the representative's material decides the blend
            // split for every member) and the shader kind. Layers, colours, cutoffs and routes ride the table and do
            // not split batches.
            for (int khme_p = 0; khme_p < KH_MAT_MAPS; ++khme_p) {
                const uintptr_t khme_v = reinterpret_cast<uintptr_t>(khme_s.gpu.pages[khme_i].p[khme_p]);
                khme_sig = CryptoGenerator::fnv1a64_update(khme_sig, &khme_v, sizeof(khme_v));
            }
            const int khme_k = khme_m.used ? khme_m.shader : -1;
            const int khme_a = khme_m.used ? khme_m.alpha_mode : 0;
            khme_sig = CryptoGenerator::fnv1a64_update(khme_sig, &khme_k, sizeof(khme_k));
            khme_sig = CryptoGenerator::fnv1a64_update(khme_sig, &khme_a, sizeof(khme_a));
            if (khme_m.used && khme_m.shader == 1) {
                khme_sig = CryptoGenerator::fnv1a64_update(khme_sig, khme_m.user_shader.data(), khme_m.user_shader.size());
            }
        }
        // A load grew a page mid-fill: the entries above name the old SRVs. Round again; the top clears the table.
        if (khme_gen0 != g_tex_page_gen) {
            g_mat_gpu_dirty = true;
            continue;
        }
        khme_s.gpu.base = khme_base;
        khme_s.gpu.page_sig = khme_sig;
        g_mat_gpu_dirty = true;
        return;
    }
}

// The table itself: a dynamic structured buffer re-uploaded whole when dirty. It holds an entry per slot of every
// set ensured since the last g_tex_page_gen change, live or not: a script that sends a changed material each frame
// interns a set a frame, each adding its slots (and a whole upload), until kh_material_gc graves them
// KH_MAT_GC_IDLE_MS after their last reference and rebuilds. Bound at t38 for the pixel stage.
inline bool kh_mat_gpu_upload(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    if (g_mat_gpu_cpu.empty()) return false;
    if (!g_mat_anim.empty()) kh_mat_anim_refresh();   // KH_UV_ANIM: once a cycle; marks the table dirty.
    if (g_mat_gpu_dirty || !g_res.mat_sb) {
        const uint32_t khmu_n = static_cast<uint32_t>(g_mat_gpu_cpu.size());
        if (!g_res.mat_sb || g_res.mat_cap < khmu_n) {
            KH_SAFE_RELEASE(g_res.mat_srv);
            KH_SAFE_RELEASE(g_res.mat_sb);
            uint32_t khmu_cap = g_res.mat_cap ? g_res.mat_cap : 64;
            while (khmu_cap < khmu_n) khmu_cap *= 2;
            D3D11_BUFFER_DESC khmu_bd = {};
            khmu_bd.ByteWidth = khmu_cap * sizeof(KhGpuMat);
            khmu_bd.Usage = D3D11_USAGE_DYNAMIC;
            khmu_bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            khmu_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            khmu_bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            khmu_bd.StructureByteStride = sizeof(KhGpuMat);
            if (FAILED(dev->CreateBuffer(&khmu_bd, nullptr, &g_res.mat_sb))) { g_res.mat_sb = nullptr; return false; }
            D3D11_SHADER_RESOURCE_VIEW_DESC khmu_sd = {};
            khmu_sd.Format = DXGI_FORMAT_UNKNOWN;
            khmu_sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            khmu_sd.Buffer.FirstElement = 0;
            khmu_sd.Buffer.NumElements = khmu_cap;
            if (FAILED(dev->CreateShaderResourceView(g_res.mat_sb, &khmu_sd, &g_res.mat_srv))) {
                KH_SAFE_RELEASE(g_res.mat_sb); g_res.mat_srv = nullptr; return false;
            }
            g_res.mat_cap = khmu_cap;
        }
        D3D11_MAPPED_SUBRESOURCE khmu_map = {};
        if (FAILED(ctx->Map(g_res.mat_sb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khmu_map))) return false;
        memcpy(khmu_map.pData, g_mat_gpu_cpu.data(), khmu_n * sizeof(KhGpuMat));
        ctx->Unmap(g_res.mat_sb, 0);
        g_mat_gpu_dirty = false;
    }
    return g_res.mat_srv != nullptr;
}

// Per-draw material bind: the slot's pages at t14-t18, t42 and t43-t48 (KH_USER_TEX), the table at t38, the matCtl
// lanes. set == nullptr binds the one-slot default set. khbm_mode >= 0 overrides the alpha mode for this draw (the
// blend split's opaque part, 3).
inline void kh_bind_material(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                             ConstantData& cbd, const KhMaterialSet* khbm_set, size_t khbm_slot,
                             float khbm_mode = -1.0f) {
    // A slot the set does not carry draws the default material; that bind is uniform across an instanced batch
    // (matCtl.z).
    bool khbm_uniform = false;
    if (!khbm_set || khbm_slot >= (khbm_set->slots.empty() ? 1 : khbm_set->slots.size())) {
        khbm_set = kh_default_material_set(); khbm_slot = 0; khbm_uniform = true;
    }
    kh_mat_gpu_ensure(ctx, dev, *khbm_set);
    if (khbm_slot >= khbm_set->gpu.pages.size()) khbm_slot = 0;
    const bool khbm_ok = kh_mat_gpu_upload(ctx, dev) && khbm_set->gpu.base >= 0;
    ID3D11ShaderResourceView* khbm_srvs[KH_MAT_MAPS] = {};
    if (khbm_ok) {
        for (int s = 0; s < KH_MAT_MAPS; ++s) khbm_srvs[s] = khbm_set->gpu.pages[khbm_slot].p[s];
    }
    // KH_BIND_FILTER: each range is bound unless the pass's record says it holds these views already.
    const bool khbm_bc = kh_bc_on(ctx);
    if (!khbm_bc || !t_kh_bc.k14 || memcmp(t_kh_bc.v14, khbm_srvs, sizeof(t_kh_bc.v14)) != 0) {
        ctx->PSSetShaderResources(14, 5, khbm_srvs);
        if (khbm_bc) { memcpy(t_kh_bc.v14, khbm_srvs, sizeof(t_kh_bc.v14)); t_kh_bc.k14 = true; }
    }
    if (!khbm_bc || !t_kh_bc.k42 || t_kh_bc.v42 != khbm_srvs[5]) {
        ctx->PSSetShaderResources(42, 1, &khbm_srvs[5]);   // SPECCOLOR (cb.hlsl matSpecColor).
        if (khbm_bc) { t_kh_bc.v42 = khbm_srvs[5]; t_kh_bc.k42 = true; }
    }
    // KH_USER_TEX: user0..user5 (cb.hlsl khUserMap0..5), null for every material without them; inside StateBackup's
    // t0..t69.
    if (!khbm_bc || !t_kh_bc.k43 ||
        memcmp(t_kh_bc.v43, &khbm_srvs[KH_MAT_MAPS_BUILTIN], sizeof(t_kh_bc.v43)) != 0) {
        ctx->PSSetShaderResources(43, KH_MAT_USER_MAPS, &khbm_srvs[KH_MAT_MAPS_BUILTIN]);
        if (khbm_bc) { memcpy(t_kh_bc.v43, &khbm_srvs[KH_MAT_MAPS_BUILTIN], sizeof(t_kh_bc.v43)); t_kh_bc.k43 = true; }
    }
    if (!khbm_bc || !t_kh_bc.k38 || t_kh_bc.v38 != g_res.mat_srv) {
        ctx->PSSetShaderResources(38, 1, &g_res.mat_srv);
        if (khbm_bc) { t_kh_bc.v38 = g_res.mat_srv; t_kh_bc.k38 = true; }
    }
    cbd.mat_ctl[0] = khbm_ok ? static_cast<float>(khbm_set->gpu.base + static_cast<int>(khbm_slot)) : 0.0f;
    cbd.mat_ctl[1] = static_cast<float>(khbm_slot);
    cbd.mat_ctl[2] = khbm_uniform ? 1.0f : 0.0f;
    cbd.mat_ctl[3] = khbm_mode;
}

// The instance stream's material base for an object (0 = the default set).
inline float kh_mat_inst_base(ID3D11DeviceContext* ctx, ID3D11Device* dev, const KhMaterialSet* khib_set) {
    if (!khib_set) khib_set = kh_default_material_set();
    kh_mat_gpu_ensure(ctx, dev, *khib_set);
    return khib_set->gpu.base >= 0 ? static_cast<float>(khib_set->gpu.base) : 0.0f;
}

// Per-submesh textured draws for one object: binds per submesh, uploads the object slice (KH_TX_ONE_UPLOAD: in the
// injection, the flush and the PIP a textured object's only upload), draws the range, two-pass when ts_ordered.
// Preconditions: textured VS/PS/layout bound, VB + blend + dss set, cbd filled.
// khum_part: 1 = the depth-writing draw (a blend material contributes its solid texels only, matParams0.y = 3).
inline uint32_t kh_draw_textured(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                                 ConstantData& cbd, ID3D11Buffer* obj_cb,
                                 const KhMaterialSet& ms, int mid, bool ts_ordered,
                                 ID3D11RasterizerState*& bound_rs,
                                 int khum_ctx, ID3D11PixelShader* khum_builtin_ps,
                                 int khum_lod, int khum_part,
                                 uint32_t khum_inst_n = 0, uint32_t khum_inst_first = 0) {
    static const KhMaterial khtx_default;   // Unset slots draw with defaults.
    const MeshDef& md = mesh_def(mid);
    uint32_t draws = 0;
    auto khum_draw = [&](const MeshSubmesh& khum_sm) {
        if (khum_inst_n) {
            ctx->DrawIndexedInstanced(khum_sm.index_count, khum_inst_n, khum_sm.index_start, 0, khum_inst_first);
        } else {
            ctx->DrawIndexed(khum_sm.index_count, khum_sm.index_start, 0);
        }
        ++draws;
    };
    ID3D11PixelShader* khum_bound = khum_builtin_ps;
    // The level's submesh table: cardinality and order match level 0's (validated at cache load), so si addresses
    // the same material slot at every level.
    const std::vector<MeshSubmesh>& khum_tab = mesh_lod_submeshes(md, khum_lod);

    for (size_t si = 0; si < khum_tab.size(); ++si) {
        const MeshSubmesh& sm = khum_tab[si];
        if (sm.index_count == 0) continue;
        const KhMaterial& mat = (si < ms.slots.size() && ms.slots[si].used)
                              ? ms.slots[si] : khtx_default;
        // Part 2 = the translucent part. User material shader: per-submesh PS selection; khum_ctx names the caller's
        // pipeline variant so the user twin matches its builtin.
        if (khum_part == 2 && mat.alpha_mode != 2) continue;
        ID3D11PixelShader* khum_want = khum_builtin_ps;
        // A submesh that asked for a user shader and has not got one (pending or failed) draws PSWhite (flat unlit
        // white), not builtin PBR (builtin PBR only while the KH_WHITE_PREVIEW pixel shader is missing). PSWhite is the
        // colour alone - none of the replaced PS's per-pixel cuts (the LOD dither, our own near plane, the
        // view-distance cut, the terrain lane, the guard) - so it can show past the object's view distance or through
        // a ridge its siblings respect, and draws both levels whole in an LOD fade: briefly while the compile is
        // pending or queued, for good when it failed (reported once).
        bool khum_white = false;

        if (mat.shader == 1 && !mat.user_shader.empty()) {
            ID3D11PixelShader* khum_ps = kh_user_mat_ps(dev, mat.user_shader, khum_ctx);
            if (khum_ps) {
                khum_want = khum_ps;
            } else if (g_res.ps_white) {
                khum_want = g_res.ps_white;
                khum_white = true;
            }
        }

        if (khum_want != khum_bound) {
            ctx->PSSetShader(khum_want, nullptr, 0);
            khum_bound = khum_want;
        }

        // The substitute takes the default set rather than the real one.
        kh_bind_material(ctx, dev, cbd, khum_white ? nullptr : &ms, si,
                         (khum_part == 1 && !khum_white && mat.alpha_mode == 2) ? 3.0f : -1.0f);
        if (!kh_upload_obj_cb(ctx, obj_cb, cbd)) break;

        if (ts_ordered) {   // Interiors first, exteriors over them (the ordered contract).
            if (bound_rs != g_res.rasterizer_front) {
                ctx->RSSetState(g_res.rasterizer_front);
                bound_rs = g_res.rasterizer_front;
            }
            khum_draw(sm);
            ctx->RSSetState(g_res.rasterizer_cull);
            bound_rs = g_res.rasterizer_cull;
            khum_draw(sm);
        } else {
            khum_draw(sm);
        }
    }

    // Leave the caller's builtin bound: its PS trackers assume it.
    if (khum_bound != khum_builtin_ps) ctx->PSSetShader(khum_builtin_ps, nullptr, 0);
    return draws;
}

// Every material set a pass will draw is uploaded before the pass plans: a first-sight load grows a page and
// rebuilds the table, which would leave a base already written into a lane stale. khme_vis is the pass's own
// visibility predicate, so a culled object's first-sight resolve costs the pass nothing.
template <class KhMatVis>
inline void kh_mat_ensure_all(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                              const std::vector<RenderObject>& meshes, KhMatVis khme_vis) {
    for (int khme_round = 0; khme_round < 8; ++khme_round) {
        const uint32_t khme_gen = g_tex_page_gen;
        kh_mat_gpu_ensure(ctx, dev, *kh_default_material_set());
        for (const RenderObject& o : meshes) {
            const KhMaterialSet* khme_s = kh_obj_textured(o);
            if (!khme_s) continue;
            if (!khme_vis(o)) continue;
            kh_mat_gpu_ensure(ctx, dev, *khme_s);
        }
        if (khme_gen == g_tex_page_gen) return;
    }
}
}