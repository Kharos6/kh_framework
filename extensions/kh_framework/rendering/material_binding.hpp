#pragma once

// The feature half (rendering_integration_features.hpp): the GPU material table, material animation, bind_material
// and draw_textured. Never include it on its own.
namespace RenderIntegration {

// C++ twin of KhGpuMat (cb.hlsl): the material lanes, the page layers of the six maps, and the arma model's lanes (p4 /
// p5: filled for the arma model and, USER_SLOTS, for a .hlsl material - whose model lane p5[3] stays 0; zero for pbr
// but the specular route, which is -1). lay1.zw and lay2 are USER_TEX's user0..user5 layers (zero for every material
// without them). an0 - an3 are UV_ANIM's (uv_anim_lanes; zero without an animation, whose flag bits 12 - 14 in
// p0.x are then clear).
struct KhGpuMat {
    float p0[4]; float p1[4]; float p2[4]; float p3[4]; float lay0[4]; float lay1[4]; float p4[4]; float p5[4];
    float lay2[4];
    float an0[4]; float an1[4]; float an2[4]; float an3[4];
};
static_assert(sizeof(KhGpuMat) == 13 * 16, "KhGpuMat is thirteen float4 (cb.hlsl twin, the table's stride)");
// MAT_TABLE: the material table's CPU copy, its dirty flag and the page generation it was built at.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_objects_reset resets every member.
struct MatGpuState {
    std::vector<KhGpuMat> cpu;   // The table's CPU shadow, one entry per set slot.
    bool dirty = false;
    uint32_t page_gen = 0;   // The g_tex_page_gen the table was built against.
};
static MatGpuState g_mat_gpu;

// UV_ANIM - the clock-driven entries, rewritten once a cycle. An entry is listed when mat_gpu_ensure adds it
// (its index and a copy of its parameters) and the list is emptied wherever the table is; mat_gpu_upload - the
// funnel every material bind reaches - rewrites the listed lanes at the first upload of each cycle
// (g_topo_cycles_pub: the render thread's counter, readable by the game thread under the park). One time per cycle
// (g_mat_anim_t: stamped at the main depth clear that opens the cycle - mat_anim_stamp - or, before the first, at
// the first read), so every route and every caster that cycle reads one phase, a frame's period apart. Same
// serialized window as the table (the render thread, or the game thread under the park). Empty list = no work.
struct MatAnimEnt { uint32_t ix; UvAnim a; };
static std::vector<MatAnimEnt> g_mat_anim;
static uint64_t g_mat_anim_done = ~0ull;   // The cycle whose rewrite ran.
static uint64_t g_mat_anim_tcyc = ~0ull;   // The cycle g_mat_anim_t was taken in.
static double   g_mat_anim_t = 0.0;        // That cycle's session time (effect_time_seconds_d), s.
// At the main depth clear, right after the cycle counter moves: the cycle's time, while a clock-driven entry exists.
inline void mat_anim_stamp(uint64_t c) {
    if (g_mat_anim.empty()) return;
    g_mat_anim_tcyc = c;
    g_mat_anim_t = effect_time_seconds_d();
}
// The casters' skip keys (sun union and tiers, DLS, seam) mix this while a caster's set has uv_anim_alpha: a new key
// each cycle - whether or not anything bound a material yet - so an animated cutout's shadow is redrawn at the cycle's
// phase (its draw binds the material, which runs the rewrite). An opaque animated material's keys do not mix it.
inline uint64_t mat_anim_cycle_key() { return g_topo_cycles_pub.load(std::memory_order_relaxed); }
inline double mat_anim_time() {
    const uint64_t c = g_topo_cycles_pub.load(std::memory_order_relaxed);
    if (c != g_mat_anim_tcyc) {
        g_mat_anim_tcyc = c;
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
inline void uv_anim_lanes(const UvAnim& a, double t, KhGpuMat& g) {
    const double pi = 3.14159265358979323846;
    double th = static_cast<double>(a.rot) + static_cast<double>(a.rot_speed) * t;
    th = std::fmod(th, 360.0) * (pi / 180.0);
    const double cs = std::cos(th), sn = std::sin(th);
    const double m[4] = { a.scale[0] * cs, -a.scale[0] * sn,
                          a.scale[1] * sn, a.scale[1] * cs };
    const double c0[2] = { 0.5 - 0.5 * (m[0] + m[1]), 0.5 - 0.5 * (m[2] + m[3]) };
    double c[2], d[2];
    for (int k = 0; k < 2; ++k) {
        double o = static_cast<double>(a.offset[k]) + static_cast<double>(a.scroll[k]) * t;
        o -= std::floor(o);
        const double cf = c0[k] + o;
        c[k] = cf - std::floor(cf);
        d[k] = c[k] - c0[k];   // The translation the ripple is measured against: offset + whole uv.
    }
    double ph[2] = { 0.0, 0.0 };   // Zero, as the warp's lanes, while it is off.
    if (a.warp[0] > 0.0f) {
        double pt = static_cast<double>(a.warp[2]) * t;
        pt -= std::floor(pt);
        const double fq = a.warp[1];
        ph[0] = pt - fq * d[1];   // .x reads uv'.y.
        ph[1] = pt - fq * d[0];
        for (int k = 0; k < 2; ++k) ph[k] -= std::floor(ph[k]);
    }
    double fr = 0.0;
    if (a.flip[2] > 0.0f && a.flip[3] > 1.5f) {
        fr = std::floor(std::fmod(static_cast<double>(a.flip[2]) * t, static_cast<double>(a.flip[3])));
    }
    for (int k = 0; k < 4; ++k) g.an0[k] = static_cast<float>(m[k]);
    g.an1[0] = static_cast<float>(c[0]);
    g.an1[1] = static_cast<float>(c[1]);
    g.an1[2] = static_cast<float>(ph[0]);
    g.an1[3] = static_cast<float>(ph[1]);
    g.an2[0] = a.warp[0] > 0.0f ? a.warp[0] : 0.0f;
    g.an2[1] = a.warp[0] > 0.0f ? a.warp[1] : 0.0f;
    g.an2[2] = 0.0f;
    g.an2[3] = 0.0f;
    g.an3[0] = a.flip[0];
    g.an3[1] = a.flip[1];
    g.an3[2] = static_cast<float>(fr);
    g.an3[3] = 0.0f;
}
// The once-a-cycle rewrite (mat_gpu_upload).
inline void mat_anim_refresh() {
    const uint64_t c = g_topo_cycles_pub.load(std::memory_order_relaxed);
    if (c == g_mat_anim_done) return;
    g_mat_anim_done = c;
    const double t = mat_anim_time();
    for (const MatAnimEnt& e : g_mat_anim) {
        if (e.ix < g_mat_gpu.cpu.size()) uv_anim_lanes(e.a, t, g_mat_gpu.cpu[e.ix]);
    }
    g_mat_gpu.dirty = true;
}

// One slot's entry. Flags reflect what actually resolved (a missing texture drops its bit, and a route through it - the
// script's or the slot convention's - degrades to the shader's no-map default, ROUTE_ABSENT's one exception below),
// so the shader never samples a null layer.
inline void mat_gpu_fill(ID3D11DeviceContext* ctx, ID3D11Device* dev, const Material& m,
                         KhGpuMat& g, MaterialSet::Gpu::PageSet& pages) {
    int flags = 0;
    float lay[MAT_MAPS] = {};
    for (int s = 0; s < MAT_MAPS; ++s) {
        pages.p[s] = nullptr;
        if (!m.used || m.maps[s].path.empty()) continue;
        const TexRef r = tex_resolve(dev, ctx, m.maps[s].path, tex_map_srgb(m, s));
        if (!r.srv) continue;
        pages.p[s] = r.srv;
        lay[s] = static_cast<float>(r.layer);
        flags |= 1 << s;
    }
    // ROUTE_ABSENT: a script's route whose map did not resolve (pending, failed) reads the shader's no-map default
    // (cb.hlsl's contract: "with no map behind a route"), not the slot convention's channel of another map; that
    // convention serves an unrouted input, and a routed one whose call sets keep (its -1 reads a map: below).
    auto route_eff = [&](int user, int def_slot, int def_chan, bool keep = false) -> float {
        if (user >= 0 && (flags & (1 << (user >> 2)))) return static_cast<float>(user);
        if (user >= 0 && !keep) return -1.0f;
        if (def_slot >= 0 && (flags & (1 << def_slot))) return static_cast<float>(def_slot * 4 + def_chan);
        return -1.0f;
    };
    // The arma model reads its own conventions from the shared slots: AS.g is the occlusion, SMDI.b the gloss and
    // SMDI.g the specular intensity; it has no roughness or metal and does not take the pbr spec workflow.
    const bool arma = m.used && m.shader == 2;
    // USER_SLOTS - a .hlsl material's orm / specular slot follows the map the script named: "as" defaults the
    // occlusion to .g and leaves roughness and metal unrouted (AS carries neither; an explicit route still stands);
    // "smdi" defaults the gloss to .b and the specular intensity to .g and leaves the pbr spec workflow off (SMDI's rgb
    // is not an F0). Its arma lanes are filled whichever it named; its model lane stays 0 - the shader's own choice
    // (KhMatSurf::model).
    const bool mf_user = m.used && m.shader == 1;
    const bool as = arma || (mf_user && m.map_as);
    const bool smdi = arma || (mf_user && m.map_smdi);
    const bool lanes = arma || mf_user;
    // SPEC_AA: bit 15 arms the shader's Toksvig term (KhToksvigA2) for a normal map whose chain is the renderer's
    // own and lossless - the png / tga / bmp route (tex_build_mips): 8-bit texels box-averaged, never
    // renormalised, so a level's shortened normals measure the spread it averaged. A .jpg / .jpeg's compression error
    // shortens the texels too (blotchy roughness along its blocks), and a .dds / .paa keeps its file's levels,
    // renormalised by some tools or block-compressed: there the length is no measure, and the term stays off - the
    // test is the extension alone, so also for the levels DDS_MIP_FILL builds below a 32-bit file's last.
    if ((flags & 2) && (ends_with_ci(m.maps[1].path, ".png") || ends_with_ci(m.maps[1].path, ".tga") ||
                        ends_with_ci(m.maps[1].path, ".bmp"))) flags |= 1 << 15;
    g.p0[0] = static_cast<float>(flags | uv_anim_bits(m));   // UV_ANIM: bits 12 - 14.
    g.p0[1] = static_cast<float>(m.alpha_mode);   // 0 opaque, 1 cutout, 2 blend (the draw may
                                                       // override to 3 through matCtl.w).
    g.p0[2] = m.cutoff;
    g.p0[3] = m.normal_strength;
    g.p1[0] = m.base_color[0];
    g.p1[1] = m.base_color[1];
    g.p1[2] = m.base_color[2];
    g.p1[3] = m.roughness;
    g.p2[0] = arma ? 0.0f : m.metalness;
    g.p2[1] = m.emissive_intensity;
    g.p2[2] = route_eff(m.route_occ,   2, as ? 1 : 0);
    g.p2[3] = arma ? -1.0f : route_eff(m.route_rough, as ? -1 : 2, 1);
    g.p3[0] = arma ? -1.0f : route_eff(m.route_metal, as ? -1 : 2, 2);
    g.p3[1] = route_eff(m.route_alpha, 0, 3);
    // ROUTE_ABSENT: the shader's gloss at route -1 is slot 4's alpha (cb.hlsl KhSampleMatG; arma's: the
    // glossiness param) - under a .hlsl material's "smdi" that is no gloss, so its absent route keeps SMDI.b.
    g.p3[2] = route_eff(m.route_gloss, 4, smdi ? 2 : 3, mf_user && m.map_smdi);
    g.p3[3] = (!smdi && (flags & 16)) ? 1.0f : 0.0f;
    g.lay0[0] = lay[0]; g.lay0[1] = lay[1]; g.lay0[2] = lay[2]; g.lay0[3] = lay[3];
    g.lay1[0] = lay[4]; g.lay1[1] = lay[5];
    g.lay1[2] = lay[6]; g.lay1[3] = lay[7];   // USER_TEX: user0, user1.
    g.lay2[0] = lay[8]; g.lay2[1] = lay[9];   // user2..user5.
    g.lay2[2] = lay[10]; g.lay2[3] = lay[11];
    for (int k = 0; k < 3; ++k) g.p4[k] = lanes ? m.spec_color[k] : 0.0f;
    g.p4[3] = lanes ? m.glossiness : 0.0f;
    g.p5[0] = lanes ? m.fresnel_n : 0.0f;
    g.p5[1] = lanes ? m.fresnel_k : 0.0f;
    g.p5[2] = lanes ? route_eff(m.route_spec, smdi ? 4 : -1, 1) : -1.0f;
    g.p5[3] = arma ? 1.0f : 0.0f;
    // UV_ANIM: a clock-driven one's lanes at this cycle's time (mat_anim_refresh rewrites them each cycle).
    if (m.used && m.uv_anim) uv_anim_lanes(m.uv, m.uv_anim_time ? mat_anim_time() : 0.0, g);
}

// Uploads the set's slots into the table at first draw. Under the park invariant (render thread mid-frame or game
// thread under the lock).
inline void mat_gpu_ensure(ID3D11DeviceContext* ctx, ID3D11Device* dev, const MaterialSet& s) {
    static const Material me_default;
    for (int round = 0; round < 8; ++round) {   // Bounded: each round is a page grow.
        if (g_mat_gpu.page_gen != g_tex_page_gen) {   // A page moved: every entry's SRV/layer is stale.
            g_mat_gpu.cpu.clear();
            g_mat_anim.clear();   // UV_ANIM: its indices are the table's.
            std::lock_guard<std::mutex> l(g_mat_pool_mu);
            for (auto& kv : g_mat_pool) kv.second.set->gpu.base = -1;
            for (auto& g : g_mat_grave) if (g.set) g.set->gpu.base = -1;
            g_mat_gpu.page_gen = g_tex_page_gen;
        }
        if (s.gpu.base >= 0) return;
        const uint32_t gen0 = g_tex_page_gen;
        const size_t n = s.slots.empty() ? 1 : s.slots.size();
        s.gpu.pages.assign(n, MaterialSet::Gpu::PageSet());
        const int base = static_cast<int>(g_mat_gpu.cpu.size());
        uint64_t sig = CryptoGenerator::FNV1A64_OFFSET;
        for (size_t i = 0; i < n; ++i) {
            const Material& m = i < s.slots.size() ? s.slots[i] : me_default;
            KhGpuMat g = {};
            mat_gpu_fill(ctx, dev, m, g, s.gpu.pages[i]);
            g_mat_gpu.cpu.push_back(g);
            if (m.used && m.uv_anim_time)   // UV_ANIM: rewritten every cycle (mat_anim_refresh).
                g_mat_anim.push_back(MatAnimEnt{ static_cast<uint32_t>(g_mat_gpu.cpu.size() - 1), m.uv });
            // Batch identity: the pages bound per slot, the alpha mode (the representative's material decides the blend
            // split for every member) and the shader kind. Layers, colours, cutoffs and routes ride the table and do
            // not split batches.
            for (int p = 0; p < MAT_MAPS; ++p) {
                const uintptr_t v = reinterpret_cast<uintptr_t>(s.gpu.pages[i].p[p]);
                sig = CryptoGenerator::fnv1a64_update(sig, &v, sizeof(v));
            }
            const int k = m.used ? m.shader : -1;
            const int a = m.used ? m.alpha_mode : 0;
            sig = CryptoGenerator::fnv1a64_update(sig, &k, sizeof(k));
            sig = CryptoGenerator::fnv1a64_update(sig, &a, sizeof(a));
            if (m.used && m.shader == 1) {
                sig = CryptoGenerator::fnv1a64_update(sig, m.user_shader.data(), m.user_shader.size());
            }
        }
        // A load grew a page mid-fill: the entries above name the old SRVs. Round again; the top clears the table.
        if (gen0 != g_tex_page_gen) {
            g_mat_gpu.dirty = true;
            continue;
        }
        s.gpu.base = base;
        s.gpu.page_sig = sig;
        g_mat_gpu.dirty = true;
        return;
    }
}

// The table itself: a dynamic structured buffer re-uploaded whole when dirty. It holds an entry per slot of every
// set ensured since the last g_tex_page_gen change, live or not: a script that sends a changed material each frame
// interns a set a frame, each adding its slots (and a whole upload), until material_gc graves them
// MAT_GC_IDLE_MS after their last reference and rebuilds. Bound at t38 for the pixel stage.
inline bool mat_gpu_upload(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    if (g_mat_gpu.cpu.empty()) return false;
    if (!g_mat_anim.empty()) mat_anim_refresh();   // UV_ANIM: once a cycle; marks the table dirty.
    if (g_mat_gpu.dirty || !g_res.mat_sb) {
        const uint32_t n = static_cast<uint32_t>(g_mat_gpu.cpu.size());
        if (!g_res.mat_sb || g_res.mat_cap < n) {
            KH_SAFE_RELEASE(g_res.mat_srv);
            KH_SAFE_RELEASE(g_res.mat_sb);
            uint32_t cap = g_res.mat_cap ? g_res.mat_cap : 64;
            while (cap < n) cap *= 2;
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = cap * sizeof(KhGpuMat);
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            bd.StructureByteStride = sizeof(KhGpuMat);
            if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_res.mat_sb))) { g_res.mat_sb = nullptr; return false; }
            D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
            sd.Format = DXGI_FORMAT_UNKNOWN;
            sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            sd.Buffer.FirstElement = 0;
            sd.Buffer.NumElements = cap;
            if (FAILED(dev->CreateShaderResourceView(g_res.mat_sb, &sd, &g_res.mat_srv))) {
                KH_SAFE_RELEASE(g_res.mat_sb); g_res.mat_srv = nullptr; return false;
            }
            g_res.mat_cap = cap;
        }
        D3D11_MAPPED_SUBRESOURCE map = {};
        if (FAILED(ctx->Map(g_res.mat_sb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map))) return false;
        memcpy(map.pData, g_mat_gpu.cpu.data(), n * sizeof(KhGpuMat));
        ctx->Unmap(g_res.mat_sb, 0);
        g_mat_gpu.dirty = false;
    }
    return g_res.mat_srv != nullptr;
}

// Per-draw material bind: the slot's pages at t14-t18, t42 and t43-t48 (USER_TEX), the table at t38, the matCtl
// lanes. set == nullptr binds the one-slot default set. mode >= 0 overrides the alpha mode for this draw (the
// blend split's opaque part, 3).
inline void bind_material(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                          ConstantData& cbd, const MaterialSet* set, size_t slot,
                          float mode = -1.0f) {
    // A slot the set does not carry draws the default material; that bind is uniform across an instanced batch
    // (matCtl.z).
    bool uniform = false;
    if (!set || slot >= (set->slots.empty() ? 1 : set->slots.size())) {
        set = default_material_set(); slot = 0; uniform = true;
    }
    mat_gpu_ensure(ctx, dev, *set);
    if (slot >= set->gpu.pages.size()) slot = 0;
    const bool ok = mat_gpu_upload(ctx, dev) && set->gpu.base >= 0;
    ID3D11ShaderResourceView* srvs[MAT_MAPS] = {};
    if (ok) {
        for (int s = 0; s < MAT_MAPS; ++s) srvs[s] = set->gpu.pages[slot].p[s];
    }
    // BIND_FILTER: each range is bound unless the pass's record says it holds these views already.
    const bool bc = bc_on(ctx);
    if (!bc || !t_kh_bc.k14 || memcmp(t_kh_bc.v14, srvs, sizeof(t_kh_bc.v14)) != 0) {
        ctx->PSSetShaderResources(14, 5, srvs);
        if (bc) { memcpy(t_kh_bc.v14, srvs, sizeof(t_kh_bc.v14)); t_kh_bc.k14 = true; }
    }
    if (!bc || !t_kh_bc.k42 || t_kh_bc.v42 != srvs[5]) {
        ctx->PSSetShaderResources(42, 1, &srvs[5]);   // SPECCOLOR (cb.hlsl matSpecColor).
        if (bc) { t_kh_bc.v42 = srvs[5]; t_kh_bc.k42 = true; }
    }
    // USER_TEX: user0..user5 (cb.hlsl khUserMap0..5), null for every material without them; inside StateBackup's
    // t0..t69.
    if (!bc || !t_kh_bc.k43 ||
        memcmp(t_kh_bc.v43, &srvs[MAT_MAPS_BUILTIN], sizeof(t_kh_bc.v43)) != 0) {
        ctx->PSSetShaderResources(43, MAT_USER_MAPS, &srvs[MAT_MAPS_BUILTIN]);
        if (bc) { memcpy(t_kh_bc.v43, &srvs[MAT_MAPS_BUILTIN], sizeof(t_kh_bc.v43)); t_kh_bc.k43 = true; }
    }
    if (!bc || !t_kh_bc.k38 || t_kh_bc.v38 != g_res.mat_srv) {
        ctx->PSSetShaderResources(38, 1, &g_res.mat_srv);
        if (bc) { t_kh_bc.v38 = g_res.mat_srv; t_kh_bc.k38 = true; }
    }
    cbd.mat_ctl[0] = ok ? static_cast<float>(set->gpu.base + static_cast<int>(slot)) : 0.0f;
    cbd.mat_ctl[1] = static_cast<float>(slot);
    cbd.mat_ctl[2] = uniform ? 1.0f : 0.0f;
    cbd.mat_ctl[3] = mode;
}

// The instance stream's material base for an object (0 = the default set).
inline float mat_inst_base(ID3D11DeviceContext* ctx, ID3D11Device* dev, const MaterialSet* set) {
    if (!set) set = default_material_set();
    mat_gpu_ensure(ctx, dev, *set);
    return set->gpu.base >= 0 ? static_cast<float>(set->gpu.base) : 0.0f;
}

// Per-submesh textured draws for one object: binds per submesh, uploads the object slice (TX_ONE_UPLOAD: in the
// injection, the flush and the PIP a textured object's only upload), draws the range, two-pass when ts_ordered.
// Preconditions: textured VS/PS/layout bound, VB + blend + dss set, cbd filled.
// part: 1 = the depth-writing draw (a blend material contributes its solid texels only, matParams0.y = 3).
inline uint32_t draw_textured(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                              ConstantData& cbd, ID3D11Buffer* obj_cb,
                              const MaterialSet& ms, int mid, bool ts_ordered,
                              ID3D11RasterizerState*& bound_rs,
                              int um_ctx, ID3D11PixelShader* builtin_ps,
                              int lod, int part,
                              uint32_t inst_n = 0, uint32_t inst_first = 0) {
    static const Material tx_default;   // Unset slots draw with defaults.
    const MeshDef& md = mesh_def(mid);
    uint32_t draws = 0;
    auto draw = [&](const MeshSubmesh& um_sm) {
        if (inst_n) {
            ctx->DrawIndexedInstanced(um_sm.index_count, inst_n, um_sm.index_start, 0, inst_first);
        } else {
            ctx->DrawIndexed(um_sm.index_count, um_sm.index_start, 0);
        }
        ++draws;
    };
    ID3D11PixelShader* bound = builtin_ps;
    // The level's submesh table: cardinality and order match level 0's (validated at cache load), so si addresses
    // the same material slot at every level.
    const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, lod);

    for (size_t si = 0; si < tab.size(); ++si) {
        const MeshSubmesh& sm = tab[si];
        if (sm.index_count == 0) continue;
        const Material& mat = (si < ms.slots.size() && ms.slots[si].used)
                            ? ms.slots[si] : tx_default;
        // Part 2 = the translucent part. User material shader: per-submesh PS selection; um_ctx names the caller's
        // pipeline variant so the user twin matches its builtin.
        if (part == 2 && mat.alpha_mode != 2) continue;
        ID3D11PixelShader* want = builtin_ps;
        // A submesh that asked for a user shader and has not got one (pending or failed) draws PSWhite (flat unlit
        // white), not builtin PBR (builtin PBR only while the WHITE_PREVIEW pixel shader is missing). PSWhite is the
        // colour alone - none of the replaced PS's per-pixel cuts (the LOD dither, our own near plane, the
        // view-distance cut, the terrain lane, the guard) - so it can show past the object's view distance or through
        // a ridge its siblings respect, and draws both levels whole in an LOD fade: briefly while the compile is
        // pending or queued, for good when it failed (reported once).
        bool white = false;

        if (mat.shader == 1 && !mat.user_shader.empty()) {
            ID3D11PixelShader* ps = user_mat_ps(dev, mat.user_shader, um_ctx);
            if (ps) {
                want = ps;
            } else if (g_res.ps_white) {
                want = g_res.ps_white;
                white = true;
            }
        }

        if (want != bound) {
            ctx->PSSetShader(want, nullptr, 0);
            bound = want;
        }

        // The substitute takes the default set rather than the real one.
        bind_material(ctx, dev, cbd, white ? nullptr : &ms, si,
                      (part == 1 && !white && mat.alpha_mode == 2) ? 3.0f : -1.0f);
        if (!upload_obj_cb(ctx, obj_cb, cbd)) break;

        if (ts_ordered) {   // Interiors first, exteriors over them (the ordered contract).
            if (bound_rs != g_res.rasterizer_front) {
                ctx->RSSetState(g_res.rasterizer_front);
                bound_rs = g_res.rasterizer_front;
            }
            draw(sm);
            ctx->RSSetState(g_res.rasterizer_cull);
            bound_rs = g_res.rasterizer_cull;
            draw(sm);
        } else {
            draw(sm);
        }
    }

    // Leave the caller's builtin bound: its PS trackers assume it.
    if (bound != builtin_ps) ctx->PSSetShader(builtin_ps, nullptr, 0);
    return draws;
}

// Every material set a pass will draw is uploaded before the pass plans: a first-sight load grows a page and
// rebuilds the table, which would leave a base already written into a lane stale. vis is the pass's own
// visibility predicate, so a culled object's first-sight resolve costs the pass nothing.
template <class MatVis>
inline void mat_ensure_all(ID3D11DeviceContext* ctx, ID3D11Device* dev,
                           const std::vector<RenderObject>& meshes, MatVis vis) {
    for (int round = 0; round < 8; ++round) {
        const uint32_t gen = g_tex_page_gen;
        mat_gpu_ensure(ctx, dev, *default_material_set());
        for (const RenderObject& o : meshes) {
            const MaterialSet* s = obj_textured(o);
            if (!s) continue;
            if (!vis(o)) continue;
            mat_gpu_ensure(ctx, dev, *s);
        }
        if (gen == g_tex_page_gen) return;
    }
}
}