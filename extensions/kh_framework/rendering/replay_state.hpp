#pragma once

// The injection half (rendering_integration_injection.hpp): KH_VOL_REPLAY's shared state: the merge, the rasterizer and
// pass scopes, the mirror targets. Never include it on its own.
namespace RenderIntegration {

// KH_VOL_REPLAY - the stencil our meshes read. The engine counts its stencil volumes for a frame at the end of the
// cycle before (the volume pass), against a footprint of our meshes the seam draws into its buffer - at the pose it
// has there, a frame old. The pass is recorded (every draw on the volume buffer, its whole state, every buffer the
// engine can still rewrite copied as the draw used it) and replayed at the injection against the engine's own depth
// plus our footprint at this cycle's pose (replay B). Our meshes read that count (t24) and its footprint distance
// (t33, the witness) instead of the copy. KH_SEAM_WITHHOLD: while they do, the seams write no footprint into the
// engine's buffer - the engine resolves its own world and hands shadow from that count, and a footprint a frame old
// shades the pixels our meshes have just left (the trailing ghost). A cycle the replay cannot serve reads the copy,
// and the seams that follow write the footprint.
static bool     g_rp_merge_ok = false;
static uint64_t g_rp_merge_cyc = ~0ull;                       // The cycle the merged textures are for.
static uint64_t g_rp_replay_cyc = ~0ull;                      // KH_REPLAY_ONCE: the cycle the pass was replayed in.
static ID3D11ShaderResourceView* g_rp_msten_srv = nullptr;   // Merged count (R8G8_UINT, .g - as the copy's view reads).
static ID3D11ShaderResourceView* g_rp_mfoot_srv = nullptr;   // Merged footprint distance (R32_FLOAT, m).
static bool     g_rp_seam_b = false;                          // A seam is drawing replay B's footprint.
static uint64_t g_rp_withheld_seq = ~0ull;                    // The frame whose world seam withheld our footprint.
inline bool kh_rp_merged_now() {
    return g_rp_merge_ok && g_rp_merge_cyc == g_topo_cycles && g_rp_msten_srv && g_rp_mfoot_srv;
}
// KH_REPLAY_SCISSOR: a rectangle the replay draws in - the whole target (full), or px (empty when !any).
struct KhRpRect {
    bool full = true;
    bool any = false;
    float x0 = 1.0e9f, y0 = 1.0e9f, x1 = -1.0e9f, y1 = -1.0e9f;   // NDC, while it is being built.
    D3D11_RECT px = {};
};
// The rectangles the replay of the merged cycle drew in (the volume count; the mirror, KH_MIR_REPLAY) and the scene
// generation (g_scene.gen) they were computed from. The merged textures hold that cycle's values only inside them.
static KhRpRect g_rp_sc_all, g_rp_sc_mir;
static uint64_t g_rp_sc_gen = ~0ull;
// KH_REPLAY_RASTER: the raster the merged cycle's replay drew in when it is not the cycle latch's. The recorded pass
// drew under the copy's projection (g_svs_vol_proj); replay B's footprint takes the latch's, and the two can part
// by a whole zoom step. Then B draws its footprint under the copy's projection terms instead (the latch's view,
// depth pair and w column kept), so every count B holds is in the copy's raster - where our meshes look it up
// already (kh_fill_sten_reproj maps their projection onto the copy's) - and the rectangles and the coverage tests
// bound our meshes in that raster too (kh_rp_obj_ndc). Off (the latch's own raster, bit for bit) whenever
// the two agree within kh_rp_b_eligible's tolerance. Set by kh_rp_replay, for its cycle.
static bool  g_rp_rast_on = false;
static float g_rp_rast[4] = {};   // P00, P11, P20, P21 over P23 (kh_sten_proj_terms).
// KH_REPLAY_RASTER: a projection with its screen terms made the replay's raster's (P23 kept, so the terms land as
// the copy's exactly as kh_sten_proj_terms reads them). A no-op while the raster is the latch's.
inline void kh_rp_rast_apply(float khra_p[4][4]) {
    if (!g_rp_rast_on) return;
    const float khra_w = khra_p[2][3];
    khra_p[0][0] = g_rp_rast[0] * khra_w;
    khra_p[1][1] = g_rp_rast[1] * khra_w;
    khra_p[2][0] = g_rp_rast[2] * khra_w;
    khra_p[2][1] = g_rp_rast[3] * khra_w;
}
// The pass being drawn may read the merged count / the merged mirror: set for a pass's extent by KhRpPassScope, from
// what that pass draws (kh_rp_covered; the in-front slice: the rectangles' full flags); outside a scope nothing reads
// them. A pass drawing an object the rectangle was not made for (the flush re-syncs the scene: a mesh moved, shown or
// added since the replay) reads the copy and the real mirror - a cycle the replay did not serve, for that pass.
static bool g_rp_pass_vol = false, g_rp_pass_mir = false;
struct KhRpPassScope {
    bool sv_vol, sv_mir;
    KhRpPassScope(bool khps_vol, bool khps_mir) : sv_vol(g_rp_pass_vol), sv_mir(g_rp_pass_mir) {
        g_rp_pass_vol = khps_vol;
        g_rp_pass_mir = khps_mir;
    }
    ~KhRpPassScope() { g_rp_pass_vol = sv_vol; g_rp_pass_mir = sv_mir; }
    KhRpPassScope(const KhRpPassScope&) = delete;
    KhRpPassScope& operator=(const KhRpPassScope&) = delete;
};
// The merged count and footprint distance, for the pass being drawn.
inline bool kh_rp_use_merged() { return g_rp_pass_vol && kh_rp_merged_now(); }
// KH_SEAM_WITHHOLD: this cycle's injection merged, so the seams that follow (for the next frame's count) write no
// footprint; a cycle that did not merge writes it, so its fallback copy is the ordinary copy.
inline bool kh_rp_seam_withhold() { return !g_rp_seam_b && kh_rp_merged_now(); }
// KH_VOL_FOOT: the copy's footprint mask, or nullptr - the one test the fill's
// arm and both binds (the injection, the flush) share.
inline ID3D11ShaderResourceView* kh_svs_foot_bound() {
    if (!g_svs_vol_foot_ok || g_svs_foot_pub < 0 || g_svs_foot_pub > 1) return nullptr;
    return g_svs_foot_srv[g_svs_foot_pub];
}

// KH_MIR_REPLAY - the mirror stencil (t28), replayed as the volume count is (KH_VOL_REPLAY). The mirror is a count
// taken against our own surfaces alone: the world seam's prepass draws every world caster into it (translucent ones
// included) and every inFront one but a whole-object translucent (the seam's list leaves those out), and the engine's
// counting draws are re-issued there with the near-patched view block. It answers the stencil for translucent texels
// (mirMeta.x = 1), for the whole of an inFront mesh (mirMeta.x = 2) and for PSComposite's near-plane fade - at the
// seam's pose, a frame old, as the volume copy does. On a cycle whose volume count merges, replay B's world seam draws
// the prepass again at this cycle's pose into mirror B, the pass's re-issued draws are replayed into it and into the
// real mirror's depth with its count cleared (mirror A, the trust), and the merge takes B where A equals the real
// mirror, else the real mirror. Every t28 bind takes kh_rp_mir_srv().
static bool     g_rp_mir_ok = false;                          // The merged mirror is g_rp_mir_cyc's.
static uint64_t g_rp_mir_cyc = ~0ull;
static bool     g_rp_mir_want = false;                        // Replay B's world seam draws mirror B's prepass...
static bool     g_rp_mirb_drawn = false;                      // ...and it did.
static uint32_t g_rp_mir_w = 0, g_rp_mir_h = 0;
// On demand: the replay draws mirror B's prepass and arms the rest (g_rp_mir_pend, for that cycle and recorded pass);
// kh_rp_mir_finish runs it for the first pass with a mesh that reads the mirror (kh_rp_mir_reads, that pass's near).
static bool     g_rp_mir_pend = false;
static uint64_t g_rp_mir_pend_cyc = ~0ull, g_rp_mir_pend_pass = ~0ull;
static ID3D11Texture2D*          g_rp_mira_tex = nullptr;     // Mirror A.
static ID3D11DepthStencilView*   g_rp_mira_dsv = nullptr;
static ID3D11ShaderResourceView* g_rp_mira_srv = nullptr;
static ID3D11Texture2D*          g_rp_mirb_tex = nullptr;     // Mirror B.
static ID3D11DepthStencilView*   g_rp_mirb_dsv = nullptr;
static ID3D11ShaderResourceView* g_rp_mirb_srv = nullptr;
static ID3D11Texture2D*          g_rp_mirm_tex = nullptr;     // The merged mirror (R8G8_UINT, the count in .g).
static ID3D11RenderTargetView*   g_rp_mirm_rtv = nullptr;
static ID3D11ShaderResourceView* g_rp_mirm_srv = nullptr;
inline void kh_rp_mir_release() {
    KH_SAFE_RELEASE(g_rp_mira_srv); KH_SAFE_RELEASE(g_rp_mira_dsv); KH_SAFE_RELEASE(g_rp_mira_tex);
    KH_SAFE_RELEASE(g_rp_mirb_srv); KH_SAFE_RELEASE(g_rp_mirb_dsv); KH_SAFE_RELEASE(g_rp_mirb_tex);
    KH_SAFE_RELEASE(g_rp_mirm_srv); KH_SAFE_RELEASE(g_rp_mirm_rtv); KH_SAFE_RELEASE(g_rp_mirm_tex);
    g_rp_mir_w = g_rp_mir_h = 0;
    g_rp_mir_ok = false;
}
inline bool kh_rp_mir_merged_now() {
    return g_rp_mir_ok && g_rp_mir_cyc == g_topo_cycles && g_rp_mirm_srv && kh_rp_merged_now();
}
// The mirror every t28 bind takes: this cycle's merge when the pass may read it (KhRpPassScope), else the real mirror.
inline ID3D11ShaderResourceView* kh_rp_mir_srv() {
    return (g_rp_pass_mir && kh_rp_mir_merged_now()) ? g_rp_mirm_srv : g_vmir_srv;
}
}