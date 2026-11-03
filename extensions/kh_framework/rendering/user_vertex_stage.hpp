#pragma once

// The feature half (rendering_integration_features.hpp): KH_USER_VS, the user vertex stage. Never include it on its
// own.
namespace RenderIntegration {
inline void kh_uvs_release_all();    // KH_USER_VS: the deformed vertex buffers (defined with the user lanes' fill).
// KH_USER_VS: a material set with a vertex stage has been made (the parse raises it). Zero = kh_uvs_upload walks
// nothing.
static std::atomic<uint32_t> g_uvs_sets{ 0 };

// KH_USER_VS - the user vertex stage: a .hlsl material that defines
//     void KhUserVertex(inout KhUserVtx v)
// (the contract is in cb.hlsl) deforms its submesh's vertices. The deformation must be the SAME in every pass that
// draws the mesh (the colour twins, the five sun tiers, the light maps, the seam, the mirror, the mask cast, PIP,
// inFront) or the shadows and the depth part from it, so it is no hook in a dozen vertex shaders: it runs ONCE a
// cycle as a stream-output pass (GPU skinning's architecture, kh_skin_so_batch) into a vertex buffer of the object's
// own, installed in the slot table every pass resolves its vertex buffer through (kh_mesh_vb_ref). No pass, layout
// or shader but VSUserSo knows it exists.
//   kh_uvs_upload (the flush, after the cloth's and the skin's uploads, which empty and refill the slot tables): for
//     each live object whose set has a vertex stage, a DEFAULT + STREAM_OUTPUT buffer, primed with a copy of its
//     SOURCE (the buffer the slot held - a cloth's, a skinned pose's - or the mesh's own) and installed over that,
//     once every vertex stage of the set has landed (pending: the source draws, undeformed; failed: reported once,
//     that submesh stays undeformed).
//   kh_uvs_step (the render thread's context steps - the cast fire's, the injection's, the flush's, and those of
//     kh_volume_seam_inject, kh_infront_seam_inject, kh_infront_frame_prepare and kh_rp_replay - right after
//     kh_attach_step, so a skinned source holds this cycle's pose; and the end of kh_uvs_upload, so a new buffer is
//     deformed before its first draw): per object once a cycle (again within it, over a source of the object's own,
//     when what it reads has moved since - see the gate), the object's lanes into the pass's own b0 / b1, then per
//     vertex RUN of a submesh with a vertex stage one point-list draw of the source through that stage into the
//     buffer at the run's byte offset. A run is a maximal stretch of vertices one submesh owns (kh_uvs_runs_build:
//     the first submesh, over every LOD level's index block, to reference a vertex) - stream output writes in order,
//     so only a contiguous stretch lands on its own vertices. Runs of submeshes without a stage keep the primed copy;
//     a source that changes under us is copied whole first.
// The shape key (g_cloth_gen_slot) moves with every evaluation, so the sun ladder, the mask, the occluders and the
// seam see an animated caster; vertexStatic (KhMaterialSet::vertex_static) evaluates again only when the object's
// lanes, its set or its source's key moved, and never applies to a source of the object's own (a pose moves under its
// key's threshold). Objects with a stage leave the buckets (kh_inst_eligible); their bounds pad by vertexBound
// (kh_bounds_size_of). Everything here runs on the flush's thread or the render thread's step (the slot tables' own
// contract) and holds no lock.
struct KhUvsRun { UINT first; UINT count; uint32_t sub; };
struct KhUvsGpu {
    ID3D11Buffer* vb = nullptr;
    UINT          bytes = 0;
    uint64_t      seq = 0;            // The object (RenderObject::seq) and mesh the buffer was made for.
    int           mesh = -1;
    KhVbRef       src;                // Weak: the source the buffer was primed from (this flush's; the pair).
    bool          src_whole = true;   // KH_SKIN_POOL: src is a whole buffer of 'bytes' (else a chunk's slice of them).
    bool          src_own = false;    // The source is the object's own buffer (cloth, skinned): its bytes move.
    bool          src_dirty = true;   // src_own: the source was written, or primed from, since the last evaluation (kh_uvs_src_wrote).
    uint32_t      src_key = 0u;       // The source's shape key at the install.
    std::vector<KhUvsRun> runs;
    bool          failed = false;     // The buffer's create failed: not retried for this object and mesh.
    bool          seen = false;       // This flush's sweep mark.
    bool          installed = false;  // This flush installed it.
    uint64_t      step_cycle = ~0ull; // The cycle (g_topo_cycles) last evaluated.
    uint32_t      key = 0u;           // The shape key of the bytes the buffer holds.
    bool          done = false;       // vertexStatic: evaluated for done_* below.
    uint32_t      done_src_key = 0u;
    const KhMaterialSet* done_set = nullptr;
    float         done_lanes[19] = {};   // pos, size, rot_m, color - everything of the object a stage can read.
    // The cycle latch (g_ro.cycle_pv) the last evaluation ran under - the once-a-cycle gate's key; its camera lanes
    // took an adopted copy of it (KH_FX_FRAME_ADOPT). A stale cycle adopts one late.
    bool          done_view_ok = false;
    float         done_view[4][4] = {};
};
static std::unordered_map<uint32_t, KhUvsGpu> g_uvs;   // By scene slot.
// KH_SKIN_POOL: the source's vertex array into the stage's buffer - the whole source, or its slice of a chunk.
inline void kh_uvs_copy_src(ID3D11DeviceContext* khcs_ctx, const KhUvsGpu& khcs_r) {
    if (khcs_r.src_whole) {
        khcs_ctx->CopyResource(khcs_r.vb, khcs_r.src.vb);
        return;
    }
    D3D11_BOX khcs_b = {};
    khcs_b.left = khcs_r.src.off;
    khcs_b.right = khcs_r.src.off + khcs_r.bytes;
    khcs_b.top = 0;
    khcs_b.bottom = 1;
    khcs_b.front = 0;
    khcs_b.back = 1;
    khcs_ctx->CopySubresourceRegion(khcs_r.vb, 0, 0, 0, 0, khcs_r.src.vb, 0, &khcs_b);
}
static uint32_t g_uvs_key_serial = 0u;
inline uint32_t kh_uvs_key_next() {
    if (++g_uvs_key_serial == 0u) g_uvs_key_serial = 1u;   // Never 0: the slot table's 'no key'.
    return g_uvs_key_serial;
}
inline void kh_uvs_release_all() {
    for (auto& khur_kv : g_uvs) {
        // The slot table never names a released buffer.
        if (khur_kv.second.vb && khur_kv.first < g_cloth_vb_slot.size() && g_cloth_vb_slot[khur_kv.first] == khur_kv.second.vb) {
            kh_cloth_slot_forget(khur_kv.first);
        }
        KH_SAFE_RELEASE(khur_kv.second.vb);
    }
    g_uvs.clear();
}
// The slot table names this slot's vertex-stage buffer, installed this flush and deformed from khud_src: what the
// slot draws is khud_src through the stage. For a step that writes a per-instance buffer only once an upload has
// installed it (kh_cloth_pin_draw_step, kh_chain_end_draw): with a stage over it the table names OUR buffer, and the
// bytes it writes still reach the draw - the record is src_own, so kh_uvs_step (after kh_attach_step at every site)
// copies the source whole before it deforms. The tables' thread.
inline bool kh_uvs_draws_from(uint32_t khud_slot, const ID3D11Buffer* khud_src) {
    if (!khud_src || khud_slot >= g_cloth_vb_slot.size()) return false;
    const auto khud_it = g_uvs.find(khud_slot);
    if (khud_it == g_uvs.end()) return false;
    const KhUvsGpu& khud_r = khud_it->second;
    // KH_SKIN_POOL: khud_src is a cloth's or a chain's own buffer (never a chunk), so its pointer names it.
    return khud_r.installed && khud_r.vb && khud_r.src.vb == khud_src && g_cloth_vb_slot[khud_slot] == khud_r.vb;
}
// KH_USER_VS_SRC_DIRTY - a slot's own per-instance vertex buffer was just written: kh_cloth_upload, kh_skin_upload
// (the flush), kh_skin_draw_finish (the batch's stream-output pose too), kh_cloth_pin_draw_step (the step) and
// kh_chain_end_draw (a chain's corrected end) - EVERY writer of one; a new writer calls this, or a stage over its
// buffer draws the bytes before its write. One empty() test when no object has a vertex stage. The tables' thread, as
// every caller is.
inline void kh_uvs_src_wrote(uint32_t khsw_slot) {
    if (g_uvs.empty()) return;
    const auto khsw_it = g_uvs.find(khsw_slot);
    if (khsw_it != g_uvs.end()) khsw_it->second.src_dirty = true;
}
// KH_UVS_SRC_FORGET - a slot's source buffer was replaced in the flush (kh_skin_gpu_move, or kh_skin_upload's drop
// of a buffer of another byte size or another mesh's chunk) before kh_uvs_upload ran:
// the record forgets the weak pair it was primed from, so the upload primes again from the new buffer whatever
// address it got (a later take that grows a chunk can place it at the released one's). The tables' thread.
inline void kh_uvs_src_forget(uint32_t khsg_slot) {
    if (g_uvs.empty()) return;
    const auto khsg_it = g_uvs.find(khsg_slot);
    if (khsg_it != g_uvs.end()) khsg_it->second.src = KhVbRef();
}
inline void kh_uvs_runs_build(const MeshDef& khrb_d, std::vector<KhUvsRun>& khrb_out) {
    khrb_out.clear();
    const size_t khrb_nv = khrb_d.verts.size();
    std::vector<uint32_t> khrb_own(khrb_nv, 0xFFFFFFFFu);
    for (int khrb_l = 0; khrb_l <= static_cast<int>(khrb_d.lod_n); ++khrb_l) {
        const std::vector<MeshSubmesh>& khrb_tab = mesh_lod_submeshes(khrb_d, khrb_l);
        for (size_t si = 0; si < khrb_tab.size(); ++si) {
            const size_t khrb_b = khrb_tab[si].index_start;
            const size_t khrb_e = khrb_b + khrb_tab[si].index_count;
            if (khrb_e > khrb_d.indices.size()) continue;
            for (size_t k = khrb_b; k < khrb_e; ++k) {
                const uint32_t v = khrb_d.indices[k];
                if (v < khrb_nv && khrb_own[v] == 0xFFFFFFFFu) khrb_own[v] = static_cast<uint32_t>(si);
            }
        }
    }
    for (size_t v = 0; v < khrb_nv;) {
        size_t e = v + 1u;
        while (e < khrb_nv && khrb_own[e] == khrb_own[v]) ++e;
        if (khrb_own[v] != 0xFFFFFFFFu) khrb_out.push_back(KhUvsRun{ static_cast<UINT>(v), static_cast<UINT>(e - v), khrb_own[v] });
        v = e;
    }
}
inline bool kh_uvs_cbs_ensure(ID3D11Device* khce_dev) {
    if (g_res.uvs_obj_cb && g_res.uvs_frame_cb) return true;
    if (!khce_dev) return false;
    KH_SAFE_RELEASE(g_res.uvs_obj_cb);
    KH_SAFE_RELEASE(g_res.uvs_frame_cb);
    D3D11_BUFFER_DESC khce_bd = {};
    khce_bd.ByteWidth = static_cast<UINT>(KH_CBOBJ_BYTES);
    khce_bd.Usage = D3D11_USAGE_DYNAMIC;
    khce_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    khce_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(khce_dev->CreateBuffer(&khce_bd, nullptr, &g_res.uvs_obj_cb))) { g_res.uvs_obj_cb = nullptr; return false; }
    khce_bd.ByteWidth = static_cast<UINT>(KH_CBFRAME_BYTES);
    if (FAILED(khce_dev->CreateBuffer(&khce_bd, nullptr, &g_res.uvs_frame_cb))) {
        g_res.uvs_frame_cb = nullptr;
        KH_SAFE_RELEASE(g_res.uvs_obj_cb);
        return false;
    }
    return true;
}
// The lanes a vertexStatic evaluation stands for.
inline void kh_uvs_lanes_of(const RenderObject& o, float khul_out[19]) {
    memcpy(khul_out, o.pos, sizeof(float) * 3u);
    memcpy(khul_out + 3, o.size, sizeof(float) * 3u);
    memcpy(khul_out + 6, o.rot_m, sizeof(float) * 9u);
    memcpy(khul_out + 15, o.color, sizeof(float) * 4u);
}
inline void kh_uvs_step(ID3D11DeviceContext* khus_ctx) {
    if (!khus_ctx || g_uvs.empty()) return;
    ID3D11Device* khus_dev = RVExtBridge::get_d3d_device();
    if (!khus_dev) return;
    kh_scene_sync();   // The lanes every pass of this cycle will draw with (the step before us may have moved them).
    static std::vector<uint32_t> khus_work;   // Scratch (one thread at a time, as the tables).
    khus_work.clear();
    for (auto& khus_kv : g_uvs) {
        const uint32_t s = khus_kv.first;
        KhUvsGpu& r = khus_kv.second;
        if (!r.vb || !r.src || !r.installed) continue;
        // Once a cycle - except over a source of the object's own: kh_attach_step skins a binding that waited for
        // the injection at ITS step, after the cast fire's evaluation of this cycle, and the colour passes would then
        // draw the stage over the previous pose. Such a record goes on to the KH_USER_VS_SRC_DIRTY test below at
        // every step (the sun map is rebuilt by KH_SUN_RESTEP on the same edge).
        if (r.step_cycle == g_topo_cycles && !r.src_own) continue;
        if (s >= g_scene.objs.size() || !g_scene.alive[s] || s >= g_cloth_vb_slot.size() || g_cloth_vb_slot[s] != r.vb) continue;
        const RenderObject& o = g_scene.objs[s];
        if (o.seq != r.seq || !o.materials || !o.materials->vertex_any || mesh_id_clamp(o.mesh) != r.mesh) continue;
        // Nothing draws it. An invisible casterOnly object still casts while a shadow map takes it (kh_shadow_active,
        // KH_SHADOW_SWITCH, KH_SHADOW_OFF: the maps draw this buffer); an inFront one in any view (KH_SKIN_CASTER_VM).
        // KH_EA_UVS_GATE: the skin's own gate, called rather than restated.
        if (!kh_skin_pose_drawn(o)) continue;   // KH_SHOW_INFRONT.
        // KH_USER_VS_SRC_DIRTY: evaluated this cycle already (so: over a source of the object's own) - again only if
        // something the stage reads has moved since: the source's bytes (kh_uvs_src_wrote - the step before us
        // re-posed it), the object's lanes or set (that step moved it), the pass camera (a stale cycle adopts its
        // view late). The clock alone does not ask: within a cycle the evaluation stands.
        if (r.step_cycle == g_topo_cycles && !r.src_dirty && r.done && r.done_set == o.materials &&
            r.done_view_ok == g_ro.cycle_pv_valid &&
            (!r.done_view_ok || memcmp(r.done_view, g_ro.cycle_pv.view, sizeof(r.done_view)) == 0)) {
            float khus_l[19];
            kh_uvs_lanes_of(o, khus_l);
            if (memcmp(khus_l, r.done_lanes, sizeof(khus_l)) == 0) continue;
        }
        if (o.materials->vertex_static && !r.src_own && r.done && r.done_set == o.materials && r.done_src_key == r.src_key) {
            float khus_l[19];
            kh_uvs_lanes_of(o, khus_l);
            if (memcmp(khus_l, r.done_lanes, sizeof(khus_l)) == 0) { r.step_cycle = g_topo_cycles; continue; }
        }
        khus_work.push_back(s);
    }
    if (khus_work.empty() || !kh_uvs_cbs_ensure(khus_dev)) return;
    {
        // The bracket, as kh_skin_so_batch's: the flag first and last; the stream-output target, which StateBackup
        // does not hold, saved by hand (slot 0 only; slots 1-3 are left null). The pass sets the layout, the
        // topology, IA slot 0, VS / GS / PS / HS / DS and VS b0 / b1 - all in the capture.
        KhInjectionFlagScope khus_flag;
        g_ro.in_injection = true;
        StateBackup khus_sb;
        khus_sb.capture(khus_ctx);
        ID3D11Buffer* khus_so_old = nullptr;
        khus_ctx->SOGetTargets(1, &khus_so_old);
        const UINT khus_stride = static_cast<UINT>(sizeof(MeshVertex));
        khus_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        khus_ctx->PSSetShader(nullptr, nullptr, 0);
        khus_ctx->HSSetShader(nullptr, nullptr, 0);
        khus_ctx->DSSetShader(nullptr, nullptr, 0);
        bool khus_ok = true;
        {   // b1: the camera lanes; everything else zero (lighting1.w = 0 says so to a reader).
            ConstantData khus_cbf = {};
            if (g_ro.cycle_pv_valid) {
                // KH_FX_FRAME_ADOPT: the colour passes fill these lanes from their adopted view (the injection, the
                // flush, the UI lane), so the stage takes the same rotation. The once-a-cycle gate keys on the latch's
                // own rows (done_view), so no step runs it again for adoption alone: when the cycle's first step comes
                // before this frame's view is published, adoption declines and the stage keeps the latch's rotation
                // for the cycle.
                RVExtBridge::ProjectionViewTransform khus_pv = g_ro.cycle_pv;
                kh_adopt_frame_view(khus_pv);
                float khus_cam[3];
                kh_cam_of(khus_pv.view, khus_cam);
                kh_fill_user_cam_cb(khus_cbf, khus_cam, khus_pv.view);
            }
            khus_ok = kh_upload_frame_cb(khus_ctx, g_res.uvs_frame_cb, khus_cbf);
        }
        ID3D11Buffer* khus_cbs[2] = { g_res.uvs_obj_cb, g_res.uvs_frame_cb };
        khus_ctx->VSSetConstantBuffers(0, 2, khus_cbs);
        ID3D11VertexShader* khus_bound_vs = nullptr;
        for (size_t w = 0; khus_ok && w < khus_work.size(); ++w) {
            const uint32_t s = khus_work[w];
            KhUvsGpu& r = g_uvs[s];
            const RenderObject& o = g_scene.objs[s];
            const KhMaterialSet& khus_ms = *o.materials;
            // The source moved under the primed copy: the whole of it again, the stages over it. (The target bound
            // here, if any, is the previous object's buffer - never this one.)
            if (r.src_own) { kh_uvs_copy_src(khus_ctx, r); r.src_dirty = false; }   // KH_SKIN_POOL: by its slice.
            ConstantData khus_cbd = {};
            khus_cbd.center_size[0] = o.pos[0];
            khus_cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            khus_cbd.center_size[2] = o.pos[1];
            khus_cbd.size_axes[0] = o.size[0];    // SQF [x,y,z] sizes -> engine [x,z,y].
            khus_cbd.size_axes[1] = o.size[2];
            khus_cbd.size_axes[2] = o.size[1];
            kh_fill_obj_rot(khus_cbd, o.rot_m);
            memcpy(khus_cbd.color, o.color, sizeof(khus_cbd.color));
            kh_fill_user_obj_cb(khus_cbd, o);
            bool khus_drew = false;
            for (const KhUvsRun& khus_run : r.runs) {
                if (khus_run.sub >= khus_ms.slots.size()) continue;
                const KhMaterial& m = khus_ms.slots[khus_run.sub];
                if (!m.used || m.shader != 1 || !m.user_vertex || m.user_shader.empty()) continue;
                ID3D11VertexShader* khus_vs = nullptr;
                ID3D11GeometryShader* khus_gs = nullptr;
                ID3D11InputLayout* khus_il = nullptr;
                if (kh_user_mat_vs(khus_dev, m.user_shader, khus_vs, khus_gs, khus_il) != 1) continue;
                khus_cbd.shadow_meta2[3] = static_cast<float>(khus_run.first);   // SV_VertexID restarts at 0 with every draw: the run's first vertex.
                if (!kh_upload_obj_cb(khus_ctx, g_res.uvs_obj_cb, khus_cbd)) { khus_ok = false; break; }
                if (khus_vs != khus_bound_vs) {
                    khus_ctx->IASetInputLayout(khus_il);
                    khus_ctx->VSSetShader(khus_vs, nullptr, 0);
                    khus_ctx->GSSetShader(khus_gs, nullptr, 0);
                    khus_bound_vs = khus_vs;
                }
                const UINT khus_off = khus_run.first * khus_stride;
                khus_ctx->IASetVertexBuffers(0, 1, &r.src.vb, &khus_stride, &r.src.off);   // KH_SKIN_POOL.
                khus_ctx->SOSetTargets(1, &r.vb, &khus_off);
                if (g_orig_draw) g_orig_draw(khus_ctx, khus_run.count, khus_run.first); else khus_ctx->Draw(khus_run.count, khus_run.first);
                khus_drew = true;
            }
            r.step_cycle = g_topo_cycles;
            if (khus_drew || r.src_own) {
                r.key = kh_uvs_key_next();
                if (s < g_cloth_gen_slot.size()) g_cloth_gen_slot[s] = r.key;   // The key follows the bytes.
            }
            r.done = khus_ok;
            r.done_set = o.materials;
            r.done_src_key = r.src_key;
            kh_uvs_lanes_of(o, r.done_lanes);
            r.done_view_ok = g_ro.cycle_pv_valid;
            if (r.done_view_ok) memcpy(r.done_view, g_ro.cycle_pv.view, sizeof(r.done_view));
        }
        // The target off before anything binds a buffer as vertices; the engine's handed back (offset -1: it keeps appending).
        const UINT khus_append = static_cast<UINT>(-1);
        khus_ctx->SOSetTargets(1, &khus_so_old, &khus_append);
        if (khus_so_old) khus_so_old->Release();
        khus_sb.restore(khus_ctx);   // Under the flag.
    }   // The flag handed back last.
}
inline void kh_uvs_upload(ID3D11DeviceContext* khuu_ctx, ID3D11Device* khuu_dev) {
    if (!khuu_ctx || !khuu_dev) return;
    if (g_uvs.empty() && g_uvs_sets.load(std::memory_order_relaxed) == 0u) return;   // The session has made no vertex stage.
    kh_scene_sync();
    for (auto& khuu_kv : g_uvs) { khuu_kv.second.seen = false; khuu_kv.second.installed = false; }
    const uint32_t khuu_n = static_cast<uint32_t>(g_scene.objs.size());
    for (uint32_t s = 0; s < khuu_n; ++s) {
        if (!g_scene.alive[s]) continue;
        const RenderObject& o = g_scene.objs[s];
        if (!o.materials || !o.materials->vertex_any || o.fullscreen || o.effect != 0) continue;
        const int khuu_mid = mesh_id_clamp(o.mesh);
        if (khuu_mid < 0 || static_cast<size_t>(khuu_mid) >= g_res.mesh_vb.size() || !kh_mesh_alive(khuu_mid)) continue;
        const MeshDef& khuu_md = mesh_def(khuu_mid);
        const size_t khuu_nv = khuu_md.verts.size();
        if (khuu_nv == 0u || khuu_nv > 0xFFFFFFFFull / sizeof(MeshVertex)) continue;
        const UINT khuu_bytes = static_cast<UINT>(khuu_nv * sizeof(MeshVertex));
        KhUvsGpu& r = g_uvs[s];
        if (r.seq != o.seq || r.mesh != khuu_mid || r.bytes != khuu_bytes) {   // Another object in the slot, or another mesh: start over.
            KH_SAFE_RELEASE(r.vb);
            r = KhUvsGpu();
            r.seq = o.seq; r.mesh = khuu_mid; r.bytes = khuu_bytes;
        }
        r.seen = true;
        if (r.failed) continue;
        // Every vertex stage of the set has landed, and one at least is ready.
        bool khuu_pend = false, khuu_any = false;
        for (const KhMaterial& m : o.materials->slots) {
            if (!m.used || m.shader != 1 || !m.user_vertex || m.user_shader.empty()) continue;
            ID3D11VertexShader* khuu_vs = nullptr;
            ID3D11GeometryShader* khuu_gs = nullptr;
            ID3D11InputLayout* khuu_il = nullptr;
            const int khuu_st = kh_user_mat_vs(khuu_dev, m.user_shader, khuu_vs, khuu_gs, khuu_il);
            if (khuu_st == 0) khuu_pend = true;
            else if (khuu_st == 1) khuu_any = true;
        }
        if (khuu_pend || !khuu_any) {
            // Not installed this flush, so nothing evaluates it: what the last evaluation stood for is forgotten.
            // done_set is a WEAK pointer, and this is the one window in which the set it names can be released while
            // the record lives on (the object holds ANOTHER set, whose stage is pending or failed) - a later set
            // allocated at that address would otherwise pass both gates of kh_uvs_step for the old one.
            r.done = false;
            r.src = KhVbRef();   // KH_UVS_SRC_FORGET: src is weak too (below).
            continue;
        }
        // What the cloth's and the skin's uploads left in the slot, else the mesh's own (KH_SKIN_POOL: the pair).
        // KH_UVS_SRC_FORGET: a record not installed this flush forgets the source it was primed from - src is a weak
        // pair, and a buffer made later at a released one's address (and offset) would pass for it, skipping the
        // prime (stale input; and src_whole naming the wrong kind of copy). The next install primes again.
        const KhVbRef khuu_src = kh_mesh_vb_ref(khuu_mid, s);
        if (!khuu_src) { r.src = KhVbRef(); continue; }
        if (!r.vb) {
            D3D11_BUFFER_DESC khuu_bd = {};
            khuu_bd.ByteWidth = khuu_bytes;
            khuu_bd.Usage = D3D11_USAGE_DEFAULT;
            khuu_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
            if (FAILED(khuu_dev->CreateBuffer(&khuu_bd, nullptr, &r.vb))) {
                r.vb = nullptr;
                r.failed = true;
                report_error_once_safe("KH RenderIntegration: user vertex stage: a deformed vertex buffer could not "
                                       "be made (the mesh draws undeformed)");
                continue;
            }
            r.src = KhVbRef();
        }
        if (r.src != khuu_src) {   // Prime: the runs no stage owns draw from this copy.
            // Not this mesh's vertex array: the source draws. KH_SKIN_POOL: a chunk's slice is one when the chunk's
            // slices are this size (its pool's key).
            const KhSkinPoolChunk* const khuu_pc = kh_mesh_pool_of(khuu_mid, s);
            if (khuu_pc) {
                if (static_cast<UINT>(khuu_pc->key & 0xFFFFFFFFull) != khuu_bytes) { r.src = KhVbRef(); continue; }
            } else {
                D3D11_BUFFER_DESC khuu_sd = {};
                khuu_src.vb->GetDesc(&khuu_sd);
                if (khuu_sd.ByteWidth != khuu_bytes || khuu_src.off != 0u) { r.src = KhVbRef(); continue; }
            }
            r.src = khuu_src;
            r.src_whole = khuu_pc == nullptr;
            kh_uvs_copy_src(khuu_ctx, r);
            r.src_dirty = true;   // Another source: whatever was evaluated this cycle was evaluated over the old one.
            r.done = false;
            r.step_cycle = ~0ull;   // KH_USER_VS_REPRIME: the copy voids this cycle's evaluation; the step redoes it.
        }
        r.src_own = khuu_src.vb != g_res.mesh_vb[khuu_mid];
        r.src_key = kh_cloth_gen_of(s);
        if (r.runs.empty()) kh_uvs_runs_build(khuu_md, r.runs);
        if (r.key == 0u) r.key = kh_uvs_key_next();
        if (g_cloth_vb_slot.size() <= s) g_cloth_vb_slot.resize(s + 1u, nullptr);
        if (g_cloth_gen_slot.size() <= s) g_cloth_gen_slot.resize(s + 1u, 0u);
        if (g_cloth_mesh_slot.size() <= s) g_cloth_mesh_slot.resize(s + 1u, -1);
        if (g_cloth_sim_slot.size() <= s) g_cloth_sim_slot.resize(s + 1u, 0u);   // KH_LOD_SIM_SLOT: the source's.
        if (g_cloth_voff_slot.size() <= s) g_cloth_voff_slot.resize(s + 1u, 0u);   // KH_SKIN_POOL.
        if (g_cloth_pool_slot.size() <= s) g_cloth_pool_slot.resize(s + 1u, nullptr);
        g_cloth_vb_slot[s] = r.vb;
        g_cloth_voff_slot[s] = 0u;   // KH_SKIN_POOL: the stage's buffer is its own.
        g_cloth_pool_slot[s] = nullptr;
        g_cloth_gen_slot[s] = r.key;
        g_cloth_mesh_slot[s] = khuu_mid;
        r.installed = true;
    }
    for (auto khuu_it = g_uvs.begin(); khuu_it != g_uvs.end();) {   // An object that left, or lost its stage.
        if (!khuu_it->second.seen) { KH_SAFE_RELEASE(khuu_it->second.vb); khuu_it = g_uvs.erase(khuu_it); }
        else ++khuu_it;
    }
    kh_uvs_step(khuu_ctx);   // A buffer installed just now is deformed before its first draw.
}
}