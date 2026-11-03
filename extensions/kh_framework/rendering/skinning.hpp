#pragma once

// The feature half (rendering_integration_features.hpp): SKIN_GPU / SKEL: skinning on the CPU and the GPU (stream
// output), the skin pools, the sync and upload. Never include it on its own.
namespace RenderIntegration {
inline void skin_release_all();   // SKEL: the skinned vertex buffers (defined after the cloth runtime).
inline void uvs_src_forget(uint32_t slot);   // UVS_SRC_FORGET: a slot's source buffer was replaced.
inline void skin_drop_all();
// SKEL_DRAW (defined with the skin): the consume-point skinning of one skeletal binding, from the stash
// attach_step just took, in three parts (SKIN_FORK): the maps (render thread), the vertices (any thread - the
// pool's), the upload and box (render thread).
inline bool skin_draw_prep(Attach& a, const RenderObject& o, ID3D11DeviceContext* ctx);
inline void skin_draw_verts(Attach& a, float bc[3], float bs[3]);
inline void skin_draw_finish(const std::string& h, Attach& a, RenderObject& o,
                             ID3D11DeviceContext* ctx, const float bc[3], const float bs[3],
                             bool& moved, const VbRef& so_vb);
// Does something other than the draw read this instance's pose (its cloth's guide, its collider)? The step's stash
// gate for a hidden mesh (defined with the skin).
inline bool skin_pose_read(const SkinInst& in);
// SKIN_GPU (defined after the hook state it raises): the GPU skinning of every binding of a step that can take it.
inline void skin_so_batch(ID3D11DeviceContext* ctx, std::vector<SkinDrawJob>& jobs);

// Defined in rendering/device_resources.hpp (declared here for the uses below).
inline bool ensure_ok(const char* what, const std::string& err);

// SKIN_CASTER: whether a skeletal binding's pose is drawn - it is visible, or a casterOnly binding a shadow map
// takes (shadow_active: it casts and takes part - lit, no effect, depth on - so the sun / DLS maps draw it;
// uvs_step's gate calls this). The step's stash and skin gates and skin_sync's ask this, so a hidden
// caster does not cast a frozen pose. SKIN_CASTER_VM: an inFront one in any view (shadow_active_any_view: the
// slice term left out) - the slice verdict moves on the render thread at the main depth clear, after the game
// thread's step, so a gate that took it would give the maps a pose one or more frames old at a third -> first
// person switch.
inline bool skin_pose_drawn(const RenderObject& o) {
    return obj_vis(o) || (o.caster_only && shadow_active_any_view(o));   // SHOW_INFRONT.
}
inline void skin_stat_note(uint32_t gpu, uint32_t cpu, uint32_t cull) {
    if (g_skin_stat_cycle != g_topo_cycles) {
        if (g_skin_stat_cycle != ~0ull) {
            g_skin_so_n.store(g_skin_stat_gpu, std::memory_order_relaxed);
            g_skin_cpu_n.store(g_skin_stat_cpu, std::memory_order_relaxed);
            g_skin_cull_n.store(g_skin_stat_cull, std::memory_order_relaxed);
        }
        g_skin_stat_cycle = g_topo_cycles;
        g_skin_stat_gpu = 0u;
        g_skin_stat_cpu = 0u;
        g_skin_stat_cull = 0u;
    }
    g_skin_stat_gpu += gpu;
    g_skin_stat_cpu += cpu;
    g_skin_stat_cull += cull;
}
inline void skin_nrm_cof(const float r[3][3], const float nrm[3], float nn[3]) {
    const float* const r0 = r[0];
    const float* const r1 = r[1];
    const float* const r2 = r[2];
    const float* const n = nrm;
    nn[0] = n[0] * r0[0] + n[1] * r0[1] + n[2] * r0[2];   // n . B: dot(n, row c).
    nn[1] = n[0] * r1[0] + n[1] * r1[1] + n[2] * r1[2];
    nn[2] = n[0] * r2[0] + n[1] * r2[1] + n[2] * r2[2];
    // x0 / x1 / x2 = cross(r1, r2) / cross(r2, r0) / cross(r0, r1) in HLSL cross's terms; c = their dots with n.
    const float x0[3] = { r1[1] * r2[2] - r1[2] * r2[1], r1[2] * r2[0] - r1[0] * r2[2], r1[0] * r2[1] - r1[1] * r2[0] };
    const float x1[3] = { r2[1] * r0[2] - r2[2] * r0[1], r2[2] * r0[0] - r2[0] * r0[2], r2[0] * r0[1] - r2[1] * r0[0] };
    const float x2[3] = { r0[1] * r1[2] - r0[2] * r1[1], r0[2] * r1[0] - r0[0] * r1[2], r0[0] * r1[1] - r0[1] * r1[0] };
    const float c[3] = { n[0] * x0[0] + n[1] * x0[1] + n[2] * x0[2], n[0] * x1[0] + n[1] * x1[1] + n[2] * x1[2],
                         n[0] * x2[0] + n[1] * x2[1] + n[2] * x2[2] };
    const float lc = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
    const float la = nn[0] * nn[0] + nn[1] * nn[1] + nn[2] * nn[2];
    if (!(lc > SKIN_NRM_LC_MIN && lc < SKIN_NRM_L_MAX && la < SKIN_NRM_L_MAX)) return;
    const float det = r0[0] * x0[0] + r0[1] * x0[1] + r0[2] * x0[2];
    const float g = (det >= 0.0f ? 1.0f : -1.0f) * (sqrtf(la) / sqrtf(lc));
    for (int k = 0; k < 3; ++k) nn[k] = c[k] * g;
}
// The upload's publish, under g_draw_list_mutex (the box writes' scope). The table is handed over whole and never
// written after, so the cloth sync shares it.
inline void skin_col_publish(SkinColMap& col) {
    std::shared_ptr<const SkinColMap> t = std::make_shared<const SkinColMap>(std::move(col));
    std::lock_guard<std::mutex> l(g_skin_col_mu);
    g_skin_col.swap(t);
    g_skin_col_pub = g_skin_col_stamp;
}   // The previous table is released with the swapped-out local, after the lock.
// The cloth sync's take, once per call: under g_draw_list_mutex beside the
// object copy when it simulates (snap), alone on the inert path.
inline void skin_col_take(bool snap) {
    std::lock_guard<std::mutex> l(g_skin_col_mu);
    g_skin_col_lo = g_skin_col_hi;
    g_skin_col_hi = g_skin_col_pub;
    if (snap) g_skin_col_snap = g_skin_col;
}
inline bool skin_col_fresh(uint64_t s) {
    return s > g_skin_col_lo && s <= g_skin_col_hi;
}
// One bone's tree over ix (indices into sb_p, 3 floats a point; reordered in place), appended to s.
// Returns the root.
inline uint32_t skin_sup_build(SkinSup& s, const std::vector<float>& sb_p, std::vector<uint32_t>& ix) {
    struct SbTask { uint32_t node, lo, hi; };
    const uint32_t root = static_cast<uint32_t>(s.node.size());
    s.node.emplace_back();
    std::vector<SbTask> st;
    st.push_back(SbTask{ root, 0u, static_cast<uint32_t>(ix.size()) });
    while (!st.empty()) {
        const SbTask t = st.back();
        st.pop_back();
        float mn[3] = { 3.0e38f, 3.0e38f, 3.0e38f }, mx[3] = { -3.0e38f, -3.0e38f, -3.0e38f };
        for (uint32_t i = t.lo; i < t.hi; ++i) {
            const float* p = &sb_p[static_cast<size_t>(ix[i]) * 3u];
            for (int k = 0; k < 3; ++k) { if (p[k] < mn[k]) mn[k] = p[k]; if (p[k] > mx[k]) mx[k] = p[k]; }
        }
        {
            SkinSupNode& n = s.node[t.node];
            memcpy(n.mn, mn, sizeof(mn));
            memcpy(n.mx, mx, sizeof(mx));
            n.left = 0u; n.first = 0u; n.count = 0u;
        }
        const uint32_t sb_n = t.hi - t.lo;
        int ax = 0;
        if (mx[1] - mn[1] > mx[ax] - mn[ax]) ax = 1;
        if (mx[2] - mn[2] > mx[ax] - mn[ax]) ax = 2;
        if (sb_n <= SKIN_SUP_LEAF || !(mx[ax] - mn[ax] > 0.0f)) {   // Few points, or all at one place: a leaf.
            SkinSupNode& n = s.node[t.node];
            n.first = static_cast<uint32_t>(s.pt.size() / 3u);
            n.count = sb_n;
            for (uint32_t i = t.lo; i < t.hi; ++i) {
                const float* p = &sb_p[static_cast<size_t>(ix[i]) * 3u];
                s.pt.push_back(p[0]); s.pt.push_back(p[1]); s.pt.push_back(p[2]);
            }
            continue;
        }
        const uint32_t mid = t.lo + sb_n / 2u;
        std::nth_element(ix.begin() + t.lo, ix.begin() + mid, ix.begin() + t.hi,
                         [&](uint32_t a, uint32_t b) { return sb_p[static_cast<size_t>(a) * 3u + ax] < sb_p[static_cast<size_t>(b) * 3u + ax]; });
        const uint32_t l = static_cast<uint32_t>(s.node.size());
        s.node.emplace_back();
        s.node.emplace_back();
        s.node[t.node].left = l;
        st.push_back(SbTask{ l,      t.lo,  mid });
        st.push_back(SbTask{ l + 1u, mid,   t.hi });
    }
    return root;
}
// A node's box support along u: no point under it has a larger p . u (each product and sum rounds monotonically;
// a fused multiply-add can exceed it by about an ulp - see skin_so_bound).
// SKIN_BOUND_PRUNE: skin_sup_max's own test and skin_so_bound's skip share it.
inline float skin_sup_box(const SkinSupNode& n, const float u[3]) {
    return (u[0] > 0.0f ? u[0] * n.mx[0] : u[0] * n.mn[0]) +
           (u[1] > 0.0f ? u[1] * n.mx[1] : u[1] * n.mn[1]) +
           (u[2] > 0.0f ? u[2] * n.mx[2] : u[2] * n.mn[2]);
}
// The support of one bone's points along u: max over them of p . u. Exact.
inline float skin_sup_max(const SkinSup& s, uint32_t root, const float u[3]) {
    uint32_t st[96];
    uint32_t sm_n = 0u;
    float best = -3.0e38f;
    auto bound = [&](const SkinSupNode& n) { return skin_sup_box(n, u); };
    st[sm_n++] = root;
    while (sm_n != 0u) {
        const SkinSupNode& n = s.node[st[--sm_n]];
        // The box's own support, a hair up: its sum rounds apart from a point's.
        const float b = bound(n);
        if (!(b + fabsf(b) * 1.0e-6f > best)) continue;
        if (n.count != 0u) {
            const float* p = &s.pt[static_cast<size_t>(n.first) * 3u];
            for (uint32_t i = 0; i < n.count; ++i, p += 3) {
                const float d = p[0] * u[0] + p[1] * u[1] + p[2] * u[2];
                if (d > best) best = d;
            }
            continue;
        }
        if (sm_n + 2u > 96u) {   // Never reached by a median tree (depth ~ log2 n); scan rather than drop.
            const float d = b;
            if (d > best) best = d;   // The box's support: conservative.
            continue;
        }
        const float bl = bound(s.node[n.left]), br = bound(s.node[n.left + 1u]);
        if (bl > br) { st[sm_n++] = n.left + 1u; st[sm_n++] = n.left; }   // The better child is popped first.
        else                   { st[sm_n++] = n.left;      st[sm_n++] = n.left + 1u; }
    }
    return best;
}
// SKIN_GPU: the trees and boxes of a snapshot (verts / inf / drawn / ctr / ext set), for nb bones. Game
// thread, skin_rest_of.
inline void skin_sup_prepare(SkinRest& r, size_t nb) {
    SkinSup& s = r.sup;
    const size_t nv = r.verts.size();
    if (nb == 0u || r.inf.size() != nv) return;   // No influences: every vertex stands at rest; the CPU path keeps it.
    std::vector<float> p(nv * 3u);
    for (size_t i = 0; i < nv; ++i) {
        for (int k = 0; k < 3; ++k) p[i * 3u + k] = r.verts[i].pos[k] * r.ext[k] + r.ctr[k];   // skin_verts' authored position.
    }
    const uint8_t* dr = r.drawn.size() == nv ? r.drawn.data() : nullptr;
    std::vector<std::vector<uint32_t>> set(nb);
    float id_mn[3] = { 3.0e38f, 3.0e38f, 3.0e38f }, id_mx[3] = { -3.0e38f, -3.0e38f, -3.0e38f };
    for (size_t i = 0; i < nv; ++i) {
        if (dr && dr[i] == 0u) continue;   // The box is the drawn vertices' (skin_verts).
        const SkinInf& f = r.inf[i];
        float ws = 0.0f;
        for (int k = 0; k < 4; ++k) {
            if (!(f.w[k] > 0.0f) || f.b[k] >= nb) continue;   // skin_verts' own skip.
            std::vector<uint32_t>& v = set[f.b[k]];
            if (v.empty() || v.back() != static_cast<uint32_t>(i)) v.push_back(static_cast<uint32_t>(i));   // A bone named twice by one vertex files it once.
            ws += f.w[k];
        }
        // The bound is a convex-combination argument and skin_verts does not renormalise, so weights summing past
        // one (beyond the float slack the pad covers) would carry a vertex outside it: no trees, the mesh keeps the
        // CPU path with its exact box.
        if (ws > 1.0f + SKIN_SO_ROOT_W) { s = SkinSup{}; r.bone_box.clear(); return; }
        if ((ws < 1.0f ? 1.0f - ws : 0.0f) > SKIN_SO_ROOT_W) {
            for (int k = 0; k < 3; ++k) { const float a = p[i * 3u + k]; if (a < id_mn[k]) id_mn[k] = a; if (a > id_mx[k]) id_mx[k] = a; }
            s.id_any = true;
        }
    }
    memcpy(s.id_mn, id_mn, sizeof(id_mn));
    memcpy(s.id_mx, id_mx, sizeof(id_mx));
    s.root.assign(nb, 0xFFFFFFFFu);
    r.bone_box.assign(nb * 6u, 0.0f);
    for (size_t b = 0; b < nb; ++b) {
        float* bx = &r.bone_box[b * 6u];
        bx[0] = bx[1] = bx[2] = 1.0f; bx[3] = bx[4] = bx[5] = -1.0f;   // None.
        if (set[b].empty()) continue;
        s.root[b] = skin_sup_build(s, p, set[b]);
        const SkinSupNode& n = s.node[s.root[b]];
        memcpy(bx, n.mn, sizeof(n.mn));
        memcpy(bx + 3, n.mx, sizeof(n.mx));
    }
    // SKIN_SO_PAL: every bone a weight names - over every vertex, drawn or not, as the stream carries them all,
    // with skin_verts' own skip (a weight not above zero, a bone past the skeleton). A bone no vertex carries (an
    // ancestor skin_finish keeps for the chain) takes no palette row.
    {
        std::vector<uint8_t> on(nb, 0u);
        for (const SkinInf& f : r.inf) {
            for (int k = 0; k < 4; ++k) {
                if (f.w[k] > 0.0f && f.b[k] < nb) on[f.b[k]] = 1u;
            }
        }
        for (size_t b = 0; b < nb; ++b) {
            if (on[b] != 0u) s.pal.push_back(static_cast<uint32_t>(b));
        }
    }
    s.ok = true;
}
// SKIN_GPU: the box of the pose a (skin_affine_of's layout, the snapshot's bone count) - skin_verts'
// bc / bs contract, from the trees. Contains the skinned drawn vertices - SKIN_BOUND_PRUNE: those whose weight to
// the root is above SKIN_SO_ROOT_W; one at or below it is stood in by its bones' images alone, so it can lie
// outside by up to that share of its bone's distance from rest (centimetres for a bone hundreds of metres out) - the
// box only, the drawn position is exact either way. Equals the exact box plus SKIN_SO_PAD_M a side wherever the
// extreme vertex follows one bone.
// SKIN_BOUND_PRUNE: a bone whose root box (skin_sup_box, the tree's own first test) cannot move the running
// extreme along an axis skips that query - its exact value could not be taken (hi > mx / lo < mn are strict, and
// the box bounds every point: the tree prunes by the same test). The same box, bit for bit, as querying every bone
// wherever products and sums round separately (no contraction: MSVC x64 /fp:precise or /fp:strict); a build that
// fuses them (FMA under /fp:fast or /fp:contract) can differ by about an ulp of the terms, far inside
// SKIN_SO_PAD_M, as the tree's own pruning already can. A NaN box never skips.
inline void skin_so_bound(const SkinRest& r, const float* a, float bc[3], float bs[3]) {
    const SkinSup& s = r.sup;
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    if (s.id_any) { memcpy(mn, s.id_mn, sizeof(mn)); memcpy(mx, s.id_mx, sizeof(mx)); }
    for (size_t b = 0; b < s.root.size(); ++b) {
        if (s.root[b] == 0xFFFFFFFFu) continue;
        const float* m = a + b * 12u;
        const SkinSupNode& n = s.node[s.root[b]];
        for (int c = 0; c < 3; ++c) {   // v'[c] = v . (m[c], m[3 + c], m[6 + c]) + m[9 + c].
            const float u[3] = { m[c], m[3 + c], m[6 + c] };
            const float v[3] = { -u[0], -u[1], -u[2] };
            const float hb = skin_sup_box(n, u), lb = skin_sup_box(n, v);
            if (!(hb + fabsf(hb) * 1.0e-6f + m[9 + c] <= mx[c])) {
                const float hi = skin_sup_max(s, s.root[b], u) + m[9 + c];
                if (hi > mx[c]) mx[c] = hi;
            }
            if (!(-(lb + fabsf(lb) * 1.0e-6f) + m[9 + c] >= mn[c])) {
                const float lo = -skin_sup_max(s, s.root[b], v) + m[9 + c];
                if (lo < mn[c]) mn[c] = lo;
            }
        }
    }
    for (int k = 0; k < 3; ++k) {
        if (!(mn[k] <= mx[k])) { mn[k] = mx[k] = 0.0f; }   // Nothing drawn (skin_verts).
        else { mn[k] -= SKIN_SO_PAD_M; mx[k] += SKIN_SO_PAD_M; }
        bc[k] = 0.5f * (mn[k] + mx[k]);
        const float sz = mx[k] - mn[k];
        bs[k] = sz > 1.0e-4f ? sz : 1.0e-4f;
    }
}
inline VbRef skin_gpu_ref(const SkinGpu& g) {
    VbRef r;
    r.vb = g.vb;
    r.off = g.off;
    return r;
}
inline bool skin_gpu_retry_open(uint64_t at) {   // ~0 = never refused; a counter that restarted opens it.
    return at == ~0ull || g_topo_cycles < at || g_topo_cycles - at >= SKIN_GPU_RETRY;
}
// Whether skin_so_ensure may be asked now: the objects never failed, or a refusal's retry is due.
inline bool skin_so_askable() {
    return !g_skin_so_failed.load(std::memory_order_relaxed) ||
           (g_skin_so_retry_at != ~0ull && skin_gpu_retry_open(g_skin_so_retry_at));
}
inline bool skin_so_big_ready() {
    return g_res.skin_so_vs_l && g_res.skin_so_gs_l && g_res.skin_so_cb_l;
}
inline bool skin_so_big_askable() {
    if (g_shbatch_state != 2) return false;   // The prewarm is in flight (or not armed yet): not now.
    return !g_skin_so_big_failed || (g_skin_so_big_retry_at != ~0ull && skin_gpu_retry_open(g_skin_so_big_retry_at));
}
// SKIN_SO_LAZY: the snapshot's palette has a class whose objects stand - the small one always (its own objects
// are the callers' test), the large one once skin_so_big_ensure made it. Every admission site asks this.
inline bool skin_so_class_ok(const SkinRest& r) {
    const size_t n = r.sup.pal.size();
    return n <= SKIN_SO_BONES_S || (n <= KH_SKIN_SO_BONES && skin_so_big_ready());
}
inline uint32_t skin_pool_maxcap(UINT bytes) {   // A chunk's most slices.
    return static_cast<uint32_t>(std::max<UINT>(1u, SKIN_POOL_CHUNK_BYTES / bytes));
}
// A chunk's buffer and view for cap slices of bytes; false = neither made.
inline bool skin_pool_buffer(ID3D11Device* dev, uint32_t cap, UINT bytes, ID3D11Buffer*& vb,
                             ID3D11ShaderResourceView*& srv) {
    vb = nullptr;
    srv = nullptr;
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = cap * bytes;   // At most max(64 MB, one slice).
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateBuffer(&bd, nullptr, &vb)) || !vb) {
        vb = nullptr;
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_R32G32B32A32_UINT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sd.Buffer.FirstElement = 0;
    sd.Buffer.NumElements = bd.ByteWidth / 16u;   // 48 = 3 x 16: a vertex is three elements.
    if (FAILED(dev->CreateShaderResourceView(vb, &sd, &srv)) || !srv) {
        srv = nullptr;
        KH_SAFE_RELEASE(vb);
        return false;
    }
    g_skin_pool_made = true;   // SKIN_POOL_MADE: the device takes the kind.
    return true;
}
// A chunk's buffer changed (skin_pool_grow): every binding in it, and every slot table element the walk
// installed for one, takes the new pointer. The offsets stand.
inline void skin_pool_rebind(const SkinPoolChunk& c) {
    for (auto& kv : g_skin_gpu) {
        if (kv.second.chunk == &c) kv.second.vb = c.vb;
    }
    for (size_t s = 0; s < g_cloth_pool_slot.size() && s < g_cloth_vb_slot.size(); ++s) {
        if (g_cloth_pool_slot[s] == &c) g_cloth_vb_slot[s] = c.vb;
    }
}
// The chunk grown to to slices (more than it has) in place: a new buffer with the old one copied to its start,
// so every slice keeps its offset, bytes and base. False = refused (the chunk stands as it was).
inline bool skin_pool_grow(ID3D11Device* dev, ID3D11DeviceContext* ctx, SkinPoolChunk& c,
                           UINT bytes, uint32_t to) {
    const uint32_t cap = static_cast<uint32_t>(c.used.size());
    if (to <= cap) return false;
    ID3D11Buffer* vb = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    if (!skin_pool_buffer(dev, to, bytes, vb, srv)) return false;
    ctx->CopySubresourceRegion(vb, 0, 0, 0, 0, c.vb, 0, nullptr);   // The whole old buffer, at 0.
    KH_SAFE_RELEASE(c.srv);
    KH_SAFE_RELEASE(c.vb);
    c.vb = vb;
    c.srv = srv;
    c.used.resize(to, 0u);
    skin_pool_rebind(c);
    return true;
}
// A slice of the pool for (mesh, bytes) into g (vb / off / chunk / slice); false = none (the caller
// makes a buffer of the binding's own). g holds no buffer on entry. The first free slice in chunk order; else
// the last chunk doubles; else (it is at the most a chunk takes) a new chunk of one slice.
inline bool skin_pool_take(ID3D11Device* dev, ID3D11DeviceContext* ctx, int mesh, UINT bytes,
                           SkinGpu& g) {
    if (g_skin_pool_failed || !dev || !ctx || mesh < 0 || bytes == 0u ||
        bytes % static_cast<UINT>(sizeof(MeshVertex)) != 0u) return false;
    const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(mesh)) << 32) | bytes;
    SkinPool& p = g_skin_pool[key];
    const uint32_t pt_max = skin_pool_maxcap(bytes);
    SkinPoolChunk* c = nullptr;
    uint32_t s = 0u;
    for (const std::unique_ptr<SkinPoolChunk>& up : p.chunks) {
        if (up->live >= up->used.size()) continue;
        for (uint32_t k = 0; k < static_cast<uint32_t>(up->used.size()); ++k) {
            if (!up->used[k]) { c = up.get(); s = k; break; }
        }
        if (c) break;
    }
    if (!c && !p.chunks.empty()) {
        SkinPoolChunk& l = *p.chunks.back();
        const uint32_t was = static_cast<uint32_t>(l.used.size());
        if (skin_pool_grow(dev, ctx, l, bytes, std::min<uint32_t>(was * 2u, pt_max))) {
            c = &l;
            s = was;
        } else if (was < pt_max) {
            return false;   // A refused growth: this binding takes a buffer of its own; the pool stands.
        }
    }
    if (!c) {
        std::unique_ptr<SkinPoolChunk> n(new SkinPoolChunk());
        if (!skin_pool_buffer(dev, 1u, bytes, n->vb, n->srv)) {
            // SKIN_POOL_MADE: no pool buffer ever stood on this device - it refuses the kind itself: no more
            // pooling. After one stood the refusal is this take's alone (the binding takes a buffer of its own).
            if (!g_skin_pool_made) g_skin_pool_failed = true;
            if (p.chunks.empty()) g_skin_pool.erase(key);
            return false;
        }
        if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
        n->serial = g_skin_pool_serial;
        n->key = key;
        n->used.assign(1u, 0u);
        c = n.get();
        s = 0u;
        p.chunks.push_back(std::move(n));
    }
    c->used[s] = 1u;
    ++c->live;
    g.vb = c->vb;
    g.chunk = c;
    g.slice = s;
    g.off = s * bytes;
    return true;
}
// SKIN_POOL: a pool about to take need more slices in this flush takes the room at once - its last chunk
// grown to fit them, at least doubled (to the cap), then whole chunks and one sized to the rest - so a mass spawn
// makes each buffer once and copies the old last chunk at most once, not a doubling ladder, and a trickle still
// grows by doubling. The takes then fill first-free. A refusal leaves the takes to grow as they go; a chunk nothing
// took goes at the next trim. skin_upload's, before its walk, under g_skin_mu.
inline void skin_pool_reserve(ID3D11Device* dev, ID3D11DeviceContext* ctx, uint64_t key,
                              uint32_t need) {
    const UINT bytes = static_cast<UINT>(key & 0xFFFFFFFFull);
    if (g_skin_pool_failed || !dev || !ctx || need == 0u || bytes == 0u ||
        bytes % static_cast<UINT>(sizeof(MeshVertex)) != 0u) return;
    const uint32_t pr_max = skin_pool_maxcap(bytes);
    const auto it = g_skin_pool.find(key);
    uint64_t pr_free = 0u;
    if (it != g_skin_pool.end()) {
        for (const std::unique_ptr<SkinPoolChunk>& c : it->second.chunks) {
            pr_free += c->used.size() - c->live;
        }
    }
    if (pr_free >= need) return;
    uint64_t want = need - pr_free;
    if (it != g_skin_pool.end() && !it->second.chunks.empty()) {   // The last chunk first, to the cap.
        SkinPoolChunk& l = *it->second.chunks.back();
        const uint64_t was = l.used.size();
        if (was < pr_max) {   // At least doubled (a trickle of spawns does not grow a slice a flush).
            const uint64_t to = std::min<uint64_t>(std::max<uint64_t>(was + want, was * 2u),
                                                   pr_max);
            if (!skin_pool_grow(dev, ctx, l, bytes, static_cast<uint32_t>(to))) return;
            want -= std::min<uint64_t>(want, to - was);
        }
    }
    while (want > 0u) {   // Then whole chunks, the last one sized to what is left.
        std::unique_ptr<SkinPoolChunk> n(new SkinPoolChunk());
        const uint32_t cap = static_cast<uint32_t>(std::min<uint64_t>(want, pr_max));
        if (!skin_pool_buffer(dev, cap, bytes, n->vb, n->srv)) return;   // The takes try.
        if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
        n->serial = g_skin_pool_serial;
        n->key = key;
        n->used.assign(cap, 0u);
        g_skin_pool[key].chunks.push_back(std::move(n));
        want -= cap;
    }
}
// The chunks with no live slice (a reservation nothing took) released; then every pool whose live slices fell to a
// quarter of its slices, repacked into half that: fresh chunks (as many full ones as needed, the last one twice its
// share), each binding's slice copied to its new place on the GPU and its fields re-pointed. Offsets change, so the
// bases go to the records with this flush's puts (skin_upload), and until a record carries its new base the
// instanced routes' tests send that binding per object (inst_pool_of, cast_pool_of). Every new buffer is made
// before anything moves: a refused one leaves the pool as it was. Called by skin_upload after its sweep, under
// g_skin_mu, with the slot tables empty (cloth_upload) - no element names a released chunk.
inline void skin_pool_trim(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    if (!dev || !ctx) return;
    for (auto pi = g_skin_pool.begin(); pi != g_skin_pool.end();) {   // Chunks with no live slice go.
        std::vector<std::unique_ptr<SkinPoolChunk>>& v = pi->second.chunks;
        for (size_t i = 0; i < v.size();) {
            if (v[i]->live != 0u) { ++i; continue; }
            KH_SAFE_RELEASE(v[i]->srv);
            KH_SAFE_RELEASE(v[i]->vb);
            v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));
        }
        if (v.empty()) pi = g_skin_pool.erase(pi);
        else ++pi;
    }
    for (auto& kv : g_skin_pool) {
        SkinPool& p = kv.second;
        uint64_t live = 0u, cap = 0u;
        for (const std::unique_ptr<SkinPoolChunk>& c : p.chunks) {
            live += c->live;
            cap += c->used.size();
        }
        if (live == 0u || cap <= 1u || live * 4u > cap) continue;
        if (cap == p.trim_refused && g_topo_cycles - p.trim_refused_at < SKIN_POOL_TRIM_RETRY) {
            continue;   // Refused at this size lately: wait for the pool to change, or a while.
        }
        const UINT bytes = static_cast<UINT>(kv.first & 0xFFFFFFFFull);
        const uint32_t tr_max = skin_pool_maxcap(bytes);
        std::vector<std::unique_ptr<SkinPoolChunk>> tr_new;
        bool ok = true;
        for (uint64_t left = live; left > 0u && ok;) {
            const uint32_t n = left >= tr_max ? tr_max
                             : static_cast<uint32_t>(std::min<uint64_t>(left * 2u, tr_max));
            std::unique_ptr<SkinPoolChunk> c(new SkinPoolChunk());
            ok = skin_pool_buffer(dev, n, bytes, c->vb, c->srv);
            if (!ok) break;
            c->key = kv.first;
            c->used.assign(n, 0u);
            left -= std::min<uint64_t>(left, n);
            tr_new.push_back(std::move(c));
        }
        if (!ok) {
            for (std::unique_ptr<SkinPoolChunk>& c : tr_new) {
                KH_SAFE_RELEASE(c->srv);
                KH_SAFE_RELEASE(c->vb);
            }
            p.trim_refused = cap;
            p.trim_refused_at = g_topo_cycles;
            continue;
        }
        size_t ci = 0;
        uint32_t si = 0u;
        for (auto& g : g_skin_gpu) {
            SkinGpu& m = g.second;
            if (!m.chunk || m.chunk->key != kv.first) continue;
            if (si >= tr_new[ci]->used.size()) { ++ci; si = 0u; }
            SkinPoolChunk& d = *tr_new[ci];
            D3D11_BOX b = {};
            b.left = m.off;
            b.right = m.off + bytes;
            b.top = 0;
            b.bottom = 1;
            b.front = 0;
            b.back = 1;
            ctx->CopySubresourceRegion(d.vb, 0, si * bytes, 0, 0, m.vb, 0, &b);
            d.used[si] = 1u;
            ++d.live;
            m.vb = d.vb;
            m.chunk = &d;
            m.slice = si;
            m.off = si * bytes;
            ++si;
        }
        for (std::unique_ptr<SkinPoolChunk>& c : p.chunks) {
            KH_SAFE_RELEASE(c->srv);
            KH_SAFE_RELEASE(c->vb);
        }
        for (std::unique_ptr<SkinPoolChunk>& c : tr_new) {
            if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
            c->serial = g_skin_pool_serial;
        }
        p.chunks.swap(tr_new);   // The old chunk objects go with tr_new.
    }
}
// The binding's buffer goes: its own released, or its slice handed back (and the chunk with its last slice).
inline void skin_gpu_vb_drop(SkinGpu& g) {
    SkinPoolChunk* const c = g.chunk;
    if (c) {
        if (g.slice < c->used.size() && c->used[g.slice]) {
            c->used[g.slice] = 0u;
            --c->live;
        }
        if (c->live == 0u) {
            const auto p = g_skin_pool.find(c->key);
            if (p != g_skin_pool.end()) {
                std::vector<std::unique_ptr<SkinPoolChunk>>& v = p->second.chunks;
                for (size_t i = 0; i < v.size(); ++i) {
                    if (v[i].get() != c) continue;
                    KH_SAFE_RELEASE(v[i]->srv);
                    KH_SAFE_RELEASE(v[i]->vb);
                    v.erase(v.begin() + static_cast<std::ptrdiff_t>(i));
                    break;
                }
                if (v.empty()) g_skin_pool.erase(p);
            }
        }
        g.vb = nullptr;   // The chunk's: not this binding's to release.
    } else {
        KH_SAFE_RELEASE(g.vb);
    }
    g.chunk = nullptr;
    g.slice = 0u;
    g.off = 0u;
}
// A CPU write of a stream-output buffer's whole vertex array (bytes): the buffer of the binding's own, or
// (pooled) its slice of a chunk, by a box.
inline void skin_so_write(ID3D11DeviceContext* ctx, const VbRef& r, UINT bytes, bool pooled,
                          const void* src) {
    if (!pooled) {
        ctx->UpdateSubresource(r.vb, 0, nullptr, src, 0, 0);
        return;
    }
    D3D11_BOX b = {};
    b.left = r.off;
    b.right = r.off + bytes;
    b.top = 0;
    b.bottom = 1;
    b.front = 0;
    b.back = 1;
    ctx->UpdateSubresource(r.vb, 0, &b, src, 0, 0);
}
// SKIN_GPU_MOVE: the binding's DYNAMIC buffer handed over to stream output (skin_gpu_movable said it may) - a
// slice of its mesh's pool, else a DEFAULT buffer of its own with the stream-output bind, as the walk makes a first
// one. Its bytes are copied on the GPU and the old buffer released: every other field (gen, key, box, collider,
// valid, the step's claim) stands with the bytes, so the binding draws the pose it held and its next step skins it
// on the GPU. The engine's predicate is set aside for the whole move and put back - a predicated copy would leave
// the new buffer unwritten, or a chunk the take grows without its older slices. A refusal keeps the DYNAMIC buffer
// and stamps so_refused. True = moved. Under g_skin_mu, in skin_upload's walk.
inline bool skin_gpu_move(ID3D11Device* dev, ID3D11DeviceContext* ctx, int mesh, SkinGpu& g) {
    ID3D11Predicate* pr = nullptr;
    BOOL pv = FALSE;
    ctx->GetPredication(&pr, &pv);
    if (pr) ctx->SetPredication(nullptr, FALSE);
    SkinGpu n;   // The new buffer: vb / chunk / slice / off.
    bool ok = skin_pool_take(dev, ctx, mesh, g.bytes, n);
    if (!ok) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = g.bytes;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
        ok = SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &n.vb)) && n.vb;
        if (!ok) n.vb = nullptr;
    }
    if (ok) {
        D3D11_BOX b = {};
        b.left = 0;
        b.right = g.bytes;
        b.top = 0;
        b.bottom = 1;
        b.front = 0;
        b.back = 1;
        ctx->CopySubresourceRegion(n.vb, 0, n.off, 0, 0, g.vb, 0, &b);
        KH_SAFE_RELEASE(g.vb);   // The DYNAMIC buffer: the binding's own (never pooled).
        g.vb = n.vb;
        g.chunk = n.chunk;
        g.slice = n.slice;
        g.off = n.off;
        g.so = true;
        g.so_refused = ~0ull;
    } else {
        g.so_refused = g_topo_cycles;
    }
    if (pr) {
        ctx->SetPredication(pr, pv);
        pr->Release();
    }
    return ok;
}
// SKIN_SO_PAL: a weight's bone is stored as its palette row (SkinSup::pal - every bone the skip below lets
// through has one), so the shader reads row j where skin_verts reads bone pal[j]: the same map, the same sums.
inline void skin_so_stream(const SkinRest& ss_r, size_t nb, std::vector<SkinSoVert>& out) {
    const size_t nv = ss_r.verts.size();
    std::vector<uint32_t> row(nb, 0u);   // SKIN_SO_PAL: bone -> palette row.
    for (size_t j = 0; j < ss_r.sup.pal.size(); ++j) {
        if (ss_r.sup.pal[j] < nb) row[ss_r.sup.pal[j]] = static_cast<uint32_t>(j);
    }
    out.resize(nv);
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& r = ss_r.verts[i];
        SkinSoVert& o = out[i];
        for (int k = 0; k < 3; ++k) {   // skin_verts' first three lines, to the float.
            o.p[k] = r.pos[k] * ss_r.ext[k] + ss_r.ctr[k];
            o.n[k] = r.nrm[k] / ss_r.ext[k];
            o.t[k] = r.tan[k] * ss_r.ext[k];
        }
        o.t[3] = r.tan[3];
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
        const SkinInf& f = ss_r.inf[i];   // The caller checked inf.size() == nv.
        for (int k = 0; k < 4; ++k) {
            const bool on = f.w[k] > 0.0f && f.b[k] < nb;   // skin_verts' skip, made a zero weight.
            o.b[k] = on ? row[f.b[k]] : 0u;   // SKIN_SO_PAL.
            o.w[k] = on ? f.w[k] : 0.0f;
        }
    }
}
// CBSkin's image (skin.hlsl): the box's centre and size, then three float4 a bone - row c = (M[0 + c], M[3 + c],
// M[6 + c], M[9 + c]) of skin_affine_of's map, so the shader's dot-and-add is skin_verts' sum. dst holds
// (2 + np * 3) * 4 floats. SKIN_SO_PAL: row j is bone pal[j]'s map (np rows; every entry is below
// the snapshot's bone count, so inside a).
inline void skin_so_cb_fill(float* dst, const float bc[3], const float bs[3], const float* a,
                            const uint32_t* pal, size_t np) {
    dst[0] = bc[0]; dst[1] = bc[1]; dst[2] = bc[2]; dst[3] = 0.0f;
    dst[4] = bs[0]; dst[5] = bs[1]; dst[6] = bs[2]; dst[7] = 0.0f;
    float* o = dst + 8;
    for (size_t j = 0; j < np; ++j) {
        const float* m = a + static_cast<size_t>(pal[j]) * 12u;
        for (int c = 0; c < 3; ++c, o += 4) {
            o[0] = m[c]; o[1] = m[3 + c]; o[2] = m[6 + c]; o[3] = m[9 + c];
        }
    }
}
// A snapshot's rest stream, made once (g_skin_mu held; marks it seen for the upload's sweep). Null = this snapshot
// cannot be skinned on the GPU, or its create failed - an entry stands for a failure, so it is not retried on every
// pass: SKIN_GPU_MOVE asks again SKIN_GPU_RETRY cycles on (fail_at); the entry leaves with the snapshot's last
// binding.
inline ID3D11Buffer* skin_rest_gpu_ensure(ID3D11Device* dev, const std::shared_ptr<const SkinRest>& rest, size_t nb) {
    if (!dev || !rest || !rest->sup.ok || nb == 0u ||
        !skin_so_class_ok(*rest) ||   // SKIN_SO_BIG / SKIN_SO_LAZY: the palette's class stands.
        rest->sup.root.size() != nb || rest->verts.empty() || rest->inf.size() != rest->verts.size()) return nullptr;
    const auto it = g_skin_rest_gpu.find(rest.get());
    if (it != g_skin_rest_gpu.end()) {
        it->second.seen = true;
        if (it->second.vb || !skin_gpu_retry_open(it->second.fail_at)) return it->second.vb;
    }
    SkinRestGpu& rg = g_skin_rest_gpu[rest.get()];
    rg.seen = true;
    rg.rest = rest;
    try {
        std::vector<SkinSoVert> sv;
        skin_so_stream(*rest, nb, sv);
        D3D11_BUFFER_DESC rd = {};
        rd.ByteWidth = static_cast<UINT>(sv.size() * sizeof(SkinSoVert));
        rd.Usage = D3D11_USAGE_IMMUTABLE;
        rd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ri = {};
        ri.pSysMem = sv.data();
        if (FAILED(dev->CreateBuffer(&rd, &ri, &rg.vb))) rg.vb = nullptr;
    } catch (...) { rg.vb = nullptr; }
    rg.nv = rg.vb ? static_cast<UINT>(rest->verts.size()) : 0u;
    rg.fail_at = rg.vb ? ~0ull : g_topo_cycles;   // SKIN_GPU_MOVE.
    if (!rg.vb) stat_inc(g_stats.skin_gpu_refused);   // SKIN_GPU_MOVE: skinGpuRefused.
    return rg.vb;
}
// SKIN_GPU_MOVE: a binding's CPU-skinned (DYNAMIC) buffer that stream output can take over - its mesh's snapshot
// skins on the GPU (the bound's trees, a bone at least and a palette of at most KH_SKIN_SO_BONES (SKIN_SO_BIG), an
// influence entry for every vertex: the walk's first-install test, item by item), the buffer is the rest stream's
// size, and no refusal of the device is younger than SKIN_GPU_RETRY cycles. The rest stream itself is the
// caller's to have. Under g_skin_mu.
inline bool skin_gpu_movable(const SkinGpu& g, const SkinRest& r, size_t nb) {
    return g.vb && !g.so && !g.chunk && r.sup.ok && nb != 0u &&
           skin_so_class_ok(r) &&   // SKIN_SO_BIG / _LAZY: a large palette moves once its class stands.
           r.sup.root.size() == nb && r.inf.size() == r.verts.size() && !r.verts.empty() &&
           static_cast<uint64_t>(g.bytes) == static_cast<uint64_t>(r.verts.size()) * sizeof(MeshVertex) &&
           skin_gpu_retry_open(g.so_refused);
}
// EA_SKIN_SO_DECL: the stream-output declaration of both classes (skin_so_ensure, skin_so_big_ensure):
// MeshVertex's four lanes in its order, 48 bytes (skin.hlsl's VSOutSkin); nothing is rasterised.
static const D3D11_SO_DECLARATION_ENTRY SKIN_SO_DECL[] = {
    { 0, "POSITION", 0, 0, 3, 0 },
    { 0, "NORMAL",   0, 0, 3, 0 },
    { 0, "TEXCOORD", 0, 0, 2, 0 },
    { 0, "TANGENT",  0, 0, 4, 0 },
};
// The device objects, once. False = not available: no device, or not now (reported once; SKIN_SO_RETRY: a
// memory refusal is asked again SKIN_GPU_RETRY cycles on, any other failure stands until the device or the
// session resets). From skin_upload, holding no lock: VSSkinSo is taken here from the arming prewarm
// (PREWARM_ALL) - compile_shader waits for its worker, or compiles it, only if the batch has not reached it.
inline bool skin_so_ensure(ID3D11Device* dev) {
    if (!SKIN_GPU_ON || !dev || !skin_so_askable()) return false;
    if (g_res.skin_so_vs && g_res.skin_so_gs && g_res.skin_so_layout && g_res.skin_so_cb) return true;
    // SKIN_SO_RETRY: retry = the device refused memory (asked again, and counted as skinGpuRefused).
    auto fail = [](const std::string& why, bool retry) {
        KH_SAFE_RELEASE(g_res.skin_so_cb);
        KH_SAFE_RELEASE(g_res.skin_so_layout);
        KH_SAFE_RELEASE(g_res.skin_so_gs);
        KH_SAFE_RELEASE(g_res.skin_so_vs);
        g_skin_so_failed.store(true, std::memory_order_relaxed);
        g_skin_so_retry_at = retry ? g_topo_cycles : ~0ull;
        if (retry) stat_inc(g_stats.skin_gpu_refused);   // skinGpuRefused.
        ensure_ok("GPU skinning (the CPU path stands)", why);
        return false;
    };
    static const D3D_SHADER_MACRO defs[] = { { nullptr, nullptr } };
    ID3DBlob* vb = nullptr;
    const std::string err = compile_shader(hlsl_src(HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", defs, &vb);
    if (!err.empty() || !vb) {
        if (vb) vb->Release();
        return fail(err.empty() ? std::string("VSSkinSo: no bytecode") : err, false);
    }
    static const D3D11_INPUT_ELEMENT_DESC il[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    const UINT stride = static_cast<UINT>(sizeof(MeshVertex));
    HRESULT hr = dev->CreateVertexShader(vb->GetBufferPointer(), vb->GetBufferSize(), nullptr, &g_res.skin_so_vs);
    if (SUCCEEDED(hr)) {
        hr = dev->CreateGeometryShaderWithStreamOutput(vb->GetBufferPointer(), vb->GetBufferSize(),
                                                       SKIN_SO_DECL, 4u, &stride, 1u,
                                                       D3D11_SO_NO_RASTERIZED_STREAM, nullptr,
                                                       &g_res.skin_so_gs);
    }
    if (SUCCEEDED(hr)) {
        hr = dev->CreateInputLayout(il, 6u, vb->GetBufferPointer(), vb->GetBufferSize(), &g_res.skin_so_layout);
    }
    vb->Release();
    if (SUCCEEDED(hr)) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = (2u + SKIN_SO_BONES_S * 3u) * 16u;   // CBSkin, the small class (SKIN_SO_LAZY).
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.skin_so_cb);
    }
    if (FAILED(hr)) {
        return fail(std::string("stream-output objects: ") + hr_str(hr), hr == E_OUTOFMEMORY);
    }
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // SKIN_SO_RETRY: made (on a retry too).
    g_skin_so_retry_at = ~0ull;
    return true;
}
inline bool skin_so_big_ensure(ID3D11Device* dev) {
    if (!SKIN_GPU_ON || !dev) return false;
    if (skin_so_big_ready()) return true;
    if (!g_res.skin_so_layout || !skin_so_big_askable()) return false;   // The small class's layout serves both.
    auto fail = [](const std::string& why, bool retry) {
        KH_SAFE_RELEASE(g_res.skin_so_cb_l);
        KH_SAFE_RELEASE(g_res.skin_so_gs_l);
        KH_SAFE_RELEASE(g_res.skin_so_vs_l);
        g_skin_so_big_failed = true;
        g_skin_so_big_retry_at = retry ? g_topo_cycles : ~0ull;
        if (retry) stat_inc(g_stats.skin_gpu_refused);   // skinGpuRefused.
        ensure_ok("GPU skinning of skeletons over 256 weighted bones (they stay on the CPU path)", why);
        return false;
    };
    ID3DBlob* vb = nullptr;
    const std::string err = compile_shader(hlsl_src(HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0",
                                           SKIN_SO_DEFS_L, &vb);
    if (!err.empty() || !vb) {
        if (vb) vb->Release();
        return fail(err.empty() ? std::string("VSSkinSo (large): no bytecode") : err, false);
    }
    const UINT stride = static_cast<UINT>(sizeof(MeshVertex));
    HRESULT hr = dev->CreateVertexShader(vb->GetBufferPointer(), vb->GetBufferSize(), nullptr,
                                         &g_res.skin_so_vs_l);
    if (SUCCEEDED(hr)) {
        hr = dev->CreateGeometryShaderWithStreamOutput(
            vb->GetBufferPointer(), vb->GetBufferSize(), SKIN_SO_DECL, 4u, &stride, 1u,
            D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &g_res.skin_so_gs_l);
    }
    vb->Release();
    if (SUCCEEDED(hr)) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = (2u + KH_SKIN_SO_BONES * 3u) * 16u;   // CBSkin, the large class.
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.skin_so_cb_l);
    }
    if (FAILED(hr)) {
        return fail(std::string("stream-output objects (large): ") + hr_str(hr), hr == E_OUTOFMEMORY);
    }
    g_skin_so_big_failed = false;
    g_skin_so_big_retry_at = ~0ull;
    return true;
}

// Which proxy moves each bone, for this mesh and this set of memory points. Game thread.
inline void skin_drive(SkinInst& in, const MeshDef& d, const std::vector<std::string>& mem) {
    const size_t nb = d.skin_bones.size();
    in.drive.assign(nb, -1);
    in.drive_px.assign(nb, -1);
    in.bones_driven = 0;
    std::unordered_map<std::string, int32_t> px;
    for (size_t j = 0; j < mem.size(); ++j) px.emplace(mem[j], static_cast<int32_t>(j));
    std::vector<int32_t> own(nb, -1);
    for (size_t b = 0; b < nb; ++b) {
        const auto it = px.find(d.skin_bones[b].name);
        if (it == px.end()) continue;
        own[b] = it->second;
        ++in.bones_driven;
    }
    for (size_t b = 0; b < nb; ++b) {
        // Up the chain to the nearest matched bone; bounded by the bone count so a malformed parent table cannot loop.
        int32_t c = static_cast<int32_t>(b);
        for (size_t s = 0; s <= nb && c >= 0 && static_cast<size_t>(c) < nb; ++s) {
            if (own[c] >= 0) {
                in.drive[b] = c;
                in.drive_px[b] = own[c];
                break;
            }
            c = d.skin_bones[c].parent;
        }
    }
}

// A proxy's transform against a parent's: the relative pose, false when the proxy is out of reach of it. Positions
// SQF order, bases rows aside / up / dir in engine axes, as the raw read hands them. xd: the proxy's
// PXY_SMOOTH delta (SQF order), or null. The offset is taken in double and rounded once: with no delta that is
// the float difference exactly (IEEE subtraction is correctly rounded), and a delta finer than the world
// position's float step survives into the small offset.
inline bool skin_rel(const float xp[3], const float xr[9], const float pp[3], const float rp[9],
                     SkinXf& x, const float* xd) {
    // Engine axes: SQF [x, y, zASL] -> [x, zASL, y].
    double w[3] = { static_cast<double>(xp[0]) - static_cast<double>(pp[0]),
                    static_cast<double>(xp[2]) - static_cast<double>(pp[2]),
                    static_cast<double>(xp[1]) - static_cast<double>(pp[1]) };
    if (xd) {
        w[0] += static_cast<double>(xd[0]);
        w[1] += static_cast<double>(xd[2]);
        w[2] += static_cast<double>(xd[1]);
    }
    const float d[3] = { static_cast<float>(w[0]), static_cast<float>(w[1]),
                         static_cast<float>(w[2]) };
    if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > SKEL_REACH_M * SKEL_REACH_M) return false;
    for (int r = 0; r < 3; ++r) {
        // Row vectors: local = world * R^T, so each lane is a dot with one of the parent's rows.
        x.t[r] = d[0] * rp[r * 3 + 0] + d[1] * rp[r * 3 + 1] + d[2] * rp[r * 3 + 2];
        for (int c = 0; c < 3; ++c) {
            x.q[r * 3 + c] = xr[r * 3 + 0] * rp[c * 3 + 0] + xr[r * 3 + 1] * rp[c * 3 + 1] +
                             xr[r * 3 + 2] * rp[c * 3 + 2];
        }
    }
    return true;
}
// The affine maps from an explicit pose set (the instance's drive tables).
inline void skin_affine_of(const SkinInst& in, const MeshDef& d, const std::vector<SkinXf>& pose,
                           const std::vector<uint8_t>& ok, std::vector<float>& a) {
    const size_t nb = d.skin_bones.size();
    a.assign(nb * 12u, 0.0f);
    for (size_t b = 0; b < nb; ++b) {
        float* m = &a[b * 12u];
        const int32_t px = b < in.drive_px.size() ? in.drive_px[b] : -1;
        if (px < 0 || static_cast<size_t>(px) >= pose.size() || !ok[px]) {
            m[0] = m[4] = m[8] = 1.0f;   // The root: the identity.
            continue;
        }
        const SkinXf& x = pose[px];
        const float* h = d.skin_bones[in.drive[b]].head;
        memcpy(m, x.q, sizeof(x.q));
        for (int c = 0; c < 3; ++c) {
            m[9 + c] = x.t[c] - (h[0] * x.q[0 * 3 + c] + h[1] * x.q[1 * 3 + c] +
                                 h[2] * x.q[2 * 3 + c]);
        }
    }
}
// One affine map per bone from the current pose - the job's input (in_a), a few floats per bone; also the cloth's
// guide. Game thread; into in_a only on an instance that is not busy (the guide is written busy or not - no worker
// reads it).
inline void skin_affine(const SkinInst& in, const MeshDef& d, std::vector<float>& a) {
    skin_affine_of(in, d, in.pose, in.pose_ok, a);
}

// The vertices of one pose, from a rest snapshot and the per-bone affine maps: out (positions stored in the pose's
// own box, which bc / bs return) and met (the metric positions, the collider's input). No instance state: the worker
// (skin_run) and the consume-point step (skin_draw_verts) both call it.
// SKIN_GPU: full false = the metric positions and the box alone (a collider's pose while the GPU draws the
// mesh): out is sized to the vertex count, as for a full pose, but not written.
inline void skin_verts(const SkinRest& d, const float* a, size_t nb, std::vector<MeshVertex>& out,
                       std::vector<float>& met, float bc[3], float bs[3], bool full = true) {
    const size_t nv = d.verts.size();
    const bool inf = d.inf.size() == nv;
    const uint8_t* drawn = d.drawn.size() == nv ? d.drawn.data() : nullptr;   // LOD_SKIN.
    out.resize(nv);
    met.resize(nv * 3u);
    const float* e = d.ext;
    const float* c0 = d.ctr;
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& r = d.verts[i];
        MeshVertex& o = out[i];
        float p[3], n[3], t[3];
        for (int k = 0; k < 3; ++k) {
            p[k] = r.pos[k] * e[k] + c0[k];   // Authored, metres.
            n[k] = full ? r.nrm[k] / e[k] : 0.0f;   // The authored normal (IMPORT_NRM; NORMALS above).
            t[k] = full ? r.tan[k] * e[k] : 0.0f;   // Covariant: KhVsCore's tan * size.
        }
        float pp[3] = { 0.0f, 0.0f, 0.0f }, nn[3] = { 0.0f, 0.0f, 0.0f }, tt[3] = { 0.0f, 0.0f, 0.0f };
        float ws = 0.0f;
        float br[3][3] = {};   // SKIN_NRM_COF: the blended map's rows.
        if (inf) {
            const SkinInf& f = d.inf[i];
            for (int sr_k = 0; sr_k < 4; ++sr_k) {
                const float w = f.w[sr_k];
                if (!(w > 0.0f) || f.b[sr_k] >= nb) continue;
                const float* m = a + static_cast<size_t>(f.b[sr_k]) * 12u;
                for (int c = 0; c < 3; ++c) {
                    pp[c] += w * (p[0] * m[c] + p[1] * m[3 + c] + p[2] * m[6 + c] + m[9 + c]);
                    if (!full) continue;   // SKIN_GPU: positions alone.
                    tt[c] += w * (t[0] * m[c] + t[1] * m[3 + c] + t[2] * m[6 + c]);
                    br[c][0] += w * m[c];   // SKIN_NRM_COF: row c of the blend (khsk_b0 - b2).
                    br[c][1] += w * m[3 + c];
                    br[c][2] += w * m[6 + c];
                }
                ws += w;
            }
        }
        const float rest = ws < 1.0f ? 1.0f - ws : 0.0f;   // The weight left to the root.
        const bool in_box = drawn == nullptr || drawn[i] != 0u;
        for (int k = 0; k < 3; ++k) {
            pp[k] += rest * p[k];
            tt[k] += rest * t[k];
            if (!in_box) continue;
            if (pp[k] < mn[k]) mn[k] = pp[k];
            if (pp[k] > mx[k]) mx[k] = pp[k];
        }
        memcpy(&met[i * 3u], pp, sizeof(pp));
        if (!full) continue;   // SKIN_GPU: out is not written.
        for (int k = 0; k < 3; ++k) br[k][k] += rest;   // SKIN_NRM_COF: the root's share, the identity.
        skin_nrm_cof(br, n, nn);   // SKIN_NRM_COF: the normal by the blend's inverse transpose.
        memcpy(o.nrm, nn, sizeof(nn));   // Metric for now; the box pass below scales them.
        memcpy(o.tan, tt, sizeof(tt));
        o.tan[3] = r.tan[3];   // The chart's handedness does not deform.
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
    }
    for (int k = 0; k < 3; ++k) {
        if (!(mn[k] <= mx[k])) { mn[k] = mx[k] = 0.0f; }   // Nothing drawn.
        bc[k] = 0.5f * (mn[k] + mx[k]);
        const float s = mx[k] - mn[k];
        bs[k] = s > 1.0e-4f ? s : 1.0e-4f;   // A flat axis, as native_size.
    }
    // Stored in the pose's own box: KhVsCore scales a position by size, divides a normal by it and multiplies a
    // tangent by it.
    if (!full) return;   // SKIN_GPU.
    for (size_t i = 0; i < nv; ++i) {
        MeshVertex& o = out[i];
        const float* p = &met[i * 3u];
        for (int k = 0; k < 3; ++k) {
            o.pos[k] = (p[k] - bc[k]) / bs[k];
            o.nrm[k] *= bs[k];
            o.tan[k] /= bs[k];
        }
    }
}
// The skinning: ON A WORKER, from in_rest and in_a only, into the back buffer. Writes that buffer's box and shape
// key (a new key when some vertex has left the shape that took the current one by more than CLOTH_SHAPE_M, the
// cloth's rule), then publishes.
inline void skin_run(SkinInst& in) {
    KH_PROF_SCOPE(PROF_W_SKIN_JOB);   // PROF.
    const SkinRest& d = *in.in_rest;
    const uint32_t back = 1u - (in.front.load(std::memory_order_relaxed) & 1u);
    std::vector<MeshVertex>& out = in.out[back];
    const size_t nv = d.verts.size();
    const float* e = d.ext;    // The collider's rest pose below scales the authored positions as the vertices did.
    const float* c0 = d.ctr;
    float* bc = in.box_ctr[back];
    float* bs = in.box_size[back];
    // SKIN_GPU: the vertices for a buffer the upload will write, the positions for a collider, neither for a job
    // that only records a collider switched off, nor for one of a binding hidden by SKIN_1X_HIDDEN alone (the box
    // and key are then the last result's).
    const bool pos = in.in_verts || in.in_collide;
    if (in.in_verts && stats_on()) g_skin_pool_full.fetch_add(1u, std::memory_order_relaxed);   // skinPoolFull.
    if (pos) {
        skin_verts(d, in.in_a.data(), in.in_a.size() / 12u, out, in.met, bc, bs, in.in_verts);
    } else {
        out.resize(nv);   // The upload sizes the buffer from it.
        memcpy(bc, in.box_ctr[1u - back], sizeof(float) * 3u);
        memcpy(bs, in.box_size[1u - back], sizeof(float) * 3u);
    }
    in.out_ok[back] = in.in_verts;
    if (pos && stats_on() && d.sup.ok && d.sup.root.size() * 12u == in.in_a.size()) {   // skinBoundWorst.
        float gc[3], gs[3];
        skin_so_bound(d, in.in_a.data(), gc, gs);
        float ratio = 0.0f;
        for (int k = 0; k < 3; ++k) {   // The pad off first; an axis the exact box barely has is not a ratio.
            if (!(bs[k] >= 0.05f)) continue;
            const float q = (gs[k] - 2.0f * SKIN_SO_PAD_M) / bs[k];
            if (q > ratio) ratio = q;
        }
        uint32_t rb = 0u;
        memcpy(&rb, &ratio, sizeof(rb));
        uint32_t cur = g_skin_bound_worst_bits.load(std::memory_order_relaxed);
        while (ratio == ratio && rb > cur &&
               !g_skin_bound_worst_bits.compare_exchange_weak(cur, rb, std::memory_order_relaxed)) {}
    }
    // COLLIDER: this pose's BVH, over level 0's triangles, in metres about the box centre (the stored positions
    // times the box: the space a view with bake = size queries, physics_gather_colliders). The topology never
    // changes between poses, so the rest pose's BVH is built once and each pose is a copy of it refitted
    // (physics_bvh_refit, a tenth of a build), with a full build only for a pose the refit refuses.
    if (in.in_collide && d.idx0.size() >= 3u) {
        const uint32_t ni = static_cast<uint32_t>(d.idx0.size());
        if (!in.col_tpl) {
            std::vector<float> rp(nv * 3u);   // The rest pose, authored metres.
            for (size_t i = 0; i < nv; ++i) {
                for (int k = 0; k < 3; ++k) rp[i * 3u + k] = d.verts[i].pos[k] * e[k] + c0[k];
            }
            std::shared_ptr<PhysicsBvh> tb = std::make_shared<PhysicsBvh>();
            physics_bvh_build(*tb, rp.data(), static_cast<uint32_t>(nv), d.idx0.data(), ni,
                               &in.col_topo);
            in.col_tpl = tb;
        }
        std::vector<float> cp(nv * 3u);
        for (size_t i = 0; i < nv; ++i) {
            for (int k = 0; k < 3; ++k) cp[i * 3u + k] = in.met[i * 3u + k] - bc[k];
        }
        // The motion since the last collider pose, in the same space (about THIS box's centre: the difference is
        // the limbs' own, root-relative motion), unless the game thread cut the chain (in_cut). A pose that is not
        // finite carries none either.
        std::vector<float> pp;
        if (!in.in_cut && in.col_prev.size() == in.met.size()) {
            bool bad = false;
            for (size_t i = 0; i < nv && !bad; ++i) {
                if (!cloth_finite3(&in.met[i * 3u]) || !cloth_finite3(&in.col_prev[i * 3u])) bad = true;
            }
            if (!bad) {
                pp.resize(nv * 3u);
                for (size_t i = 0; i < nv; ++i) {
                    for (int k = 0; k < 3; ++k) pp[i * 3u + k] = in.col_prev[i * 3u + k] - bc[k];
                }
            }
        }
        in.col_prev = in.met;
        // COL_CLEAR: col_prev names THIS pose from here on, so the identity of the pose vd swept from is taken
        // now and col_pose_seq stands at none until this pose's BVH exists: a job that throws below (an allocation)
        // leaves the next pose a cut chain, never a link to a pose its vd was not measured from.
        const uint32_t from = in.col_pose_seq;
        in.col_pose_seq = 0u;
        std::shared_ptr<PhysicsBvh> bv = std::make_shared<PhysicsBvh>(*in.col_tpl);
        // COL_CONVEX: a convex collider's pose is its planes: the refit leaves the nodes below the root and the
        // pseudonormals alone.
        if (!physics_bvh_refit(*bv, in.col_topo, cp.data(), static_cast<uint32_t>(nv), in.col_scr,
                                pp.empty() ? nullptr : pp.data(), in.in_convex)) {
            physics_bvh_build(*bv, cp.data(), static_cast<uint32_t>(nv), d.idx0.data(), ni);
        }
        // COL_CLEAR: vd is this pose less the LAST collider job's (col_prev), so that job's pose is the one this
        // sweeps from; none without vd.
        bv->prev_seq = bv->vd.empty() ? 0u : from;
        in.col_pose_seq = bv->pose_seq;
        in.bvh_of[back] = bv;
    } else {
        in.bvh_of[back].reset();
        in.col_prev.clear();   // Switched off: the next collider pose starts still.
        in.col_pose_seq = 0u;   // COL_CLEAR.
    }
    bool moved = pos && (in.shape_key == 0u || in.shape_ref.size() != in.met.size());   // SKIN_GPU: no positions, no test - the key stands.
    for (size_t i = 0; pos && i < in.met.size() && !moved; ++i) {
        if (!(fabsf(in.met[i] - in.shape_ref[i]) <= CLOTH_SHAPE_M)) moved = true;
    }
    if (moved) {
        in.shape_ref = in.met;
        uint32_t sr_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (sr_k == 0u) sr_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        in.shape_key = sr_k;
    }
    in.key_of[back] = in.shape_key;
    uint32_t g = g_skin_gen_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    if (g == 0u) g = g_skin_gen_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    in.gen_of[back] = g;   // SKIN_GEN_OF: before front's release store, as the box and key.
    in.front.store(back, std::memory_order_release);
    in.gen.store(g, std::memory_order_release);
}

// The pool's entry point (declared ahead of cloth_worker). busy is cleared on every path out, after the publish:
// the game thread may write this instance's inputs the instant it turns false.
inline void skin_job(const std::shared_ptr<SkinInst>& in) {
    try { skin_run(*in); } catch (...) {}
    in->busy.store(false, std::memory_order_release);
}

// The rest snapshot for a mesh, made on first use. Game thread.
inline std::shared_ptr<const SkinRest> skin_rest_of(int mesh) {
    const auto it = g_skin_rest.find(mesh);
    if (it != g_skin_rest.end()) return it->second;
    const MeshDef& d = mesh_def(mesh);
    std::shared_ptr<SkinRest> r = std::make_shared<SkinRest>();
    r->verts = d.verts;
    if (d.skin_inf.size() == d.verts.size()) r->inf = d.skin_inf;
    const uint32_t n0 = mesh_base_icount(d);
    if (n0 <= d.indices.size()) {
        r->idx0.assign(d.indices.begin(), d.indices.begin() + n0);
    }
    // LOD_SKIN: the box set, every vertex some level draws (all of them, as a rule: empty then).
    if (d.lod_n != 0) {
        r->drawn.assign(d.verts.size(), 0u);
        for (const uint32_t v : d.indices) {
            if (v < r->drawn.size()) r->drawn[v] = 1u;
        }
        if (std::find(r->drawn.begin(), r->drawn.end(), static_cast<uint8_t>(0u)) == r->drawn.end()) {
            r->drawn.clear();
        }
    }
    memcpy(r->ctr, d.native_ctr, sizeof(r->ctr));
    memcpy(r->ext, d.native_ext, sizeof(r->ext));
    // SKIN_GPU: the bound's trees. A failure (an allocation) leaves sup.ok false and this mesh on the CPU path.
    try { skin_sup_prepare(*r, d.skin_bones.size()); } catch (...) { r->sup = SkinSup{}; r->bone_box.clear(); }
    g_skin_rest.emplace(mesh, r);
    return r;
}

// SKEL_DRAW - the consume-point skinning (declared with skin_pose_drawn). From a render-thread drawer's
// attach_step (usually the cast fire's, else the injection's when the skin waited for it, or the render-thread
// flush's when no injection ran), under g_draw_list_mutex, with the stash just taken: the relative poses and the
// per-bone maps through the instance's drive tables (render thread), the vertices from the rest snapshot (any pool
// thread, SKIN_FORK; SKIN_GPU: or skinned on the GPU by skin_so_batch), and the result written into the
// buffer the upload installed for this handle (g_skin_gpu; skin_upload, in the flush, keeps creating and
// installing it, and writes the worker's result into it only when no step wrote it within SKIN_STEP_OWN cycles,
// SkinGpu::step_cycle). The box the pose was stored in goes to the object as the upload's put does, and the step's
// centre takes it from there.
// Nothing here touches the instance's job inputs or its published buffers: the worker path stays whole for the
// collider and the cloth guide.
inline bool skin_pose_read(const SkinInst& in) {   // Declared with skin_pose_drawn.
    return in.cloth || in.want_collide;
}
// The per-bone maps (skin_affine_of's layout) of the stash this cycle's step took, into a.draw_a. False
// when the stash, instance or drive tables do not fit. Shared by SKEL_DRAW and CLOTH_PIN_STEP so both draw one
// pose.
inline bool skin_step_maps(Attach& a, const SkinInst* in, const RenderObject& o) {
    if (!a.skel_smp_ok || a.skel_smp_gen != a.skel_gen) return false;
    if (in->seq != o.seq || in->mesh != o.mesh || !mesh_alive(in->mesh)) return false;
    const MeshDef& d = mesh_def(in->mesh);
    if (in->drive_px.size() != d.skin_bones.size() || in->drive.size() != d.skin_bones.size()) return false;
    // SKIN_DRIVE_GEN: the drive tables index the proxy list of the binding they were built for (skin_sync,
    // which rebuilds them only for a binding whose pose is drawn - skin_pose_drawn: visible, or a casterOnly one
    // a shadow map takes - or that is a cloth or a collider); a rebind's new list against the old tables drives bones
    // from the wrong proxies. Until the sync catches up, the last pose stands.
    if (in->drive_mesh != in->mesh || in->drive_gen != a.skel_gen) return false;
    const size_t np = a.skel_proxy.size();
    if (a.skel_smp.size() != np * 12u) return false;
    a.draw_pose.assign(np, SkinXf());
    a.draw_ok.assign(np, 0u);
    for (size_t j = 0; j < np; ++j) {
        const float* s = &a.skel_smp[j * 12u];
        const float* const xd = a.skel_smp_d.size() == np * 3u ? &a.skel_smp_d[j * 3u]
                                                                 : nullptr;   // PXY_SMOOTH.
        if (skin_rel(s, s + 3, a.skel_smp_par, a.skel_smp_par + 3, a.draw_pose[j],
                     xd)) {
            a.draw_ok[j] = 1u;
        }
    }
    skin_affine_of(*in, d, a.draw_pose, a.draw_ok, a.draw_a);
    return true;
}
// SKEL_SHAPE - the caching passes (sun ladder, point-light shadows, occlusion sets) hash a slot's shape key
// (g_cloth_gen_slot). The draw's bytes are the consume-point step's, so the step publishes a key per written pose by
// the cloth's rule (a new key when some vertex left the keyed shape by more than CLOTH_SHAPE_M, on metric
// positions), from the process-wide serial; otherwise a mesh posing in place keeps its old shadow.
inline uint32_t skin_draw_shape_note(Attach& a) {
    const std::vector<float>& m = a.draw_met;
    bool moved = a.draw_shape_key == 0u || a.draw_shape_ref.size() != m.size();
    for (size_t i = 0; i < m.size() && !moved; ++i) {
        if (!(fabsf(m[i] - a.draw_shape_ref[i]) <= CLOTH_SHAPE_M)) moved = true;
    }
    if (a.draw_shape_gpu) moved = true;   // SKIN_GPU: the reference is a palette, not these positions.
    if (moved) {
        a.draw_shape_gpu = false;
        a.draw_shape_ref = m;
        uint32_t k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (k == 0u) k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        a.draw_shape_key = k;
    }
    return a.draw_shape_key;
}
// SKIN_GPU: the same key from the palette, for a pose whose vertices the CPU never sees. A vertex moves by a
// convex combination of its bones' maps' changes at it, and a map's change over a bone's rest box is largest at a
// corner, so 'no bone moved any corner of its box by more than CLOTH_SHAPE_M on any axis' implies the vertex
// test above would have passed: this re-keys whenever that would, and sometimes when it would not (never a stale
// shadow; at most a re-render the vertex test would have skipped).
inline uint32_t skin_so_shape_note(Attach& sn_a, const SkinRest& sn_r) {
    const std::vector<float>& a = sn_a.draw_a;
    bool moved = sn_a.draw_shape_key == 0u || !sn_a.draw_shape_gpu || sn_a.draw_shape_ref.size() != a.size() ||
                 sn_r.bone_box.size() * 2u != a.size();
    for (size_t b = 0; !moved && b * 12u < a.size(); ++b) {
        const float* bx = &sn_r.bone_box[b * 6u];
        if (!(bx[0] <= bx[3])) continue;   // The bone carries no vertex.
        const float* m = &a[b * 12u];
        const float* r = &sn_a.draw_shape_ref[b * 12u];
        for (int c = 0; c < 3 && !moved; ++c) {
            // |centre . dM + dT| + half . |dM|: the largest change over the box.
            float d = m[9 + c] - r[9 + c], h = 0.0f;
            for (int j = 0; j < 3; ++j) {
                const float dm = m[j * 3 + c] - r[j * 3 + c];
                d += 0.5f * (bx[j] + bx[3 + j]) * dm;
                h += 0.5f * (bx[3 + j] - bx[j]) * fabsf(dm);
            }
            if (!(fabsf(d) + h <= CLOTH_SHAPE_M)) moved = true;
        }
    }
    if (moved) {
        sn_a.draw_shape_ref = a;
        sn_a.draw_shape_gpu = true;
        uint32_t k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (k == 0u) k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        sn_a.draw_shape_key = k;
    }
    return sn_a.draw_shape_key;
}
// SKIN_FORK - the consume-point skin in three parts, so attach_step can run the middle one (the vertices, all
// of the cost) for every binding of a step at once across the cloth pool. prep: the checks and the maps (draw_a);
// false = this binding does not skin this step. verts: the pose into draw_out / draw_met and the box; touches only
// this binding's scratch and reads its rest snapshot, both stable while the step holds g_draw_list_mutex
// (skin_sync writes skin and rest_draw only under it), and takes no lock. finish: the upload, the shape key and
// the box, on the render thread's context.
inline bool skin_draw_prep(Attach& a, const RenderObject& o, ID3D11DeviceContext* ctx) {
    SkinInst* in = a.skin.get();
    if (!ctx || !in || in->cloth || !in->rest_draw) return false;
    return skin_step_maps(a, in, o);
}
inline void skin_draw_verts(Attach& a, float bc[3], float bs[3]) {
    const SkinInst* const in = a.skin.get();
    skin_verts(*in->rest_draw, a.draw_a.data(), a.draw_a.size() / 12u, a.draw_out, a.draw_met,
               bc, bs);
}
// SKIN_GPU: so_vb = the buffer skin_so_batch already skinned this pose into (the bytes are there; bc / bs
// are its bound); null = the pool's vertices (draw_out), written here. SKIN_POOL: the buffer is the pair (a
// chunk's slice, or the binding's own at 0).
inline void skin_draw_finish(const std::string& h, Attach& a, RenderObject& o,
                             ID3D11DeviceContext* ctx, const float bc[3], const float bs[3],
                             bool& moved, const VbRef& so_vb) {
    VbRef vb = so_vb;
    if (!vb) {
        const UINT bytes = static_cast<UINT>(a.draw_out.size() * sizeof(MeshVertex));
        if (bytes == 0u) return;
        bool sd_default = false;   // SKIN_GPU: a stream-output buffer takes no Map.
        bool pooled = false;    // SKIN_POOL: a chunk's slice.
        {   // g_draw_list_mutex -> g_skin_mu: the upload takes g_skin_mu on its own, never inside the list mutex.
            std::lock_guard<std::mutex> g(g_skin_mu);
            const auto it = g_skin_gpu.find(h);
            if (it != g_skin_gpu.end() && it->second.vb && it->second.bytes == bytes) {
                vb = skin_gpu_ref(it->second);
                sd_default = it->second.so;
                pooled = it->second.chunk != nullptr;
            }
        }
        if (!vb) return;   // Not installed yet (the first upload after the binding creates it).
        if (sd_default) {
            // EA_SKIN_PRED: UpdateSubresource is predicated and a step runs in the engine's hook with no StateBackup
            // (PRED_ASIDE): a predicate the engine left bound would skip the pose while the stamps below claim it.
            PredAside pa;
            pa.begin(ctx);
            skin_so_write(ctx, vb, bytes, pooled, a.draw_out.data());   // Its slice alone.
            pa.end();
        } else {   // A DYNAMIC buffer is the binding's own (never pooled).
            D3D11_MAPPED_SUBRESOURCE m = {};
            if (FAILED(ctx->Map(vb.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) return;
            memcpy(m.pData, a.draw_out.data(), bytes);
            ctx->Unmap(vb.vb, 0);
        }
    }
    uvs_src_wrote(o.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again (uvs_step follows this step).
    // The key of the bytes just written: the palette's for a GPU pose (its snapshot stood under the batch's checks,
    // in this same step).
    const uint32_t key = (so_vb && a.skin && a.skin->rest_draw)
                       ? skin_so_shape_note(a, *a.skin->rest_draw) : skin_draw_shape_note(a);
    {   // The stamp, after the bytes landed (the same edge as above; the map cannot change under the render thread).
        std::lock_guard<std::mutex> g(g_skin_mu);
        const auto it = g_skin_gpu.find(h);
        if (it != g_skin_gpu.end() && skin_gpu_ref(it->second) == vb) {   // SKIN_POOL: the pair.
            // SKIN_RESTEP_KEY: bytes of a new shape (SKEL_SHAPE's key, against the key of the bytes the buffer
            // held - the step's or the worker's) move the mesh for the passes that cache on it (the scene record,
            // the sun map's reopen: SUN_RESTEP, CAST_REPICK) as a moved box does; otherwise a pose that changed
            // inside its box keeps the earlier step's self-shadow. The held key is g_skin_gpu's (step_key or key),
            // not the slot's, so a binding under a vertex stage (whose slot carries the stage's key) takes it too.
            const uint32_t held = it->second.by_step ? it->second.step_key : it->second.key;
            if (held != key) moved = true;
            it->second.step_cycle = g_topo_cycles;
            it->second.by_step = true;
            it->second.step_key = key;
            if (a.skin) a.skin->step_cycle_pub.store(g_topo_cycles, std::memory_order_relaxed);   // SKIN_GPU: the game thread's in_verts.
            // The slot draws these bytes now, so it carries their key for every later pass.
            if (o.slot < g_cloth_vb_slot.size() && o.slot < g_cloth_gen_slot.size() &&
                o.slot < g_cloth_voff_slot.size() && g_cloth_vb_slot[o.slot] == vb.vb &&
                g_cloth_voff_slot[o.slot] == vb.off) {   // SKIN_POOL: the pair.
                g_cloth_gen_slot[o.slot] = key;
            }
        }
    }
    // The pose's box, as skin_upload's put writes it: size in SQF order, the centre in engine axes; the step's
    // skel_centre_d (attach_skel_place) re-derives pos.
    const float sz[3] = { bs[0], bs[2], bs[1] };
    // sizeof(o.skel_ctr), never sizeof(bc): bc is an array PARAMETER and decays to a pointer (8
    // bytes, two floats), so sizeof(bc) would compare and copy only two of the centre's three lanes and the
    // forward lane would stop following the bone.
    if (memcmp(o.skel_ctr, bc, sizeof(o.skel_ctr)) != 0 || memcmp(o.size, sz, sizeof(sz)) != 0) {
        memcpy(o.skel_ctr, bc, sizeof(o.skel_ctr));
        memcpy(o.size, sz, sizeof(sz));
        moved = true;
    }
}
// skin_sync: GAME THREAD, from flush_frame before the park: builds every skeletal binding's poses from the
// consume-point stash (SKEL_SAMPLE: its proxies and their parent, read together at Draw3D), then hands every
// instance whose pose changed to the pool. A stale stash gives no new pose (the last stands); a proxy out of reach
// of its parent - one the attach has not yet moved off its spawn point too - holds its last pose (the rest pose
// before its first), as an attachment lane holds its transform.
inline void skin_sync() {
    KH_PROF_SCOPE(PROF_SKIN_SYNC);   // PROF.
    if (g_attach_n.load(std::memory_order_relaxed) == 0 && g_skin.empty()) return;
    // SKIN_REST_EARLY: a binding's missing rest snapshot (skin_rest_of: copies and the bound's per-bone trees,
    // tens of ms for a large mesh) is built here, holding no lock, as physics_bvh_for builds outside its own - in
    // the locked walk below the render thread would wait it out on g_draw_list_mutex / g_skin_mu. The list is taken
    // under g_draw_list_mutex with the walk's own filters up to its skin_rest_of, so the walk finds each one
    // cached; the snapshot is a function of the published mesh def alone (immutable; the mesh stays referenced, so no
    // collector takes it), so it is the one the walk would have built. g_skin_rest: game thread only.
    {
        std::vector<int> miss;
        {
            std::lock_guard<std::mutex> pg(g_draw_list_mutex);
            for (const auto& kv : g_attach) {
                if (!kv.second.skel) continue;
                const auto oit = g_draw_list.find(kv.first);
                if (oit == g_draw_list.end()) continue;
                const RenderObject& o = oit->second;
                if (!o.skel || o.fullscreen || o.slot == 0xFFFFFFFFu) continue;
                if (!mesh_alive(o.mesh)) continue;
                if (g_skin_rest.find(o.mesh) != g_skin_rest.end()) continue;
                if (std::find(miss.begin(), miss.end(), o.mesh) == miss.end()) {
                    miss.push_back(o.mesh);
                }
            }
        }
        for (const int m : miss) skin_rest_of(m);
    }
    std::vector<std::shared_ptr<SkinInst>> jobs;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        // SKIN_MAP: the inserts and the field writes the render-thread upload reads. No call below takes a lock;
        // SKIN_REST_EARLY built every rest snapshot it asks for.
        std::lock_guard<std::mutex> sm(g_skin_mu);
        for (auto& kv : g_skin) kv.second->seen = false;
        for (auto& kv : g_attach) {
            Attach& a = kv.second;
            if (!a.skel) continue;
            const auto oit = g_draw_list.find(kv.first);
            if (oit == g_draw_list.end()) continue;
            const RenderObject& o = oit->second;
            if (!o.skel || o.fullscreen || o.slot == 0xFFFFFFFFu) continue;
            if (!mesh_alive(o.mesh)) continue;
            // A simulating mesh is the cloth's to draw (CLOTH above); an unpainted one does not simulate, so the skin
            // keeps it. CHAIN: a simulating chain is drawn the same way, its bones guiding its start (and its end,
            // where it follows the skeleton). CHAIN_REFUSED_SKIN: a chain whose build was refused does not
            // simulate either (no instance, nothing draws it), so the skin keeps it from the frame after the refusal
            // is filed.
            const bool cloth = (cloth_obj_sim(o) && cloth_mesh_ok(o.mesh)) ||
                               (chain_obj_sim(o) && chain_mesh_ok(o.mesh) &&
                                !chain_refused(kv.first, o));
            std::shared_ptr<SkinInst>& sp = g_skin[kv.first];
            if (!sp || sp->seq != o.seq || sp->mesh != o.mesh) {
                // Another object or mesh: a fresh instance. One a worker still holds finishes into nothing and dies
                // with its job.
                sp = std::make_shared<SkinInst>();
                sp->seq = o.seq;
                sp->mesh = o.mesh;
            }
            SkinInst& in = *sp;
            in.seen = true;
            a.skin = sp;   // SKEL_DRAW: the render-thread drawers' steps skin through this.
            in.rest_draw = skin_rest_of(in.mesh);
            if (in.skel_gen != a.skel_gen || in.pose.size() != a.skel_proxy.size()) {
                // New proxies: their poses start over.
                in.cut = true;
                in.skel_gen = a.skel_gen;
                in.pose.assign(a.skel_proxy.size(), SkinXf());
                in.pose_ok.assign(a.skel_proxy.size(), 0u);
            }
            in.slot = o.slot;
            in.cloth = cloth;
            in.want_collide = physics_obj_collider(o);
            in.want_convex = o.physics_col_convex;   // COL_CONVEX.
            // SKIN_HIDDEN_COL; SKIN_CASTER. SKIN_1X_HIDDEN: at FSAA 1x no world mesh is drawn and no step
            // skins a buffer (fsaa_world_standdown), so a visible binding is hidden too - no full CPU skin of
            // bytes nothing reads (a collider still gets its positions, a first result its vertices).
            in.hidden.store(!skin_pose_drawn(o) || fsaa_world_standdown(), std::memory_order_relaxed);
            // A hidden mesh keeps its instance and its last result, so the buffer the upload installs and the box
            // the object carries stay one pair while hidden and when shown again; one not drawn stops asking for
            // new poses (one hidden by SKIN_1X_HIDDEN alone still asks - its job takes no vertices). A cloth
            // simulates hidden or not, so its guide does not wait; a hidden collider still collides, so its pose
            // does not wait either.
            if (!skin_pose_drawn(o) && !cloth && !in.want_collide) continue;   // SKIN_CASTER.
            if (in.drive_mesh != in.mesh || in.drive_gen != in.skel_gen) {
                skin_drive(in, mesh_def(in.mesh), a.skel_mem);
                in.drive_mesh = in.mesh;
                in.drive_gen = in.skel_gen;
                in.dirty = true;
                in.cut = true;
            }
            // SKEL_SAMPLE: the consume-point stash (attach_step: the cycle's Draw3D sample), taken whole or not
            // at all - the proxies and their parent are one pair. Not fresh: no new pose this sync; the last stands.
            const bool smp = a.skel_smp_ok && a.skel_smp_gen == a.skel_gen &&
                             a.skel_smp.size() == a.skel_proxy.size() * 12u &&
                             steady_now_ms() - a.skel_smp_ms < SKEL_SAMPLE_KEEP_MS;
            for (size_t j = 0; smp && j < a.skel_proxy.size(); ++j) {
                SkinXf x;
                const float* const xd = a.skel_smp_d.size() == a.skel_proxy.size() * 3u
                                      ? &a.skel_smp_d[j * 3u] : nullptr;   // PXY_SMOOTH.
                if (!skin_rel(&a.skel_smp[j * 12u], &a.skel_smp[j * 12u + 3u],
                              a.skel_smp_par, a.skel_smp_par + 3, x, xd)) continue;
                in.pose[j] = x;
                if (!in.pose_ok[j]) in.cut = true;   // Rest to live: not a motion.
                in.pose_ok[j] = 1u;
            }
            if (cloth) {
                // The cloth's guide, every frame (a few floats per bone). The skin's own job waits (dirty) until the
                // cloth lets go; a job still running from before finishes into a buffer the upload does not install.
                skin_affine(in, mesh_def(in.mesh), in.guide);
                in.dirty = true;
                in.cut = true;   // No collider pose is built meanwhile: the chain breaks here.
                continue;
            }
            jobs.push_back(sp);
        }
    }
    {   // SKIN_MAP: the erase.
        std::lock_guard<std::mutex> se(g_skin_mu);
        for (auto it = g_skin.begin(); it != g_skin.end();) {
            if (!it->second->seen) it = g_skin.erase(it);
            else ++it;
        }
    }
    // Rest snapshots no live instance's mesh uses. A job still running holds its own reference.
    for (auto it = g_skin_rest.begin(); it != g_skin_rest.end();) {
        bool used = false;
        for (const auto& kv : g_skin) if (kv.second->mesh == it->first) { used = true; break; }
        if (!used) it = g_skin_rest.erase(it);
        else ++it;
    }
    bool queued = false;
    for (const std::shared_ptr<SkinInst>& sp : jobs) {
        SkinInst& in = *sp;
        // A new job only when a pose some bone follows has changed since the last one was handed over, or the
        // upload asked for one. SKIN_NEED_FULL: need_full is the upload's request for a result that carries
        // vertices, and is a reason on its own: a collider's jobs keep done current (a mesh that is no collider
        // leaves done behind through its skipped jobs, so its pose still differs), so were a pose change the only
        // reason, a collider whose pose stood bit-still from the moment the step stopped owning its buffer would never
        // hand the full job over - the upload would ask again every pass, take nothing, and the last positions-only
        // result (its BVH and box) would never be installed. The request makes the job below a full one (it clears
        // owned) and that job clears it.
        bool need = in.dirty || in.done.size() != in.pose.size() ||
                    in.done_collide != in.want_collide ||   // COLLIDER: switched on or off.
                    in.done_convex != in.want_convex ||     // COL_CONVEX: the pose's refit changes.
                    in.need_full.load(std::memory_order_relaxed);   // SKIN_NEED_FULL.
        for (size_t b = 0; b < in.drive_px.size() && !need; ++b) {
            const int32_t j = in.drive_px[b];
            if (j < 0) continue;
            if (in.done_ok[j] != in.pose_ok[j] ||
                memcmp(&in.done[j], &in.pose[j], sizeof(SkinXf)) != 0) need = true;
        }
        if (!need) continue;
        // SKIN_GPU: while the render thread's step owns the buffer (it wrote it within SKIN_STEP_OWN cycles,
        // the span skin_upload leaves the bytes and the box to it) the worker's vertices would be thrown away. A
        // collider's job then takes the positions alone; a mesh that is no collider takes no job at all, unless its
        // collider flags just changed (that one job records the switch: the BVH dropped, the motion chain cut).
        // dirty and done stand through a skipped job, so the moment the step stops owning (a hidden mesh, a session
        // the render thread does not flush in) the next sync hands over a full one.
        const uint64_t sc = in.step_cycle_pub.load(std::memory_order_relaxed);
        const uint64_t now = g_topo_cycles_pub.load(std::memory_order_relaxed);
        const bool owned = sc != ~0ull && now >= sc && now - sc <= SKIN_STEP_OWN &&
                           in.gen.load(std::memory_order_relaxed) != 0u &&
                           !in.need_full.load(std::memory_order_relaxed);
        if (owned && !in.want_collide && in.done_collide == in.want_collide &&
            in.done_convex == in.want_convex) continue;
        if (in.busy.load(std::memory_order_acquire)) {
            // Still skinning the last pose: this one waits a frame (done is left alone, so the next sync sees it
            // again) and is counted.
            if (stats_on()) g_skin_skipped.fetch_add(1u, std::memory_order_relaxed);
            continue;
        }
        const MeshDef& d = mesh_def(in.mesh);
        skin_affine(in, d, in.in_a);
        in.in_rest = skin_rest_of(in.mesh);
        in.done = in.pose;
        in.done_ok = in.pose_ok;
        in.dirty = false;
        in.in_collide = in.want_collide;
        in.done_collide = in.want_collide;
        in.in_convex = in.want_convex;   // COL_CONVEX.
        in.done_convex = in.want_convex;
        // SKIN_GPU: no vertices while the step owns the buffer. SKIN_HIDDEN_COL: nor while the object is
        // hidden - unless the upload asked (need_full: a buffer that holds no pose yet) or this is the first result.
        in.in_verts = !owned && (!in.hidden.load(std::memory_order_relaxed) ||
                                 in.need_full.load(std::memory_order_relaxed) ||
                                 in.gen.load(std::memory_order_relaxed) == 0u);
        if (in.in_verts) in.need_full.store(false, std::memory_order_relaxed);   // This job answers it.
        in.in_cut = in.cut;
        in.cut = false;
        if (!queued) { cloth_workers_start(); queued = true; }
        {
            std::lock_guard<std::mutex> q(g_cloth_mu);
            g_skin_q.push_back(sp);
            in.busy.store(true, std::memory_order_release);   // POOL_BUSY_AFTER_PUSH (cloth_sync).
            g_skin_q_n.store(static_cast<uint32_t>(g_skin_q.size()), std::memory_order_relaxed);
        }
        g_skin_cv.notify_one();   // POOL_SPLIT: the skin pool's own wake.
    }
}

// SKEL - the pose a skeletal binding hands its cloth: one affine map per bone from this frame's skin_sync,
// for this creation stamp and mesh. False when no binding of it is the cloth's, or its mesh has no bones. Called
// from both threads (cloth_sync, and cloth_upload via cloth_pin_fresh), so it walks g_skin under g_skin_mu.
inline bool skin_guide_of(uint64_t seq, int mesh, std::vector<float>& out) {
    std::lock_guard<std::mutex> g(g_skin_mu);
    for (const auto& kv : g_skin) {
        const SkinInst& in = *kv.second;
        if (!in.cloth || !in.seen || in.seq != seq || in.mesh != mesh) continue;
        out = in.guide;
        return !out.empty();
    }
    return false;
}

// Device side, in the flush after cloth_upload (which empties the slot tables, installs the cloth's buffers and
// records their boxes). Uploads and installs each newly published skinned result, then hands every binding's object
// the box of what its slot draws, under g_draw_list_mutex: the skinned buffer's box, a cloth's installed buffer's
// box, or the rest box. A failed upload leaves the last installed result and box. Box, key and buffer all come from
// the same published result (SkinGpu).
inline void skin_upload(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    KH_PROF_SCOPE(PROF_SKIN_UPLOAD);   // PROF.
    if (!ctx || !dev) return;
    uint64_t meshes = 0, bones = 0, driven = 0, proxies = 0;
    uint64_t pooled = 0;   // SKIN_POOL.
    // put = false: vb holds the render thread's step's bytes (SkinGpu::by_step - this cycle's or an earlier one's,
    // no worker write since), and the step wrote their box with them; the buffer's vertices are normalised to THAT
    // box (skin_verts), so the object keeps it.
    // vbase: SKIN_POOL, the slice's vertex base (0 = none), handed over whatever 'put' says.
    struct SkinPut { std::string h; uint64_t seq; int mesh; float ctr[3]; float size[3]; bool put; uint32_t vbase; };
    std::vector<SkinPut> su_put;
    SkinColMap col;   // This upload's collider table, published with the boxes.
    // A put for this binding, holding the rest box (skel_rest_box's values) until something installed says
    // otherwise.
    auto start = [](const std::string& su_h, const SkinInst& in) {
        SkinPut p;
        p.h = su_h;
        p.seq = in.seq;
        p.mesh = in.mesh;
        p.put = true;
        p.vbase = 0u;
        const MeshDef& d = mesh_def(in.mesh);
        memcpy(p.ctr, d.native_ctr, sizeof(p.ctr));
        memcpy(p.size, d.native_size, sizeof(p.size));   // SQF order.
        return p;
    };
    // An installed buffer's box: engine axes in, the object's SQF order out.
    auto box = [](SkinPut& p, const float* c, const float* s) {
        memcpy(p.ctr, c, sizeof(p.ctr));
        p.size[0] = s[0];
        p.size[1] = s[2];
        p.size[2] = s[1];
    };
    {
        // The cloth's bindings, and the counts for every binding.
        std::lock_guard<std::mutex> c(g_cloth_mu);
        std::lock_guard<std::mutex> sk(g_skin_mu);   // Cloth -> skin order.
        for (auto& kv : g_skin) {
            SkinInst& in = *kv.second;
            proxies += in.pose.size();
            if (!mesh_alive(in.mesh)) continue;
            bones += mesh_def(in.mesh).skin_bones.size();
            driven += in.bones_driven;
            if (!in.cloth) continue;
            SkinPut p = start(kv.first, in);
            const auto ci = g_cloth.find(in.slot);
            if (ci != g_cloth.end()) {
                const ClothInst& cl = *ci->second;
                const uint32_t s = in.slot;
                const bool on = !cl.retired && cl.seq == in.seq && cl.mesh == in.mesh &&
                                cl.vb && s < g_cloth_vb_slot.size() && g_cloth_vb_slot[s] == cl.vb;
                if (on && cl.uploaded_box_on) box(p, cl.uploaded_box_ctr, cl.uploaded_box_size);
            }
            su_put.push_back(p);
        }
    }
    // SKIN_GPU: the device objects, before any lock (a first compile takes a moment), once some binding has a
    // result to install.
    bool so_ok = false;
    if (SKIN_GPU_ON && skin_so_askable()) {   // SKIN_SO_RETRY.
        bool any = false;
        {
            std::lock_guard<std::mutex> g(g_skin_mu);
            for (const auto& kv : g_skin) {
                if (!kv.second->cloth && kv.second->gen.load(std::memory_order_acquire) != 0u) { any = true; break; }
            }
        }
        if (any) so_ok = skin_so_ensure(dev);
    }
    // SKIN_SO_LAZY: the large class, once some binding with a result has a palette only it takes - asked only
    // while it is missing and askable (after the prewarm, no standing refusal), so a made or refused class costs
    // nothing here; until then each flush walks the bindings once under g_skin_mu (a session without such a
    // skeleton: every flush, a few compares a binding).
    if (so_ok && !skin_so_big_ready() && skin_so_big_askable()) {
        bool big = false;
        {
            std::lock_guard<std::mutex> g(g_skin_mu);
            for (const auto& kv : g_skin) {
                const SkinInst& bi = *kv.second;
                if (bi.cloth || bi.gen.load(std::memory_order_acquire) == 0u || !bi.rest_draw) continue;
                const size_t bn = bi.rest_draw->sup.pal.size();
                if (bi.rest_draw->sup.ok && bn > SKIN_SO_BONES_S && bn <= KH_SKIN_SO_BONES) {
                    big = true;
                    break;
                }
            }
        }
        if (big) skin_so_big_ensure(dev);
    }
    {
        // g_cloth_mu first (cloth -> skin): every job leaves g_skin_q under it, so none can start writing out[front]
        // (the buffer this walk copies, and the box, key and BVH beside it) before the walk ends. A job already
        // running writes the other buffer. The cloth's upload holds it for the same reason.
        std::lock_guard<std::mutex> c2(g_cloth_mu);
        std::lock_guard<std::mutex> g(g_skin_mu);
        for (auto& rg : g_skin_rest_gpu) rg.second.seen = false;   // SKIN_GPU: swept below.
        for (auto it = g_skin_gpu.begin(); it != g_skin_gpu.end();) {
            if (g_skin.find(it->first) == g_skin.end()) {   // Its instance went: released here, in the flush.
                skin_gpu_vb_drop(it->second);   // SKIN_POOL: or its slice handed back.
                it = g_skin_gpu.erase(it);
            } else ++it;
        }
        skin_pool_trim(dev, ctx);   // SKIN_POOL: a thinned pool repacked, before any install.
        if (so_ok && !g_skin_pool_failed) {   // SKIN_POOL: this flush's new slices, reserved per pool.
            std::unordered_map<uint64_t, uint32_t> need;
            for (const auto& rk : g_skin) {
                const SkinInst& ri = *rk.second;
                if (ri.cloth || !mesh_alive(ri.mesh) || ri.mesh < 0 ||
                    ri.gen.load(std::memory_order_acquire) == 0u) continue;
                const SkinRest* const rr = ri.rest_draw.get();
                const size_t rb = mesh_def(ri.mesh).skin_bones.size();
                if (!rr || !rr->sup.ok || rb == 0u ||
                    !skin_so_class_ok(*rr) ||   // SKIN_SO_BIG / SKIN_SO_LAZY.
                    rr->sup.root.size() != rb || rr->inf.size() != rr->verts.size() ||
                    rr->verts.empty() || rr->verts.size() > 0xFFFFFFFFull / sizeof(MeshVertex)) continue;
                const UINT rv = static_cast<UINT>(rr->verts.size() * sizeof(MeshVertex));
                const uint64_t rkey = (static_cast<uint64_t>(static_cast<uint32_t>(ri.mesh)) << 32) | rv;
                const auto rg = g_skin_gpu.find(rk.first);
                // SKIN_GPU_MOVE: a CPU-skinned buffer the walk moves to stream output takes a slice, new result or
                // not - when its rest stream stands (one the walk makes again this flush is taken by growing).
                const auto rs = g_skin_rest_gpu.find(rr);
                const bool rmv = rg != g_skin_gpu.end() && rs != g_skin_rest_gpu.end() &&
                                 rs->second.vb && skin_gpu_movable(rg->second, *rr, rb);
                if (!rmv && rg != g_skin_gpu.end() && rg->second.valid &&   // The walk skips it.
                    rg->second.gen == ri.gen.load(std::memory_order_acquire)) continue;
                const uint32_t rf = ri.front.load(std::memory_order_acquire) & 1u;
                if (ri.out[rf].size() * sizeof(MeshVertex) != rv) continue;   // Not stream output.
                if (!rmv && rg != g_skin_gpu.end() && rg->second.vb &&
                    rg->second.bytes == rv &&
                    (!rg->second.chunk || rg->second.chunk->key == rkey)) continue;   // It stands.
                ++need[rkey];
            }
            for (const auto& nk : need) {
                skin_pool_reserve(dev, ctx, nk.first, nk.second);
            }
        }
        for (auto& kv : g_skin) {
            SkinInst& in = *kv.second;
            if (in.cloth || !mesh_alive(in.mesh)) continue;
            SkinPut p = start(kv.first, in);
            {   // SKIN_GPU: this binding's rest stream stays, and is made here when its snapshot changed under a
                // buffer that stands (a mesh of the same byte size, a rebuilt snapshot): the batch would find none
                // and leave it to the CPU.
                const auto gq = g_skin_gpu.find(kv.first);
                if (so_ok && gq != g_skin_gpu.end() && gq->second.vb && gq->second.so) {
                    skin_rest_gpu_ensure(dev, in.rest_draw, mesh_def(in.mesh).skin_bones.size());
                } else {
                    const auto rg = g_skin_rest_gpu.find(in.rest_draw.get());
                    if (rg != g_skin_rest_gpu.end()) rg->second.seen = true;
                }
            }
            const uint32_t gen = in.gen.load(std::memory_order_acquire);
            if (gen != 0u) {
                SkinGpu& gp = g_skin_gpu[kv.first];
                // SKIN_GPU_MOVE: a CPU-skinned buffer whose binding the GPU can skin now (the device refused it a
                // stream-output buffer or its mesh's rest stream before) moves to stream output, its bytes and record
                // standing; the install below then takes it as a stream-output buffer that stood.
                {
                    const size_t mvb = mesh_def(in.mesh).skin_bones.size();
                    if (so_ok && in.rest_draw && skin_gpu_movable(gp, *in.rest_draw, mvb) &&
                        skin_rest_gpu_ensure(dev, in.rest_draw, mvb)) {
                        // UVS_SRC_FORGET: a vertex stage over this binding primes again from the new buffer - its
                        // source is a weak pair, and a chunk a later take grows could land at the old one's address.
                        if (skin_gpu_move(dev, ctx, in.mesh, gp)) {
                            uvs_src_forget(in.slot);
                            stat_inc(g_stats.skin_gpu_moves);   // skinGpuMoves.
                        } else {
                            stat_inc(g_stats.skin_gpu_refused);   // skinGpuRefused.
                        }
                    }
                }
                if (!gp.valid || gp.gen != gen) {
                    const uint32_t f = in.front.load(std::memory_order_acquire) & 1u;
                    // SKIN_GEN_OF: the buffer's own gen - gen, read first, can name the result before it
                    // (a job stores front, then gen), and installing under it would install this one twice.
                    const uint32_t gf = in.gen_of[f];
                    const std::vector<MeshVertex>& src = in.out[f];
                    const UINT bytes = static_cast<UINT>(src.size() * sizeof(MeshVertex));
                    if (bytes != 0u) {
                        // SKIN_POOL: and a slice of another mesh's pool (the binding's mesh changed at the same
                        // byte size) is handed back, so a chunk holds one mesh's poses.
                        const uint64_t pm = static_cast<uint64_t>(static_cast<uint32_t>(in.mesh));
                        if (gp.vb && (gp.bytes != bytes ||
                                      (gp.chunk && (gp.chunk->key >> 32) != pm))) {
                            skin_gpu_vb_drop(gp);   // SKIN_POOL: or its slice handed back.
                            gp.valid = false;
                            gp.step_cycle = ~0ull;
                            gp.by_step = false;
                            gp.step_key = 0u;
                            // UVS_SRC_FORGET: the buffer made below may land at the dropped one's address - a vertex
                            // stage over this binding primes again from it (a move's rule).
                            uvs_src_forget(in.slot);
                        }
                        if (!gp.vb) {
                            // SKIN_GPU: a buffer the GPU can skin into (DEFAULT with the stream-output bind) when
                            // the objects, this mesh's trees and its rest stream all exist and its palette fits; the
                            // CPU's DYNAMIC buffer otherwise, and if the device refuses the first. SKIN_POOL: the
                            // first is a slice of the mesh's pool, or a buffer of the binding's own when the pool
                            // cannot give one. SKIN_GPU_MOVE: a CPU buffer made for a refusal moves to stream
                            // output once the device gives one.
                            gp.so = false;
                            const std::shared_ptr<const SkinRest>& rest = in.rest_draw;
                            const size_t nb = mesh_def(in.mesh).skin_bones.size();
                            bool so = so_ok && rest && rest->sup.ok && nb != 0u &&
                                      skin_so_class_ok(*rest) &&   // SKIN_SO_BIG / SKIN_SO_LAZY.
                                      rest->sup.root.size() == nb && rest->inf.size() == rest->verts.size() &&
                                      rest->verts.size() * sizeof(MeshVertex) == bytes;
                            if (so) so = skin_rest_gpu_ensure(dev, rest, nb) != nullptr;   // The rest stream, once per snapshot.
                            const bool sow = so;   // SKIN_GPU_MOVE: stream output wanted (and possible).
                            D3D11_BUFFER_DESC bd = {};
                            bd.ByteWidth = bytes;
                            if (so && !skin_pool_take(dev, ctx, in.mesh, bytes, gp)) {
                                bd.Usage = D3D11_USAGE_DEFAULT;
                                bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
                                if (FAILED(dev->CreateBuffer(&bd, nullptr, &gp.vb))) { gp.vb = nullptr; so = false; }
                            }
                            if (!so) {
                                bd.Usage = D3D11_USAGE_DYNAMIC;
                                bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                                bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                                if (FAILED(dev->CreateBuffer(&bd, nullptr, &gp.vb))) gp.vb = nullptr;
                            }
                            // SKIN_GPU_MOVE: stream output wanted and both its buffers refused - asked again (a
                            // move) SKIN_GPU_RETRY cycles on, or by this install every flush while the DYNAMIC
                            // buffer is refused too; any other outcome leaves no refusal standing.
                            gp.so_refused = (sow && !so) ? g_topo_cycles : ~0ull;
                            if (sow && !so) stat_inc(g_stats.skin_gpu_refused);   // skinGpuRefused.
                            if (gp.vb) { gp.bytes = bytes; gp.valid = false; gp.so = so; }
                        }
                        // SKEL_DRAW: a buffer the render thread skinned this cycle keeps those bytes; the record
                        // still takes the result below. SKIN_STEP_OWN - a buffer the step skinned within
                        // SKIN_STEP_OWN cycles is the step's: the worker's result (a pose sampled at Draw3D,
                        // unpaired) updates the record (gen, key, box, collider) but not the bytes or the box. Past
                        // that many cycles without a step (hidden mesh, stalled frame) the worker's bytes return.
                        const bool owned = gp.vb && gp.step_cycle != ~0ull &&
                                           gp.step_cycle != g_topo_cycles && g_topo_cycles > gp.step_cycle &&
                                           g_topo_cycles - gp.step_cycle <= SKIN_STEP_OWN;
                        const bool stepped = gp.vb && (gp.step_cycle == g_topo_cycles ||
                                             owned);
                        // SKIN_GPU: the worker's bytes are written only when its result carries them
                        // (SkinInst::out_ok - a job handed over while the step owned the buffer skinned none). A
                        // buffer that needs them and has none asks for a full job and takes nothing this time: the
                        // record waits with the bytes, as it does for a Map that fails.
                        bool wrote = false;
                        // SKIN_HIDDEN_COL: a hidden object's result without vertices, for a buffer that already
                        // holds a pose - the record is taken, the bytes stand, nothing is asked for.
                        const bool hid = gp.vb && gp.valid && !stepped && !in.out_ok[f] &&
                                         in.hidden.load(std::memory_order_relaxed);
                        if (gp.vb && !stepped && !hid) {
                            if (!in.out_ok[f]) {
                                in.need_full.store(true, std::memory_order_relaxed);
                            } else if (gp.so) {
                                // The whole array (SKIN_POOL: its slice alone).
                                skin_so_write(ctx, skin_gpu_ref(gp), bytes,
                                              gp.chunk != nullptr, src.data());
                                wrote = true;
                            } else {
                                D3D11_MAPPED_SUBRESOURCE m = {};
                                if (SUCCEEDED(ctx->Map(gp.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) && m.pData) {
                                    memcpy(m.pData, src.data(), bytes);   // The whole array: DISCARD leaves garbage.
                                    ctx->Unmap(gp.vb, 0);
                                    wrote = true;
                                }
                            }
                            if (wrote) {
                                gp.by_step = false;   // The worker's bytes again: the put may follow.
                                uvs_src_wrote(in.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again.
                            }
                        }
                        if (gp.vb && (stepped || wrote || hid)) {
                            // COLLIDER: a pose this record has not held before takes this frame's stamp; the same
                            // pose uploaded again after a device reset keeps its own.
                            if (gp.gen != gf) gp.serial = ++g_skin_col_stamp;
                            gp.gen = gf;   // SKIN_GEN_OF.
                            gp.key = in.key_of[f];
                            memcpy(gp.box_ctr, in.box_ctr[f], sizeof(gp.box_ctr));
                            memcpy(gp.box_size, in.box_size[f], sizeof(gp.box_size));
                            gp.bvh = in.bvh_of[f];
                            gp.valid = true;
                        }
                    }
                }
                if (gp.valid && gp.vb) {
                    p.put = !gp.by_step;   // SKEL_DRAW: the step's box stands with its bytes (SkinGpu::by_step).
                    const uint32_t s = in.slot;
                    if (g_cloth_vb_slot.size() <= s) g_cloth_vb_slot.resize(s + 1u, nullptr);
                    if (g_cloth_gen_slot.size() <= s) g_cloth_gen_slot.resize(s + 1u, 0u);
                    if (g_cloth_mesh_slot.size() <= s) g_cloth_mesh_slot.resize(s + 1u, -1);
                    if (g_cloth_sim_slot.size() <= s) g_cloth_sim_slot.resize(s + 1u, 0u);
                    if (g_cloth_voff_slot.size() <= s) g_cloth_voff_slot.resize(s + 1u, 0u);   // POOL.
                    if (g_cloth_pool_slot.size() <= s) g_cloth_pool_slot.resize(s + 1u, nullptr);
                    g_cloth_sim_slot[s] = 0u;   // LOD_SIM_SLOT: the skin's buffer, every level posed.
                    g_cloth_vb_slot[s] = gp.vb;
                    g_cloth_voff_slot[s] = gp.off;      // SKIN_POOL: its slice, in this chunk.
                    g_cloth_pool_slot[s] = gp.chunk;
                    if (gp.chunk) {
                        p.vbase = gp.off / static_cast<UINT>(sizeof(MeshVertex));
                        ++pooled;
                    }
                    g_cloth_gen_slot[s] = gp.by_step ? gp.step_key : gp.key;   // The key follows the bytes.
                    g_cloth_mesh_slot[s] = in.mesh;
                    box(p, gp.box_ctr, gp.box_size);
                    ++meshes;
                    if (gp.bvh) {   // COLLIDER: this box's shape, for the gather.
                        SkinCol& sc = col[in.seq];
                        sc.mesh = in.mesh;
                        memcpy(sc.ctr, p.ctr, sizeof(sc.ctr));
                        memcpy(sc.size, p.size, sizeof(sc.size));
                        sc.bvh = gp.bvh;
                        sc.serial = gp.serial;
                    }
                }
            }
            su_put.push_back(p);
        }
        for (auto rg = g_skin_rest_gpu.begin(); rg != g_skin_rest_gpu.end();) {   // SKIN_GPU: a rest stream no binding draws from.
            if (!rg->second.seen) { KH_SAFE_RELEASE(rg->second.vb); rg = g_skin_rest_gpu.erase(rg); }
            else ++rg;
        }
    }
    {
        // The box of what the slot draws. The position moves with the centre so that it stays root + skel_ctr *
        // rot_m for a late draw in this same flush, before attach_step next recomputes it.
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (const SkinPut& p : su_put) {
            const auto oit = g_draw_list.find(p.h);
            if (oit == g_draw_list.end()) continue;
            RenderObject& o = oit->second;
            if (!o.skel || o.seq != p.seq || o.mesh != p.mesh) continue;
            // SKIN_POOL: the base of the slice the tables now name, into the record at the next sync.
            if (o.skin_vbase != p.vbase) {
                o.skin_vbase = p.vbase;
                scene_mark(o.slot);
            }
            if (!p.put) continue;
            if (memcmp(o.skel_ctr, p.ctr, sizeof(o.skel_ctr)) == 0 &&
                memcmp(o.size, p.size, sizeof(p.size)) == 0) continue;
            float d[3];
            for (int k = 0; k < 3; ++k) d[k] = p.ctr[k] - o.skel_ctr[k];
            float w[3] = { d[0], d[1], d[2] };
            if (o.rotated) {
                for (int k = 0; k < 3; ++k) {
                    w[k] = d[0] * o.rot_m[k] + d[1] * o.rot_m[3 + k] + d[2] * o.rot_m[6 + k];
                }
            }
            double x[3];   // POS_RES: the move applied to the exact centre.
            pos_exact(o, x);
            x[0] += static_cast<double>(w[0]);
            x[1] += static_cast<double>(w[2]);
            x[2] += static_cast<double>(w[1]);
            pos_set_exact(o, x);
            memcpy(o.skel_ctr, p.ctr, sizeof(o.skel_ctr));
            memcpy(o.size, p.size, sizeof(p.size));
            scene_mark(o.slot);
        }
        skin_col_publish(col);   // With the boxes, in their scope.
    }
    g_stats.skin_meshes = meshes;
    g_stats.skin_pooled = pooled;   // SKIN_POOL.
    g_skin_pool_any = pooled != 0u;   // This flush's tables (installed above, under the tables' rules).
    g_stats.skin_bones = bones;
    g_stats.skin_bones_driven = driven;
    g_stats.skin_proxies = proxies;
    g_stats.skin_skipped = g_skin_skipped.load(std::memory_order_relaxed);
    g_stats.skin_pool_full = g_skin_pool_full.load(std::memory_order_relaxed);   // skinPoolFull.
}


// Device teardown (engine_reset_release: under the graphics lock, on the render thread itself, or its best effort
// holding neither). The CPU instances keep their pose and last result; the next upload (the flush) re-creates and
// re-uploads. The slot tables are emptied by
// cloth_release_all beside it.
inline void skin_release_all() {
    std::lock_guard<std::mutex> g(g_skin_mu);
    for (auto& kv : g_skin_gpu) {
        skin_gpu_vb_drop(kv.second);   // SKIN_POOL: a slice hands back; the chunk goes with its last.
        kv.second.bytes = 0;
        kv.second.valid = false;
        kv.second.step_cycle = ~0ull;
        kv.second.by_step = false;
        kv.second.so = false;   // SKIN_GPU: decided again with the next buffer.
        kv.second.so_refused = ~0ull;   // SKIN_GPU_MOVE: a new device is asked at once.
    }
    for (auto& rg : g_skin_rest_gpu) KH_SAFE_RELEASE(rg.second.vb);   // SKIN_GPU: the rest streams.
    g_skin_rest_gpu.clear();
    for (auto& p : g_skin_pool) {   // SKIN_POOL: none is left by the drops above; a stray one goes too.
        for (std::unique_ptr<SkinPoolChunk>& c : p.second.chunks) {
            KH_SAFE_RELEASE(c->srv);
            KH_SAFE_RELEASE(c->vb);
        }
    }
    g_skin_pool.clear();
    g_skin_pool_failed = false;   // A new device is asked again.
    g_skin_pool_made = false;     // SKIN_POOL_MADE: likewise.
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // SKIN_SO_RETRY: likewise (g_res is released).
    g_skin_so_retry_at = ~0ull;
    g_skin_so_big_failed = false;   // SKIN_SO_LAZY: the large class's latch, likewise.
    g_skin_so_big_retry_at = ~0ull;
    g_skin_pool_any = false;      // cloth_release_all emptied the tables beside this.
}

// Mission teardown, from the game thread under the park, beside cloth_drop_all: the buffers and the instances
// both go. Its one caller (session_teardown) has joined the workers first (cloth_workers_stop), so no job
// runs here.
inline void skin_drop_all() {
    skin_release_all();
    {
        std::lock_guard<std::mutex> g(g_skin_mu);
        g_skin_gpu.clear();
    }
    {   // SKIN_MAP.
        std::lock_guard<std::mutex> s(g_skin_mu);
        g_skin.clear();
    }
    g_skin_rest.clear();
    // Locked rather than relying on the callers holding a park.
    { std::lock_guard<std::mutex> c(g_skin_col_mu); g_skin_col.reset(); }
    g_skin_col_snap.reset();   // Game thread, as the one caller (session_teardown, on both mission edges) is.
}
// The batch's per-binding rule: the buffer and stream a job is skinned with on the GPU, or none (the pool takes it).
// Under g_skin_mu. SKIN_CULL decides only for a job this gives a target, so a culled job is one the GPU would skin.
inline SbTarget skin_so_target(const SkinDrawJob& j) {
    const SbTarget none = { VbRef{}, nullptr, 0u };
    if (!SKIN_GPU_ON || !g_res.skin_so_vs || !g_res.skin_so_gs || !g_res.skin_so_layout || !g_res.skin_so_cb) {
        return none;
    }
    const SkinInst* const in = j.a->skin.get();
    if (!in || !in->rest_draw) return none;
    const SkinRest& r = *in->rest_draw;
    const size_t nb = j.a->draw_a.size() / 12u;
    if (!r.sup.ok || nb == 0u || !skin_so_class_ok(r) || r.sup.root.size() != nb) {   // SKIN_SO_BIG / _LAZY.
        return none;
    }
    const auto gp = g_skin_gpu.find(*j.h);
    if (gp == g_skin_gpu.end() || !gp->second.vb || !gp->second.so ||
        gp->second.bytes != static_cast<UINT>(r.verts.size() * sizeof(MeshVertex))) return none;
    const auto rg = g_skin_rest_gpu.find(&r);
    if (rg == g_skin_rest_gpu.end() || !rg->second.vb || rg->second.nv != static_cast<UINT>(r.verts.size()) ||
        rg->second.nv == 0u) return none;
    // SKIN_LVL0 (retired, SkinRest::drawn): every binding is skinned whole, a locked one too.
    return SbTarget{ skin_gpu_ref(gp->second), rg->second.vb, rg->second.nv };
}
inline void skin_so_batch(ID3D11DeviceContext* ctx, std::vector<SkinDrawJob>& jobs) {
    if (!SKIN_GPU_ON || !ctx || jobs.empty()) return;
    if (!g_res.skin_so_vs || !g_res.skin_so_gs || !g_res.skin_so_layout || !g_res.skin_so_cb) return;
    static std::vector<SbTarget> sb_tg;   // Scratch (the step's mutex).
    sb_tg.assign(jobs.size(), SbTarget{ VbRef{}, nullptr, 0u });
    bool any = false;
    {   // g_draw_list_mutex -> g_skin_mu, as skin_draw_finish. The map cannot change under the render thread.
        std::lock_guard<std::mutex> g(g_skin_mu);
        for (size_t i = 0; i < jobs.size(); ++i) {
            if (jobs[i].culled) continue;   // SKIN_CULL: nothing to draw.
            sb_tg[i] = skin_so_target(jobs[i]);
            if (sb_tg[i].vb) any = true;
        }
    }
    if (!any) return;
    {
        // The flag first and last: up before the capture, down after the restore (its scope unwinds after the
        // backup's), as every pass of ours orders them - the restore's own Set calls are ours too.
        InjectionFlagScope flag;
        g_ro.in_injection = true;
        StateBackup sb;
        sb.capture(ctx);
        ID3D11Buffer* so_old = nullptr;
        ctx->SOGetTargets(1, &so_old);
        ctx->IASetInputLayout(g_res.skin_so_layout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        ctx->PSSetShader(nullptr, nullptr, 0);
        // A point list is not a valid input to a hull shader: were the engine ever to leave tessellation stages bound
        // at a step site, the draws below would be dropped and the pose never written. Cleared for the pass, as
        // uvs_step clears them; the restore hands them back.
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        // SKIN_SO_BIG: each job's class by its palette's size (SKIN_SO_PAL) - its VSSkinSo, stream-output object
        // and CBSkin, bound when the class changes (-1: none yet).
        int cls = -1;
        for (size_t i = 0; i < jobs.size(); ++i) {
            const SbTarget& tg = sb_tg[i];
            if (!tg.vb) continue;
            SkinDrawJob& j = jobs[i];
            const std::vector<uint32_t>& pal = j.a->skin->rest_draw->sup.pal;   // The target's snapshot (checked).
            const int c = pal.size() > SKIN_SO_BONES_S ? 1 : 0;
            ID3D11Buffer* const cb = c ? g_res.skin_so_cb_l : g_res.skin_so_cb;
            if (c != cls) {
                cls = c;
                ctx->VSSetShader(c ? g_res.skin_so_vs_l : g_res.skin_so_vs, nullptr, 0);
                ctx->GSSetShader(c ? g_res.skin_so_gs_l : g_res.skin_so_gs, nullptr, 0);
                ctx->VSSetConstantBuffers(0, 1, &cb);
            }
            float bc[3], bs[3];
            if (j.bnd) {   // SKIN_CULL: the pass took the same function of the same inputs.
                memcpy(bc, j.bc, sizeof(bc));
                memcpy(bs, j.bs, sizeof(bs));
            } else {
                skin_so_bound(*j.a->skin->rest_draw, j.a->draw_a.data(), bc, bs);
            }
            D3D11_MAPPED_SUBRESOURCE m = {};
            if (FAILED(ctx->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) continue;
            skin_so_cb_fill(static_cast<float*>(m.pData), bc, bs, j.a->draw_a.data(), pal.data(),
                            pal.size());
            ctx->Unmap(cb, 0);
            const UINT stride = static_cast<UINT>(sizeof(SkinSoVert)), zero = 0u;
            ctx->IASetVertexBuffers(0, 1, &tg.rest, &stride, &zero);
            ctx->SOSetTargets(1, &tg.vb.vb, &tg.vb.off);   // SKIN_POOL: at the slice's offset.
            if (g_orig_draw) g_orig_draw(ctx, tg.nv, 0u); else ctx->Draw(tg.nv, 0u);
            memcpy(j.bc, bc, sizeof(bc));
            memcpy(j.bs, bs, sizeof(bs));
            j.so_vb = tg.vb;
        }
        // The target off before anything binds the buffer as vertices.
        const UINT append = static_cast<UINT>(-1);
        ctx->SOSetTargets(1, &so_old, &append);
        if (so_old) so_old->Release();
        sb.restore(ctx);   // Under the flag.
    }   // The flag handed back last.
}
}