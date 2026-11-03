#pragma once

// The injection half (rendering_integration_injection.hpp): VOL_REPLAY's shared state: the merge, the rasterizer and
// pass scopes, the mirror targets. Never include it on its own.
namespace RenderIntegration {

// REPLAY_SCISSOR: a rectangle the replay draws in - the whole target (full), or px (empty when !any).
struct RpRect {
    bool full = true;
    bool any = false;
    float x0 = 1.0e9f, y0 = 1.0e9f, x1 = -1.0e9f, y1 = -1.0e9f;   // NDC, while it is being built.
    D3D11_RECT px = {};
};
// VOL_REPLAY's shared state: the merge, the rasterizer and pass scopes, the mirror targets.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: rp_globals_reset resets 17 of them. session_scratch_reset resets 15 of them.
struct ReplayState {
    // VOL_REPLAY - the stencil our meshes read. The engine counts its stencil volumes for a frame at the end of the
    // cycle before (the volume pass), against a footprint of our meshes the seam draws into its buffer - at the pose it
    // has there, a frame old. The pass is recorded (every draw on the volume buffer, its whole state, every buffer the
    // engine can still rewrite copied as the draw used it) and replayed at the injection against the engine's own depth
    // plus our footprint at this cycle's pose (replay B). Our meshes read that count (t24) and its footprint distance
    // (t33, the witness) instead of the copy. SEAM_WITHHOLD: while they do, the seams write no footprint into the
    // engine's buffer - the engine resolves its own world and hands shadow from that count, and a footprint a frame old
    // shades the pixels our meshes have just left (the trailing ghost). A cycle the replay cannot serve reads the copy,
    // and the seams that follow write the footprint.
    bool     merge_ok = false;
    uint64_t merge_cyc = ~0ull;                       // The cycle the merged textures are for.
    uint64_t replay_cyc = ~0ull;                      // REPLAY_ONCE: the cycle the pass was replayed in.
    ID3D11ShaderResourceView* msten_srv = nullptr;   // Merged count (R8G8_UINT, .g - as the copy's view reads).
    ID3D11ShaderResourceView* mfoot_srv = nullptr;   // Merged footprint distance (R32_FLOAT, m).
    bool     seam_b = false;                          // A seam is drawing replay B's footprint.
    uint64_t withheld_seq = ~0ull;                    // The frame whose world seam withheld our footprint.
    // The rectangles the replay of the merged cycle drew in (the volume count; the mirror, MIR_REPLAY) and the scene
    // generation (g_scene.gen) they were computed from. The merged textures hold that cycle's values only inside them.
    RpRect sc_all, sc_mir;
    uint64_t sc_gen = ~0ull;
    // REPLAY_RASTER: the raster the merged cycle's replay drew in when it is not the cycle latch's. The recorded
    // pass drew under the copy's projection (g_svs.vol_proj); replay B's footprint takes the latch's, and the two can
    // part by a whole zoom step. Then B draws its footprint under the copy's projection terms instead (the latch's
    // view, depth pair and w column kept), so every count B holds is in the copy's raster - where our meshes look it up
    // already (fill_sten_reproj maps their projection onto the copy's) - and the rectangles and the coverage tests
    // bound our meshes in that raster too (rp_obj_ndc). Off (the latch's own raster, bit for bit) whenever the two
    // agree within rp_b_eligible's tolerance. Set by rp_replay, for its cycle.
    bool  rast_on = false;
    float rast[4] = {};   // P00, P11, P20, P21 over P23 (sten_proj_terms).
    // The pass being drawn may read the merged count / the merged mirror: set for a pass's extent by RpPassScope,
    // from what that pass draws (rp_covered; the in-front slice: the rectangles' full flags); outside a scope
    // nothing reads them. A pass drawing an object the rectangle was not made for (the flush re-syncs the scene: a mesh
    // moved, shown or added since the replay) reads the copy and the real mirror - a cycle the replay did not serve,
    // for that pass.
    bool pass_vol = false, pass_mir = false;
    // MIR_REPLAY - the mirror stencil (t28), replayed as the volume count is (VOL_REPLAY). The mirror is a count
    // taken against our own surfaces alone: the world seam's prepass draws every world caster into it (translucent ones
    // included) and every inFront one but a whole-object translucent (the seam's list leaves those out), and the
    // engine's counting draws are re-issued there with the near-patched view block. It answers the stencil for
    // translucent texels (mirMeta.x = 1), for the whole of an inFront mesh (mirMeta.x = 2) and for PSComposite's
    // near-plane fade - at the seam's pose, a frame old, as the volume copy does. On a cycle whose volume count merges,
    // replay B's world seam draws the prepass again at this cycle's pose into mirror B, the pass's re-issued draws are
    // replayed into it and into the real mirror's depth with its count cleared (mirror A, the trust), and the merge
    // takes B where A equals the real mirror, else the real mirror. Every t28 bind takes rp_mir_srv().
    bool     mir_ok = false;                          // The merged mirror is g_rp.mir_cyc's.
    uint64_t mir_cyc = ~0ull;
    bool     mir_want = false;                        // Replay B's world seam draws mirror B's prepass...
    bool     mirb_drawn = false;                      // ...and it did.
    uint32_t mir_w = 0, mir_h = 0;
    // On demand: the replay draws mirror B's prepass and arms the rest (g_rp.mir_pend, for that cycle and recorded
    // pass); rp_mir_finish runs it for the first pass with a mesh that reads the mirror (rp_mir_reads, that
    // pass's near).
    bool     mir_pend = false;
    uint64_t mir_pend_cyc = ~0ull, mir_pend_pass = ~0ull;
    ID3D11Texture2D*          mira_tex = nullptr;     // Mirror A.
    ID3D11DepthStencilView*   mira_dsv = nullptr;
    ID3D11ShaderResourceView* mira_srv = nullptr;
    ID3D11Texture2D*          mirb_tex = nullptr;     // Mirror B.
    ID3D11DepthStencilView*   mirb_dsv = nullptr;
    ID3D11ShaderResourceView* mirb_srv = nullptr;
    ID3D11Texture2D*          mirm_tex = nullptr;     // The merged mirror (R8G8_UINT, the count in .g).
    ID3D11RenderTargetView*   mirm_rtv = nullptr;
    ID3D11ShaderResourceView* mirm_srv = nullptr;
};
static ReplayState g_rp;
inline bool rp_merged_now() {
    return g_rp.merge_ok && g_rp.merge_cyc == g_topo_cycles && g_rp.msten_srv && g_rp.mfoot_srv;
}
// REPLAY_RASTER: a projection with its screen terms made the replay's raster's (P23 kept, so the terms land as
// the copy's exactly as sten_proj_terms reads them). A no-op while the raster is the latch's.
inline void rp_rast_apply(float p[4][4]) {
    if (!g_rp.rast_on) return;
    const float w = p[2][3];
    p[0][0] = g_rp.rast[0] * w;
    p[1][1] = g_rp.rast[1] * w;
    p[2][0] = g_rp.rast[2] * w;
    p[2][1] = g_rp.rast[3] * w;
}
struct RpPassScope {
    bool sv_vol, sv_mir;
    RpPassScope(bool vol, bool mir) : sv_vol(g_rp.pass_vol), sv_mir(g_rp.pass_mir) {
        g_rp.pass_vol = vol;
        g_rp.pass_mir = mir;
    }
    ~RpPassScope() { g_rp.pass_vol = sv_vol; g_rp.pass_mir = sv_mir; }
    RpPassScope(const RpPassScope&) = delete;
    RpPassScope& operator=(const RpPassScope&) = delete;
};
// The merged count and footprint distance, for the pass being drawn.
inline bool rp_use_merged() { return g_rp.pass_vol && rp_merged_now(); }
// SEAM_WITHHOLD: this cycle's injection merged, so the seams that follow (for the next frame's count) write no
// footprint; a cycle that did not merge writes it, so its fallback copy is the ordinary copy.
inline bool rp_seam_withhold() { return !g_rp.seam_b && rp_merged_now(); }
// VOL_FOOT: the copy's footprint mask, or nullptr - the one test the fill's
// arm and both binds (the injection, the flush) share.
inline ID3D11ShaderResourceView* svs_foot_bound() {
    if (!g_svs.vol_foot_ok || g_svs.foot_pub < 0 || g_svs.foot_pub > 1) return nullptr;
    return g_svs.foot_srv[g_svs.foot_pub];
}

inline void rp_mir_release() {
    KH_SAFE_RELEASE(g_rp.mira_srv); KH_SAFE_RELEASE(g_rp.mira_dsv); KH_SAFE_RELEASE(g_rp.mira_tex);
    KH_SAFE_RELEASE(g_rp.mirb_srv); KH_SAFE_RELEASE(g_rp.mirb_dsv); KH_SAFE_RELEASE(g_rp.mirb_tex);
    KH_SAFE_RELEASE(g_rp.mirm_srv); KH_SAFE_RELEASE(g_rp.mirm_rtv); KH_SAFE_RELEASE(g_rp.mirm_tex);
    g_rp.mir_w = g_rp.mir_h = 0;
    g_rp.mir_ok = false;
}
inline bool rp_mir_merged_now() {
    return g_rp.mir_ok && g_rp.mir_cyc == g_topo_cycles && g_rp.mirm_srv && rp_merged_now();
}
// The mirror every t28 bind takes: this cycle's merge when the pass may read it (RpPassScope), else the real mirror.
inline ID3D11ShaderResourceView* rp_mir_srv() {
    return (g_rp.pass_mir && rp_mir_merged_now()) ? g_rp.mirm_srv : g_vmir.srv;
}
}