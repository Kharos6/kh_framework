#pragma once

// The feature half (rendering_integration_features.hpp): KH_USER_VS, the user vertex stage. Never include it on its
// own.
namespace RenderIntegration {
inline void uvs_release_all();    // KH_USER_VS: the deformed vertex buffers (defined with the user lanes' fill).
// KH_USER_VS: a material set with a vertex stage has been made (the parse raises it). Zero = uvs_upload walks
// nothing.
static std::atomic<uint32_t> g_uvs_sets{ 0 };

// KH_USER_VS - the user vertex stage: a .hlsl material that defines
//     void KhUserVertex(inout KhUserVtx v)
// (the contract is in cb.hlsl) deforms its submesh's vertices. The deformation must be the SAME in every pass that
// draws the mesh (the colour twins, the five sun tiers, the light maps, the seam, the mirror, the mask cast, PIP,
// inFront) or the shadows and the depth part from it, so it is no hook in a dozen vertex shaders: it runs ONCE a
// cycle as a stream-output pass (GPU skinning's architecture, skin_so_batch) into a vertex buffer of the object's
// own, installed in the slot table every pass resolves its vertex buffer through (mesh_vb_ref). No pass, layout
// or shader but VSUserSo knows it exists.
//   uvs_upload (the flush, after the cloth's and the skin's uploads, which empty and refill the slot tables): for
//     each live object whose set has a vertex stage, a DEFAULT + STREAM_OUTPUT buffer, primed with a copy of its
//     SOURCE (the buffer the slot held - a cloth's, a skinned pose's - or the mesh's own) and installed over that,
//     once every vertex stage of the set has landed (pending: the source draws, undeformed; failed: reported once,
//     that submesh stays undeformed).
//   uvs_step (the render thread's context steps - the cast fire's, the injection's, the flush's, and those of
//     volume_seam_inject, infront_seam_inject, infront_frame_prepare and rp_replay - right after
//     attach_step, so a skinned source holds this cycle's pose; and the end of uvs_upload, so a new buffer is
//     deformed before its first draw): per object once a cycle (again within it, over a source of the object's own,
//     when what it reads has moved since - see the gate), the object's lanes into the pass's own b0 / b1, then per
//     vertex RUN of a submesh with a vertex stage one point-list draw of the source through that stage into the
//     buffer at the run's byte offset. A run is a maximal stretch of vertices one submesh owns (uvs_runs_build:
//     the first submesh, over every LOD level's index block, to reference a vertex) - stream output writes in order,
//     so only a contiguous stretch lands on its own vertices. Runs of submeshes without a stage keep the primed copy;
//     a source that changes under us is copied whole first.
// The shape key (g_cloth_gen_slot) moves with every evaluation, so the sun ladder, the mask, the occluders and the
// seam see an animated caster; vertexStatic (MaterialSet::vertex_static) evaluates again only when the object's
// lanes, its set or its source's key moved, and never applies to a source of the object's own (a pose moves under its
// key's threshold). Objects with a stage leave the buckets (inst_eligible); their bounds pad by vertexBound
// (bounds_size_of). Everything here runs on the flush's thread or the render thread's step (the slot tables' own
// contract) and holds no lock.
struct UvsRun { UINT first; UINT count; uint32_t sub; };
struct UvsGpu {
    ID3D11Buffer* vb = nullptr;
    UINT          bytes = 0;
    uint64_t      seq = 0;            // The object (RenderObject::seq) and mesh the buffer was made for.
    int           mesh = -1;
    VbRef       src;                // Weak: the source the buffer was primed from (this flush's; the pair).
    bool          src_whole = true;   // SKIN_POOL: src is a whole buffer of 'bytes' (else a chunk's slice of them).
    bool          src_own = false;    // The source is the object's own buffer (cloth, skinned): its bytes move.
    bool          src_dirty = true;   // src_own: the source was written, or primed from, since the last evaluation (uvs_src_wrote).
    uint32_t      src_key = 0u;       // The source's shape key at the install.
    std::vector<UvsRun> runs;
    bool          failed = false;     // The buffer's create failed: not retried for this object and mesh.
    bool          seen = false;       // This flush's sweep mark.
    bool          installed = false;  // This flush installed it.
    uint64_t      step_cycle = ~0ull; // The cycle (g_topo_cycles) last evaluated.
    uint32_t      key = 0u;           // The shape key of the bytes the buffer holds.
    bool          done = false;       // vertexStatic: evaluated for done_* below.
    uint32_t      done_src_key = 0u;
    const MaterialSet* done_set = nullptr;
    float         done_lanes[19] = {};   // pos, size, rot_m, color - everything of the object a stage can read.
    // The cycle latch (g_ro.cycle_pv) the last evaluation ran under - the once-a-cycle gate's key; its camera lanes
    // took an adopted copy of it (FX_FRAME_ADOPT). A stale cycle adopts one late.
    bool          done_view_ok = false;
    float         done_view[4][4] = {};
};
static std::unordered_map<uint32_t, UvsGpu> g_uvs;   // By scene slot.
// SKIN_POOL: the source's vertex array into the stage's buffer - the whole source, or its slice of a chunk.
inline void uvs_copy_src(ID3D11DeviceContext* ctx, const UvsGpu& r) {
    if (r.src_whole) {
        ctx->CopyResource(r.vb, r.src.vb);
        return;
    }
    D3D11_BOX b = {};
    b.left = r.src.off;
    b.right = r.src.off + r.bytes;
    b.top = 0;
    b.bottom = 1;
    b.front = 0;
    b.back = 1;
    ctx->CopySubresourceRegion(r.vb, 0, 0, 0, 0, r.src.vb, 0, &b);
}
static uint32_t g_uvs_key_serial = 0u;
inline uint32_t uvs_key_next() {
    if (++g_uvs_key_serial == 0u) g_uvs_key_serial = 1u;   // Never 0: the slot table's 'no key'.
    return g_uvs_key_serial;
}
inline void uvs_release_all() {
    for (auto& kv : g_uvs) {
        // The slot table never names a released buffer.
        if (kv.second.vb && kv.first < g_cloth_vb_slot.size() && g_cloth_vb_slot[kv.first] == kv.second.vb) {
            cloth_slot_forget(kv.first);
        }
        KH_SAFE_RELEASE(kv.second.vb);
    }
    g_uvs.clear();
}
// The slot table names this slot's vertex-stage buffer, installed this flush and deformed from src: what the
// slot draws is src through the stage. For a step that writes a per-instance buffer only once an upload has
// installed it (cloth_pin_draw_step, chain_end_draw): with a stage over it the table names OUR buffer, and the
// bytes it writes still reach the draw - the record is src_own, so uvs_step (after attach_step at every site)
// copies the source whole before it deforms. The tables' thread.
inline bool uvs_draws_from(uint32_t slot, const ID3D11Buffer* src) {
    if (!src || slot >= g_cloth_vb_slot.size()) return false;
    const auto it = g_uvs.find(slot);
    if (it == g_uvs.end()) return false;
    const UvsGpu& r = it->second;
    // SKIN_POOL: src is a cloth's or a chain's own buffer (never a chunk), so its pointer names it.
    return r.installed && r.vb && r.src.vb == src && g_cloth_vb_slot[slot] == r.vb;
}
// USER_VS_SRC_DIRTY - a slot's own per-instance vertex buffer was just written: cloth_upload, skin_upload
// (the flush), skin_draw_finish (the batch's stream-output pose too), cloth_pin_draw_step (the step) and
// chain_end_draw (a chain's corrected end) - EVERY writer of one; a new writer calls this, or a stage over its
// buffer draws the bytes before its write. One empty() test when no object has a vertex stage. The tables' thread, as
// every caller is.
inline void uvs_src_wrote(uint32_t slot) {
    if (g_uvs.empty()) return;
    const auto it = g_uvs.find(slot);
    if (it != g_uvs.end()) it->second.src_dirty = true;
}
// UVS_SRC_FORGET - a slot's source buffer was replaced in the flush (skin_gpu_move, or skin_upload's drop
// of a buffer of another byte size or another mesh's chunk) before uvs_upload ran:
// the record forgets the weak pair it was primed from, so the upload primes again from the new buffer whatever
// address it got (a later take that grows a chunk can place it at the released one's). The tables' thread.
inline void uvs_src_forget(uint32_t slot) {
    if (g_uvs.empty()) return;
    const auto it = g_uvs.find(slot);
    if (it != g_uvs.end()) it->second.src = VbRef();
}
inline void uvs_runs_build(const MeshDef& d, std::vector<UvsRun>& out) {
    out.clear();
    const size_t nv = d.verts.size();
    std::vector<uint32_t> own(nv, 0xFFFFFFFFu);
    for (int l = 0; l <= static_cast<int>(d.lod_n); ++l) {
        const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(d, l);
        for (size_t si = 0; si < tab.size(); ++si) {
            const size_t b = tab[si].index_start;
            const size_t rb_e = b + tab[si].index_count;
            if (rb_e > d.indices.size()) continue;
            for (size_t k = b; k < rb_e; ++k) {
                const uint32_t v = d.indices[k];
                if (v < nv && own[v] == 0xFFFFFFFFu) own[v] = static_cast<uint32_t>(si);
            }
        }
    }
    for (size_t v = 0; v < nv;) {
        size_t e = v + 1u;
        while (e < nv && own[e] == own[v]) ++e;
        if (own[v] != 0xFFFFFFFFu) out.push_back(UvsRun{ static_cast<UINT>(v), static_cast<UINT>(e - v), own[v] });
        v = e;
    }
}
inline bool uvs_cbs_ensure(ID3D11Device* dev) {
    if (g_res.uvs_obj_cb && g_res.uvs_frame_cb) return true;
    if (!dev) return false;
    KH_SAFE_RELEASE(g_res.uvs_obj_cb);
    KH_SAFE_RELEASE(g_res.uvs_frame_cb);
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = static_cast<UINT>(CBOBJ_BYTES);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_res.uvs_obj_cb))) { g_res.uvs_obj_cb = nullptr; return false; }
    bd.ByteWidth = static_cast<UINT>(CBFRAME_BYTES);
    if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_res.uvs_frame_cb))) {
        g_res.uvs_frame_cb = nullptr;
        KH_SAFE_RELEASE(g_res.uvs_obj_cb);
        return false;
    }
    return true;
}
// The lanes a vertexStatic evaluation stands for.
inline void uvs_lanes_of(const RenderObject& o, float out[19]) {
    memcpy(out, o.pos, sizeof(float) * 3u);
    memcpy(out + 3, o.size, sizeof(float) * 3u);
    memcpy(out + 6, o.rot_m, sizeof(float) * 9u);
    memcpy(out + 15, o.color, sizeof(float) * 4u);
}
inline void uvs_step(ID3D11DeviceContext* ctx) {
    if (!ctx || g_uvs.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    scene_sync();   // The lanes every pass of this cycle will draw with (the step before us may have moved them).
    static std::vector<uint32_t> work;   // Scratch (one thread at a time, as the tables).
    work.clear();
    for (auto& kv : g_uvs) {
        const uint32_t s = kv.first;
        UvsGpu& r = kv.second;
        if (!r.vb || !r.src || !r.installed) continue;
        // Once a cycle - except over a source of the object's own: attach_step skins a binding that waited for
        // the injection at ITS step, after the cast fire's evaluation of this cycle, and the colour passes would then
        // draw the stage over the previous pose. Such a record goes on to the USER_VS_SRC_DIRTY test below at
        // every step (the sun map is rebuilt by SUN_RESTEP on the same edge).
        if (r.step_cycle == g_topo_cycles && !r.src_own) continue;
        if (s >= g_scene.objs.size() || !g_scene.alive[s] || s >= g_cloth_vb_slot.size() || g_cloth_vb_slot[s] != r.vb) continue;
        const RenderObject& o = g_scene.objs[s];
        if (o.seq != r.seq || !o.materials || !o.materials->vertex_any || mesh_id_clamp(o.mesh) != r.mesh) continue;
        // Nothing draws it. An invisible casterOnly object still casts while a shadow map takes it (shadow_active,
        // SHADOW_SWITCH, SHADOW_OFF: the maps draw this buffer); an inFront one in any view (SKIN_CASTER_VM).
        // EA_UVS_GATE: the skin's own gate, called rather than restated.
        if (!skin_pose_drawn(o)) continue;   // SHOW_INFRONT.
        // USER_VS_SRC_DIRTY: evaluated this cycle already (so: over a source of the object's own) - again only if
        // something the stage reads has moved since: the source's bytes (uvs_src_wrote - the step before us
        // re-posed it), the object's lanes or set (that step moved it), the pass camera (a stale cycle adopts its
        // view late). The clock alone does not ask: within a cycle the evaluation stands.
        if (r.step_cycle == g_topo_cycles && !r.src_dirty && r.done && r.done_set == o.materials &&
            r.done_view_ok == g_ro.cycle_pv_valid &&
            (!r.done_view_ok || memcmp(r.done_view, g_ro.cycle_pv.view, sizeof(r.done_view)) == 0)) {
            float l[19];
            uvs_lanes_of(o, l);
            if (memcmp(l, r.done_lanes, sizeof(l)) == 0) continue;
        }
        if (o.materials->vertex_static && !r.src_own && r.done && r.done_set == o.materials && r.done_src_key == r.src_key) {
            float l[19];
            uvs_lanes_of(o, l);
            if (memcmp(l, r.done_lanes, sizeof(l)) == 0) { r.step_cycle = g_topo_cycles; continue; }
        }
        work.push_back(s);
    }
    if (work.empty() || !uvs_cbs_ensure(dev)) return;
    {
        // The bracket, as skin_so_batch's: the flag first and last; the stream-output target, which StateBackup
        // does not hold, saved by hand (slot 0 only; slots 1-3 are left null). The pass sets the layout, the
        // topology, IA slot 0, VS / GS / PS / HS / DS and VS b0 / b1 - all in the capture.
        InjectionFlagScope flag;
        g_ro.in_injection = true;
        StateBackup sb;
        sb.capture(ctx);
        ID3D11Buffer* so_old = nullptr;
        ctx->SOGetTargets(1, &so_old);
        const UINT stride = static_cast<UINT>(sizeof(MeshVertex));
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        ctx->PSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        bool ok = true;
        {   // b1: the camera lanes; everything else zero (lighting1.w = 0 says so to a reader).
            ConstantData cbf = {};
            if (g_ro.cycle_pv_valid) {
                // FX_FRAME_ADOPT: the colour passes fill these lanes from their adopted view (the injection, the
                // flush, the UI lane), so the stage takes the same rotation. The once-a-cycle gate keys on the latch's
                // own rows (done_view), so no step runs it again for adoption alone: when the cycle's first step comes
                // before this frame's view is published, adoption declines and the stage keeps the latch's rotation
                // for the cycle.
                RVExtBridge::ProjectionViewTransform pv = g_ro.cycle_pv;
                adopt_frame_view(pv);
                float cam[3];
                cam_of(pv.view, cam);
                fill_user_cam_cb(cbf, cam, pv.view);
            }
            ok = upload_frame_cb(ctx, g_res.uvs_frame_cb, cbf);
        }
        ID3D11Buffer* cbs[2] = { g_res.uvs_obj_cb, g_res.uvs_frame_cb };
        ctx->VSSetConstantBuffers(0, 2, cbs);
        ID3D11VertexShader* bound_vs = nullptr;
        for (size_t w = 0; ok && w < work.size(); ++w) {
            const uint32_t s = work[w];
            UvsGpu& r = g_uvs[s];
            const RenderObject& o = g_scene.objs[s];
            const MaterialSet& ms = *o.materials;
            // The source moved under the primed copy: the whole of it again, the stages over it. (The target bound
            // here, if any, is the previous object's buffer - never this one.)
            if (r.src_own) { uvs_copy_src(ctx, r); r.src_dirty = false; }   // SKIN_POOL: by its slice.
            ConstantData cbd = {};
            cbd.center_size[0] = o.pos[0];
            cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            cbd.center_size[2] = o.pos[1];
            cbd.size_axes[0] = o.size[0];    // SQF [x,y,z] sizes -> engine [x,z,y].
            cbd.size_axes[1] = o.size[2];
            cbd.size_axes[2] = o.size[1];
            fill_obj_rot(cbd, o.rot_m);
            memcpy(cbd.color, o.color, sizeof(cbd.color));
            fill_user_obj_cb(cbd, o);
            bool drew = false;
            for (const UvsRun& run : r.runs) {
                if (run.sub >= ms.slots.size()) continue;
                const Material& m = ms.slots[run.sub];
                if (!m.used || m.shader != 1 || !m.user_vertex || m.user_shader.empty()) continue;
                ID3D11VertexShader* vs = nullptr;
                ID3D11GeometryShader* gs = nullptr;
                ID3D11InputLayout* il = nullptr;
                if (user_mat_vs(dev, m.user_shader, vs, gs, il) != 1) continue;
                cbd.shadow_meta2[3] = static_cast<float>(run.first);   // SV_VertexID restarts at 0 with every draw: the run's first vertex.
                if (!upload_obj_cb(ctx, g_res.uvs_obj_cb, cbd)) { ok = false; break; }
                if (vs != bound_vs) {
                    ctx->IASetInputLayout(il);
                    ctx->VSSetShader(vs, nullptr, 0);
                    ctx->GSSetShader(gs, nullptr, 0);
                    bound_vs = vs;
                }
                const UINT off = run.first * stride;
                ctx->IASetVertexBuffers(0, 1, &r.src.vb, &stride, &r.src.off);   // SKIN_POOL.
                ctx->SOSetTargets(1, &r.vb, &off);
                if (g_orig_draw) g_orig_draw(ctx, run.count, run.first); else ctx->Draw(run.count, run.first);
                drew = true;
            }
            r.step_cycle = g_topo_cycles;
            if (drew || r.src_own) {
                r.key = uvs_key_next();
                if (s < g_cloth_gen_slot.size()) g_cloth_gen_slot[s] = r.key;   // The key follows the bytes.
            }
            r.done = ok;
            r.done_set = o.materials;
            r.done_src_key = r.src_key;
            uvs_lanes_of(o, r.done_lanes);
            r.done_view_ok = g_ro.cycle_pv_valid;
            if (r.done_view_ok) memcpy(r.done_view, g_ro.cycle_pv.view, sizeof(r.done_view));
        }
        // The target off before anything binds a buffer as vertices; the engine's handed back (offset -1: it keeps appending).
        const UINT append = static_cast<UINT>(-1);
        ctx->SOSetTargets(1, &so_old, &append);
        if (so_old) so_old->Release();
        sb.restore(ctx);   // Under the flag.
    }   // The flag handed back last.
}
inline void uvs_upload(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    if (!ctx || !dev) return;
    if (g_uvs.empty() && g_uvs_sets.load(std::memory_order_relaxed) == 0u) return;   // The session has made no vertex stage.
    scene_sync();
    for (auto& kv : g_uvs) { kv.second.seen = false; kv.second.installed = false; }
    const uint32_t n = static_cast<uint32_t>(g_scene.objs.size());
    for (uint32_t s = 0; s < n; ++s) {
        if (!g_scene.alive[s]) continue;
        const RenderObject& o = g_scene.objs[s];
        if (!o.materials || !o.materials->vertex_any || o.fullscreen || o.effect != 0) continue;
        const int mid = mesh_id_clamp(o.mesh);
        if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size() || !mesh_alive(mid)) continue;
        const MeshDef& md = mesh_def(mid);
        const size_t nv = md.verts.size();
        if (nv == 0u || nv > 0xFFFFFFFFull / sizeof(MeshVertex)) continue;
        const UINT bytes = static_cast<UINT>(nv * sizeof(MeshVertex));
        UvsGpu& r = g_uvs[s];
        if (r.seq != o.seq || r.mesh != mid || r.bytes != bytes) {   // Another object in the slot, or another mesh: start over.
            KH_SAFE_RELEASE(r.vb);
            r = UvsGpu();
            r.seq = o.seq; r.mesh = mid; r.bytes = bytes;
        }
        r.seen = true;
        if (r.failed) continue;
        // Every vertex stage of the set has landed, and one at least is ready.
        bool pend = false, any = false;
        for (const Material& m : o.materials->slots) {
            if (!m.used || m.shader != 1 || !m.user_vertex || m.user_shader.empty()) continue;
            ID3D11VertexShader* vs = nullptr;
            ID3D11GeometryShader* gs = nullptr;
            ID3D11InputLayout* il = nullptr;
            const int st = user_mat_vs(dev, m.user_shader, vs, gs, il);
            if (st == 0) pend = true;
            else if (st == 1) any = true;
        }
        if (pend || !any) {
            // Not installed this flush, so nothing evaluates it: what the last evaluation stood for is forgotten.
            // done_set is a WEAK pointer, and this is the one window in which the set it names can be released while
            // the record lives on (the object holds ANOTHER set, whose stage is pending or failed) - a later set
            // allocated at that address would otherwise pass both gates of uvs_step for the old one.
            r.done = false;
            r.src = VbRef();   // UVS_SRC_FORGET: src is weak too (below).
            continue;
        }
        // What the cloth's and the skin's uploads left in the slot, else the mesh's own (SKIN_POOL: the pair).
        // UVS_SRC_FORGET: a record not installed this flush forgets the source it was primed from - src is a weak
        // pair, and a buffer made later at a released one's address (and offset) would pass for it, skipping the
        // prime (stale input; and src_whole naming the wrong kind of copy). The next install primes again.
        const VbRef src = mesh_vb_ref(mid, s);
        if (!src) { r.src = VbRef(); continue; }
        if (!r.vb) {
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = bytes;
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
            if (FAILED(dev->CreateBuffer(&bd, nullptr, &r.vb))) {
                r.vb = nullptr;
                r.failed = true;
                report_error_once_safe("KH RenderIntegration: user vertex stage: a deformed vertex buffer could not "
                                       "be made (the mesh draws undeformed)");
                continue;
            }
            r.src = VbRef();
        }
        if (r.src != src) {   // Prime: the runs no stage owns draw from this copy.
            // Not this mesh's vertex array: the source draws. SKIN_POOL: a chunk's slice is one when the chunk's
            // slices are this size (its pool's key).
            const SkinPoolChunk* const pc = mesh_pool_of(mid, s);
            if (pc) {
                if (static_cast<UINT>(pc->key & 0xFFFFFFFFull) != bytes) { r.src = VbRef(); continue; }
            } else {
                D3D11_BUFFER_DESC sd = {};
                src.vb->GetDesc(&sd);
                if (sd.ByteWidth != bytes || src.off != 0u) { r.src = VbRef(); continue; }
            }
            r.src = src;
            r.src_whole = pc == nullptr;
            uvs_copy_src(ctx, r);
            r.src_dirty = true;   // Another source: whatever was evaluated this cycle was evaluated over the old one.
            r.done = false;
            r.step_cycle = ~0ull;   // USER_VS_REPRIME: the copy voids this cycle's evaluation; the step redoes it.
        }
        r.src_own = src.vb != g_res.mesh_vb[mid];
        r.src_key = cloth_gen_of(s);
        if (r.runs.empty()) uvs_runs_build(md, r.runs);
        if (r.key == 0u) r.key = uvs_key_next();
        if (g_cloth_vb_slot.size() <= s) g_cloth_vb_slot.resize(s + 1u, nullptr);
        if (g_cloth_gen_slot.size() <= s) g_cloth_gen_slot.resize(s + 1u, 0u);
        if (g_cloth_mesh_slot.size() <= s) g_cloth_mesh_slot.resize(s + 1u, -1);
        if (g_cloth_sim_slot.size() <= s) g_cloth_sim_slot.resize(s + 1u, 0u);   // LOD_SIM_SLOT: the source's.
        if (g_cloth_voff_slot.size() <= s) g_cloth_voff_slot.resize(s + 1u, 0u);   // SKIN_POOL.
        if (g_cloth_pool_slot.size() <= s) g_cloth_pool_slot.resize(s + 1u, nullptr);
        g_cloth_vb_slot[s] = r.vb;
        g_cloth_voff_slot[s] = 0u;   // SKIN_POOL: the stage's buffer is its own.
        g_cloth_pool_slot[s] = nullptr;
        g_cloth_gen_slot[s] = r.key;
        g_cloth_mesh_slot[s] = mid;
        r.installed = true;
    }
    for (auto it = g_uvs.begin(); it != g_uvs.end();) {   // An object that left, or lost its stage.
        if (!it->second.seen) { KH_SAFE_RELEASE(it->second.vb); it = g_uvs.erase(it); }
        else ++it;
    }
    uvs_step(ctx);   // A buffer installed just now is deformed before its first draw.
}
}