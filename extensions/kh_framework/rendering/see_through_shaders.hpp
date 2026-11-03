#pragma once

// The injection half (rendering_integration_injection.hpp): KH_WSEE's particle-shader variants (KH_PSV / KH_PSB): their
// state, the DXBC patching, capture and creation. Never include it on its own.
namespace RenderIntegration {

// KH_WSEE - the see-through record: the effect chain and our sorted translucents see through the world's blended draws
// (fences, foliage, glass, particles) to the surface behind. Built in (KH_WSEE_ON; rendering/see_through_state.hpp's
// KH_WSEE_BUILD 0 builds it out: no capture, no hook work beyond early-out tests); no script switch, every part of it
// always on. Threads: the pre-draw, the re-issues, the cycle's verdict, the back-off and the releases are the render
// thread's; our tail is the flush's (render thread, or a park that stalls it); the chain may be the game thread's late
// one; the want / landed / tainted / used / colour stamps are atomics.
//
// WHY. A blended draw (DestBlend INV_SRC_ALPHA) leaves the scene's colour behind it in RT0 and either its own depth
// (fence-type: it writes depth) or none (particle-type). Every depth reader of the chain then takes a faint wire for a
// wall, and the distance fog paints the sky's fog over a particle.
//
// WANTED CYCLE (g_wsee_on, kh_wsee_cycle_note at the main depth's clear): KH_WSEE_ON, a want stamped by a chain run
// that would arm a pass (kh_fx_chain_run, kh_wsee_arm) at most KH_WSEE_WANT_SPAN (2) cycles ago, no back-off, no
// KH_WSEE_FLUSHW_PRED stand-down. KH_WSEE_WASTE_RUN (8) tainted or refused attempts in a row stand the capture down
// KH_WSEE_BACKOFF (120) cycles (the KH_WSEE_FLUSHW_PRED hold where our flush's depth write ended the run).
// KH_WSEE_IDLE_FREE (600) cycles without a want release every target (KH_VMSEE_LAZY: the next capture remakes them). A
// refused target make is latched at its key and asked again KH_WSEE_HOLD_FAIL_RETRY cycles later (KH_WSEE_FAIL_RETRY).
//
// QUALIFYING DRAW (kh_wsee_pre_draw, reorder_pre_draw's call): an engine draw on the tracked context whose depth state
// writes or tests without writing (the DSS hook's dss_write / dss_nowrite), RT0 = the scene texture, blend ADD with
// DestBlend INV_SRC_ALPHA, SrcBlend ONE / SRC_ALPHA / ZERO, colour written, sample mask != 0, no alpha-to-coverage
// (an A2C draw writes depth at its covered samples alone). Light-adding (DestBlend ONE, a source factor reading no
// dest) and test-off over-blends count separately (below). Any other draw that really writes depth after the capture
// (unblended, or blended but not qualifying, alpha-to-coverage included) is a late opaque (RE_OPAQUE). Draws below the
// world's range (the view-model slice, our in-front meshes) are never tracked. A tainted cycle reads no further draw's
// state (KH_WSEE_TAINT_EARLY).
//
// CAPTURE (kh_wsee_snapshot; once a cycle) at the cycle's first qualifying draw once the world pass has begun
// (kh_reorder_world_vp, kh_reorder_min_opaques) and no injection can still come (attempts spent, no eligible mesh,
// anomalous cycle, composite path dead: kh_wsee_pre_draw's gate); else at the injection's translucent tail
// (kh_wsee_inj_capture), S and B then holding our solids. Targets (the main depth's size unless said; made at the first
// capture that needs them, remade on a size / sample-count / quality / format change; Resources' notes name each):
//   S        R32F 1x: sample 0 of the main depth, raw (PSDepthS0).
//   B        scene RT0 resolved (its view format), only with a colour want (KH_WSEE_BWANT: g_wsee_col_want_cycle within
//            WANT_SPAN; the distance fog, a user effect that names the record, the sort's modes 2 / 3); else left stale
//            and not bound.
//   T        RGBA8 at the depth's sample count, cleared (1, 0, 1, 1): R Tw (fence-type transmittance), G late-opaque
//            flag (0 / 1), B Tpre, A Tpost (particle-type, before / after B2).
//   NW / NW2 R32 typeless, D32 DSV + R32F SRV, 1x, cleared 1: the nearest particle-type depth, pre / post.
//   Tp       RGBA8 MS, cleared 1: particle-type again under a 25 % near push (R pre, G post) and a 100 % push (B / A).
//   Tf       R8G8 1x over its own D32 cleared to D's raw (D = far / 4, kh_wsee_far_raw): fragments beyond D, pre /
//            post.
//   D3 / D3m D3 R32F 1x (SRV, UAV u3, RTV): a late opaque's own sample-0 depth; D3m R32F MS: the post replays' z.
//   P0 / P1  RGBA8, N / N2 R32F, at ceil(size / 2) (KH_PSB_LS = 1): the one pass's record, pre / post pair.
//   Zb       R32G32F at P's size (PSPsbZb right after S): the block's largest S and which block pixel holds it.
//   E        R32_UINT frame size + a D24S8 stencil: the chain's class word (drawn once a run, kh_wsee_agree_draw).
//   C        Texture2DMS view of the scene the chain captures from (its sample colours).
//   Q        B's size / format: the scene resolved before the flush's first compositing part.
//   hold VB  the deferred parts' geometry.
// B2 MOMENT (kh_wsee_b2_take; once a cycle, at the first fence-type draw after the capture, before it): S2 = sample 0
// raw then (R32F 1x; drawn where B2 is takeable or a pre late opaque was marked), B2 = the scene resolved then (only in
// a cycle whose capture resolved B). Particle-type layers before this moment are "pre", after it "post".
//
// RE-ISSUES (the draw hooks' post, kh_wsee_post: the engine's draw again right after it, under its own predicate on
// purpose - KH_PRED_ASIDE; one kh_wsee_re_save / kh_wsee_re_restore pair a post). Kind by the pre-draw's mark (each
// pass's targets, states and blends: the PASS_* table, "The re-issues' passes", TWIN of kh_wsee_re_set's binds):
//   RE_BLEND (fence-type)    PASS_T_EQ: T.R *= 1 - a under EQUAL (no write), the engine's sample mask: exactly the
//                            samples it wrote, stencil or discard included.
//   RE_NOWRITE (particle)    the one pass (KH_PSB) where its variant exists, else PASS_T (T.B or .A), PASS_NW (NW /
//                            NW2), PASS_TP / TP2 (Tp R|G / B|A), PASS_TF (Tf beyond D). T, NW, Tp, Tp2 take
//                            KhWseeSurfVp's MinDepth push (TOL 0.02 / 0.2 / 0.5 of the range): a fragment within 2 % of
//                            the surface behind it is the surface's, within 25 % / 100 % in T but not Tp / Tp2.
//   RE_NOWRITE_OFF           the same with dss_off on T / Tp / Tp2 (the engine drew it with the depth test off).
//   RE_NWONLY (light-adding) PASS_NW alone: its light is in S beyond T B, its depth is a front, T unchanged.
//   RE_OPAQUE, post only     PASS_ONE under EQUAL: T = (1, 1, 1, 1), Tp = 1 (KH_PSB_TPA's rule), D3m = its own sample-0
//                            z (KH_WSEE_PXZ; else the store path into D3 through u3, D3m written -1 -
//                            kh_wsee_d3m_path). A pre late opaque (marked before the B2 attempt) is NOT replayed
//                            (KH_WSEE_NOPRE): the shaders derive it (sample 0 is pre where S2 != S in the world's
//                            range; lane bits 8 / 9). Past KH_WSEE_OPAQUE_CAP (4096) replays the cycle is tainted.
// Particle-type shaders (KH_PSV, KH_PSB): the variants "alpha", "nw" and "one", patched from the engine's PS bytecode
// copied at CreatePixelShader (a process-wide MinHook detour; KH_PSV_TOMB: this DLL must never unload), made at the
// clear (kh_psv_cycle_head, <= KH_PSV_MAKE_N a clear: a one-time driver-compile hitch per shader per session and after
// each device reset). Refused or absent: the engine's own shader draws today's passes (KH_PSV_REFUSE_REPORT).
// THE ONE PASS (KH_PSB, kh_psb_post): one draw, single sample, no depth view, into its phase's pair (P0 / N or P1 / N2)
// at 1 / 2 of the frame. The variant's prologue makes the five passes' verdicts against the largest sample 0 of the
// block's 2 x 2 pixels (bounded by Zb until kh_psb_zb_trip), discards a fragment that writes nothing, and writes o0 =
// (aT, aP1, aP2, aF), o1.x = z_1 under ZERO / INV_SRC_COLOR (N: MIN). Not taken (today's passes): kh_psb_post's
// refusals, and our own tail.
//
// OUR TAIL (kh_wsee_tail): a normal-blend part that tests without writing (the injection's, or the flush's) goes into T
// and NW (and Tp / Tp2 / Tf) as RE_NOWRITE's passes do, with PSMain / PSComposite's KH_PSV tail variants.
// SORT (kh_wsee_sort_mode, KhWseeSortScope, cb.hlsl KhWseeMeshCut / KhWseeMeshOut): the injection defers its
// translucent parts of a landed, untainted cycle to the flush's tail (g_wsee_defer_objs, KH_WSEE_DEFER_FLAG; a deformed
// part's vertex runs copied into the hold VB, a static part's entry naming the mesh's own buffer - KH_WSEE_OWN_STATIC;
// a lost or tainted deferral backs off KH_WSEE_DEFER_BACKOFF cycles - KH_WSEE_DEFER_TAINT), where they draw far to near
// ahead of the flush's own meshes, with no hardware depth test: mode 1 = the shader's occlusion test against the
// record, plain blend; 2 / 3 = that and the layer-aware dual-source composite (3: a non-float target, inputs held to
// [0, 1]); 0 = the hardware test (the capture not owed, a missing view, or a partitioned cycle: the engine's latched
// pair's near differs from the flush's by > 2e-3, KH_WSEE_MS_REL).
//
// THE CHAIN (kh_fx_chain_run; cb.hlsl KhWseeRule / KhWseeMix): a run whose cycle's capture is valid (kh_wsee_valid:
// landed, untainted, this depth) binds the record and arms each pass that uses it (fx_tex[1].z = 1, kh_wsee_mode_of;
// not an unmasked Outline nor a user effect reading the depth alone); without a valid capture nothing is bound
// (KH_WSEE_BIND_ARMED). Per pixel: front = min(r, NWpre, NWpost), behind = G ? D3 : S (S2 for a derived pre); covered
// where front >= depthParams.z and behind > front; the pass blends what it derives from depth by the layers'
// transmittance, front to back by layer group (fence-type at r, pre / post at their NW, the near-band and far parts at
// the band's middle / at D). Per sample where the samples disagree (E: up to four classes a pixel; a sited pass is
// drawn twice under E's stencil, ref 0 / 1 - kh_wsee_split_draw). Uncovered pixels, and every pass of a run without a
// valid capture (mode 0, nulls bound): the pre-WSEE picture.
// D3 is resolved from D3m once before a reader binds it (kh_wsee_d3m_resolve, PSWseeD3Res; in_injection only on the
// render thread - KH_WSEE_D3M_RT): D3 = r where the opaque's z lies within KH_WSEE_D3_TOL (8 D24 steps) of the live
// sample 0, else z.
//
// REGISTERS (the FX unit; StateBackup saves t0 - t69): t53 S, t54 T, t55 B, t56 R (the chain's input), t57 NWpre,
//  t58 E, t59 C, t60 B2, t61 NWpost, t62 S2, t63 Tp, t64 Tf, t65 D3, t66 P0, t67 P1, t68 N, t69 N2.
// The mesh unit (the sort scope): t53 S, t54 T, t55 B, t56 the live depth (MS), t57 NWpre, t58 Q, t59 NWpost, t60 B2,
//  t62 S2, t65 D3, t66 - t69 P0 / P1 / N / N2. depth_resolve: u3 D3 (store path); PSWseeOneZ* targets T, Tp, D3m.
// LANES (C++ fx_tex[1][2..3] = HLSL khFxTexLay[1].zw): .z = 1 armed (an effect pass) / the sort mode (a mesh draw);
//  .w bits: 0 E bound, 1 C bound, 2 per-sample path allowed, 3 the B2 moment stood (S2 bound; B2 too where a colour
//  want resolved it), 4 Tp / Tf, 5 D3, 6 the one pass's record drew this cycle, 7 the old record holds no particle
//  layer, 8 a pre late opaque was skipped (derived), 9 S2 bound as the derivation's reference. Bits 10 - 11 retired
//  (s fixed). fx_fog[0] on a sorted draw = the depth decode (m22, m32, the world's MinDepth / MaxDepth).
// TWINS: KH_WSEE_SURF_TOL 0.02, KH_WSEE_NEAR_TOL 0.2, KH_WSEE_NEAR_TOL2 0.5 = cb.hlsl's; KH_PSB_LS 1 = KH_WSEE_PSB_LS =
//  KH_PSB_ZB_LS (static_assert); D = far / 4 = KhWseeFarRaw; the one pass's cb layout = the patcher's prologue
//  (kh_psb_post's note); the PASS_* table ("The re-issues' passes") = kh_wsee_re_set's binds.
//
// TAINT (kh_wsee_taint: the capture stays unused this cycle): a depth write of ours on the main depth after it landed
// (a late injection, the flush's solids, kh_white_draw); a re-issue refused or impossible (UAV or SO target bound, the
// late opaque's PS bound with class instances, a target or the bound depth gone); > KH_WSEE_OPAQUE_CAP replays; an
// unblended colour draw with the depth test off (overwrites B without a depth); a DrawAuto / indirect draw writing the
// depth (KH_WSEE_XDRAW); S2 not takeable in a cycle that skipped a pre replay.
// NOT COVERED: several layers of one draw overlapping (T counts each fragment, NW the nearest); two layers of one phase
// both off any surface share the nearer's depth; a no-write blended draw that tests stencil (left out); a late opaque
// whose PS writes its own depth (EQUAL fails: G stays 0); a layer over a late opaque not coplanar with it (behind = r);
// multiply / screen / subtractive / MIN-MAX blends (not in the record); a particle-aware layer drawn after B2 but
// behind a later fence (post: fogged at the fence); the effect meshes (unarmed); FSAA 1x (the hooks do not take the
// world's draws there); the one pass's 2 x 2 blocks (an edge, a dithered fade or a spark narrower than 2 px moves at
// block steps; the last W mod 2 pixels read their neighbour block); our dynamic-light world shadow (kh_dls_world_pass
// multiplies the scene at the resolve, after B / B2 were taken) is not in B / B2, so under a layer the distance fog's
// exact stages and KhWseeBehindColor rebuild a world surface our meshes shadow from a dynamic light unshadowed, and our
// sorted parts' composite (modes 2 / 3: B / B2 against Q, which holds the shadow) darkens that surface further behind
// them (held by the stacking cap).
// COST, wanted cycles: the capture (one R32F pass, one resolve when wanted, clears, Zb), each qualifying draw once more
// (the one pass at 1 / 4 the fragments; else two to five passes), each post late opaque's vertex work again, per run E
// and, where the last pass reads C, one scene copy; the D3 resolve. VRAM at 3840x2160 8x: about 1.2 GB (T 265 MB, Tp
// 265, D3m 265, S / NW / NW2 / S2 / D3 / Tf's depth 33 each, Tf 17, B / B2 33 - 66, E 33 + stencil 33, P / N 33, Zb
// 17), plus Q and the hold once the sort runs; nothing checks it against the card's memory. Unwanted: one bool a draw.
// The re-issues' saves, null outside one kh_wsee_re_save / kh_wsee_re_restore pair (render thread; the tail keeps
// its own).
static ID3D11RenderTargetView*  g_wsee_sv_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
static ID3D11DepthStencilView*  g_wsee_sv_dsv = nullptr;
static ID3D11BlendState*        g_wsee_sv_bs = nullptr;
static FLOAT                    g_wsee_sv_bf[4] = {};
static UINT                     g_wsee_sv_mask = 0xFFFFFFFFu;
static ID3D11DepthStencilState* g_wsee_sv_dss = nullptr;
static UINT                     g_wsee_sv_ref = 0;
static ID3D11PixelShader*       g_wsee_sv_ps = nullptr;      // The late opaque's, or a KH_PSV pick's original.
static bool                     g_wsee_sv_has_ps = false;    // g_wsee_sv_ps was taken (null is a valid shader).
// The engine's PS t0, taken with the pixel shader where the late-opaque pass binds the main depth's view there
// (kh_wsee_od_on; null is a valid binding).
static ID3D11ShaderResourceView* g_wsee_sv_srv0 = nullptr;
static bool                      g_wsee_sv_has_srv0 = false;
// KH_WSEE_OQ2: this post late opaque's replay takes the D3m path (kh_wsee_d3m_path at the save: no t0 taken then).
static bool                      g_wsee_sv_d3m = false;
// The tracked draw's viewports, read once in kh_wsee_pre_draw (the whole bound array; the slice and world tests take
// entry 0) and used by its post (KhWseeSurfVp, kh_wsee_vp_world / kh_wsee_vp_beyond, kh_psb_post). Valid because
// nothing sets a viewport between the two: the snapshot / B2 inside the pre-draw set theirs back (StateBackup), the
// engine's own draw is the one call in between, the post's OM binds run under in_injection (the OM hooks stand aside)
// and RSSetViewports is not hooked. g_wsee_pd_vp_ok: set by the read, cleared where the pre-draw begins and where the
// post takes it; the flush's tail (kh_wsee_tail) never reads it. Render thread.
static D3D11_VIEWPORT           g_wsee_pd_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
static UINT                     g_wsee_pd_vpn = 0;
static bool                     g_wsee_pd_vp_ok = false;
inline void kh_wsee_pd_vp_read(ID3D11DeviceContext* khvr_ctx) {
    g_wsee_pd_vpn = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    khvr_ctx->RSGetViewports(&g_wsee_pd_vpn, g_wsee_pd_vp);
    g_wsee_pd_vp_ok = g_wsee_pd_vpn >= 1 && g_wsee_pd_vpn <= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
}
static constexpr uint32_t KH_WSEE_WASTE_RUN = 8u;      // Tainted or refused attempts in a row before a back-off.
static constexpr uint64_t KH_WSEE_BACKOFF = 120u;      // Cycles a back-off stands the capture down.
static constexpr uint64_t KH_WSEE_DEFER_BACKOFF = 120u;   // Cycles a lost deferral stands the deferral down.
static constexpr uint64_t KH_WSEE_IDLE_FREE = 600u;    // Cycles without a want before the targets are released.
static uint32_t g_wsee_waste_n = 0;        // Render thread: the current run of wasted attempts.
static uint64_t g_wsee_backoff_end = 0;    // Render thread: no capture before this cycle.
// KH_WSEE_FLAP - a capture used on some cycles and wasted on others, cycle about (an opaque count about the cap, a
// flush depth write every other frame, a re-issue refused on alternate draws) never reaches KH_WSEE_WASTE_RUN, so the
// chain would arm and disarm frame by frame (the fog through a fence, our glass under its layers, on and off at frame
// rate: worse than either look held). Verdict changes (used <-> wasted, between consecutive attempted cycles) are
// counted over KH_WSEE_FLAP_WIN cycles; KH_WSEE_FLAP_N in a window begin the plain back-off (KH_WSEE_BACKOFF: the
// pre-record picture, held), then the counter starts afresh. A run of used cycles, or of wasted ones, counts nothing.
// Render thread (kh_wsee_cycle_note); reset with g_wsee_waste_n.
static constexpr uint64_t KH_WSEE_FLAP_WIN = 64u;
static constexpr uint32_t KH_WSEE_FLAP_N = 16u;
static int      g_wsee_flap_prev = -1;     // The last attempted cycle's verdict (1 used, 0 wasted; -1 none yet).
static uint64_t g_wsee_flap_win_start = 0;
static uint32_t g_wsee_flap_n = 0;
// KH_WSEE_FLUSHW_PRED - a flush solid of ours that writes the main depth (flush_locked's dss write arm) taints every
// capture it follows; where one draws frame after frame the plain back-off pays 8 captures in every 128 cycles for
// nothing. g_wsee_flushw_cycle: the last cycle such a write followed a landed capture, or fell in a cycle this rule
// stood down. Where a waste run (KH_WSEE_WASTE_RUN) ends with the ended cycle's flush write, the hold
// (g_wsee_flushw_hold) replaces the fixed back-off: a cycle after a stamped one captures nothing, but for one probe
// KH_WSEE_FLUSHW_PROBE (= KH_WSEE_BACKOFF) cycles after the last capture the cause spoiled (g_wsee_flushw_probe); a
// cycle after an unstamped one captures; a used capture ends the hold. The picture of a stood-down cycle is the
// backed-off one (the live depth). Trade against the fixed back-off: the cause stopping resumes the capture the next
// write-free cycle instead of after up to 120, and a persistent cause costs one capture per probe instead of 8; but the
// first write-free cycle after the cause stops is stood down where the back-off's 8-capture window would have taken it
// (one frame later), and a park writing before the capture point in stood-down cycles keeps them down until the probe.
static constexpr uint64_t KH_WSEE_FLUSHW_PROBE = KH_WSEE_BACKOFF;
static std::atomic<uint64_t> g_wsee_flushw_cycle{ ~0ull };   // Either thread (the flush; a park's).
static std::atomic<uint64_t> g_wsee_flushw_off{ ~0ull };     // Render thread writes (the clear); the flush reads.
static uint64_t g_wsee_flushw_probe = 0u;                     // Render thread.
static bool     g_wsee_flushw_hold = false;                   // Render thread.

// KH_PSV - the engine's particle pixel shader, reduced for the record's passes (kh_wsee_post: RE_NOWRITE,
// RE_NOWRITE_OFF, RE_NWONLY; kh_wsee_tail). D3D11 cannot read a created shader's bytecode back, so it is copied at
// CreatePixelShader (the hook below) and patched in place: "alpha" keeps only the .w lane of every o0 write, removes
// every other colour write and sets o0.xyz = 0 once after the declarations (the passes' source factor is ZERO); "nw"
// removes every colour write (discards, oDepth, oMask and what feeds them stay) - a shader with no discard and no
// depth / coverage output binds no PS at all for the NW pass. Declarations (nw: minus forceEarlyDepthStencil, below),
// control flow, derivatives, helper lanes and both signatures are unchanged (the engine's VS still links). o0.w
// is computed by the same instructions in the same order, so every pass of one re-issue takes one alpha; the driver
// may fuse differently in the reduced program (refactoringAllowed): at worst one UNORM8 step at a rounding boundary,
// and a cut-out within one step of its threshold may be decided the other way in NW (a second program) than in T.
// A two-destination op (sincos, udiv) whose colour destination goes writes null there. Refused (the engine's shader
// draws today's passes; the code named once in the RPT - KH_PSV_REFUSE_REPORT, kh_psv_make): a malformed container /
// program, an output written through a relative index, class linkage, a UAV, a read of an output, a tiled-resource
// feedback op, (alpha) an o0 without .w - KH_PSV_R_* below.
// The patcher is pure (bytes in, bytes out; no D3D call, no global): it runs standalone on a blob.
static constexpr uint32_t KH_PSV_MODE_ALPHA = 0u;   // T, Tp, Tp2, Tf.
static constexpr uint32_t KH_PSV_MODE_NW = 1u;      // NW.
static constexpr uint32_t KH_PSV_OK = 0u;           // khpp_out holds the variant.
static constexpr uint32_t KH_PSV_NOPS = 1u;         // nw: bind no pixel shader (khpp_out untouched).
static constexpr uint32_t KH_PSV_R_CONTAINER = 2u;  // Refusals.
static constexpr uint32_t KH_PSV_R_PROGRAM = 3u;
static constexpr uint32_t KH_PSV_R_TOKENS = 4u;
static constexpr uint32_t KH_PSV_R_RELOUT = 5u;
static constexpr uint32_t KH_PSV_R_LINKAGE = 6u;
static constexpr uint32_t KH_PSV_R_UAV = 7u;
static constexpr uint32_t KH_PSV_R_NOALPHA = 8u;
static constexpr uint32_t KH_PSV_R_OUTREAD = 9u;
static constexpr uint32_t KH_PSV_R_DEPTHOUT = 10u;  // A depth / coverage output (kh_psb_patch's refusal).
static constexpr uint32_t KH_PSV_R_FEEDBACK = 12u;  // A tiled-resource feedback op (a status destination).
// The container's checksum (bytes 4 - 19): MD5's transform over bytes 20 - end with DXBC's own last block - the bit
// count in its first dword and (bits >> 2) | 1 in its last (a block of its own when 56 bytes or more remain). The
// runtime refuses a blob whose checksum does not match; this one reproduces fxc's on its blobs.
inline void kh_psv_md5(uint32_t khpm_s[4], const uint8_t* khpm_b) {
    static const uint32_t khpm_k[64] = {
        0xD76AA478u, 0xE8C7B756u, 0x242070DBu, 0xC1BDCEEEu, 0xF57C0FAFu, 0x4787C62Au, 0xA8304613u, 0xFD469501u,
        0x698098D8u, 0x8B44F7AFu, 0xFFFF5BB1u, 0x895CD7BEu, 0x6B901122u, 0xFD987193u, 0xA679438Eu, 0x49B40821u,
        0xF61E2562u, 0xC040B340u, 0x265E5A51u, 0xE9B6C7AAu, 0xD62F105Du, 0x02441453u, 0xD8A1E681u, 0xE7D3FBC8u,
        0x21E1CDE6u, 0xC33707D6u, 0xF4D50D87u, 0x455A14EDu, 0xA9E3E905u, 0xFCEFA3F8u, 0x676F02D9u, 0x8D2A4C8Au,
        0xFFFA3942u, 0x8771F681u, 0x6D9D6122u, 0xFDE5380Cu, 0xA4BEEA44u, 0x4BDECFA9u, 0xF6BB4B60u, 0xBEBFBC70u,
        0x289B7EC6u, 0xEAA127FAu, 0xD4EF3085u, 0x04881D05u, 0xD9D4D039u, 0xE6DB99E5u, 0x1FA27CF8u, 0xC4AC5665u,
        0xF4292244u, 0x432AFF97u, 0xAB9423A7u, 0xFC93A039u, 0x655B59C3u, 0x8F0CCC92u, 0xFFEFF47Du, 0x85845DD1u,
        0x6FA87E4Fu, 0xFE2CE6E0u, 0xA3014314u, 0x4E0811A1u, 0xF7537E82u, 0xBD3AF235u, 0x2AD7D2BBu, 0xEB86D391u };
    static const uint32_t khpm_r[16] = { 7u, 12u, 17u, 22u, 5u, 9u, 14u, 20u, 4u, 11u, 16u, 23u, 6u, 10u, 15u, 21u };
    uint32_t khpm_m[16];
    for (int khpm_i = 0; khpm_i < 16; ++khpm_i) {
        khpm_m[khpm_i] = static_cast<uint32_t>(khpm_b[4 * khpm_i]) |
                         (static_cast<uint32_t>(khpm_b[4 * khpm_i + 1]) << 8) |
                         (static_cast<uint32_t>(khpm_b[4 * khpm_i + 2]) << 16) |
                         (static_cast<uint32_t>(khpm_b[4 * khpm_i + 3]) << 24);
    }
    uint32_t khpm_a = khpm_s[0], khpm_bb = khpm_s[1], khpm_c = khpm_s[2], khpm_d = khpm_s[3];
    for (int khpm_i = 0; khpm_i < 64; ++khpm_i) {
        uint32_t khpm_f = 0u;
        int khpm_g = 0;
        if (khpm_i < 16) {
            khpm_f = (khpm_bb & khpm_c) | (~khpm_bb & khpm_d);
            khpm_g = khpm_i;
        } else if (khpm_i < 32) {
            khpm_f = (khpm_d & khpm_bb) | (~khpm_d & khpm_c);
            khpm_g = (5 * khpm_i + 1) % 16;
        } else if (khpm_i < 48) {
            khpm_f = khpm_bb ^ khpm_c ^ khpm_d;
            khpm_g = (3 * khpm_i + 5) % 16;
        } else {
            khpm_f = khpm_c ^ (khpm_bb | ~khpm_d);
            khpm_g = (7 * khpm_i) % 16;
        }
        khpm_f = khpm_f + khpm_a + khpm_k[khpm_i] + khpm_m[khpm_g];
        khpm_a = khpm_d;
        khpm_d = khpm_c;
        khpm_c = khpm_bb;
        const uint32_t khpm_sh = khpm_r[(khpm_i / 16) * 4 + (khpm_i % 4)];
        khpm_bb = khpm_bb + ((khpm_f << khpm_sh) | (khpm_f >> (32u - khpm_sh)));
    }
    khpm_s[0] += khpm_a;
    khpm_s[1] += khpm_bb;
    khpm_s[2] += khpm_c;
    khpm_s[3] += khpm_d;
}
inline void kh_psv_put32(uint8_t* khpu_p, uint32_t khpu_v) {
    khpu_p[0] = static_cast<uint8_t>(khpu_v);
    khpu_p[1] = static_cast<uint8_t>(khpu_v >> 8);
    khpu_p[2] = static_cast<uint8_t>(khpu_v >> 16);
    khpu_p[3] = static_cast<uint8_t>(khpu_v >> 24);
}
inline uint32_t kh_psv_get32(const uint8_t* khpg_p) {
    return static_cast<uint32_t>(khpg_p[0]) | (static_cast<uint32_t>(khpg_p[1]) << 8) |
           (static_cast<uint32_t>(khpg_p[2]) << 16) | (static_cast<uint32_t>(khpg_p[3]) << 24);
}
inline void kh_psv_checksum(const uint8_t* khpc_b, size_t khpc_n, uint32_t khpc_out[4]) {
    uint32_t khpc_s[4] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };
    const uint8_t* const khpc_d = khpc_b + 20;
    const size_t khpc_len = khpc_n - 20u;
    const size_t khpc_full = khpc_len - khpc_len % 64u;
    for (size_t khpc_o = 0; khpc_o < khpc_full; khpc_o += 64u) kh_psv_md5(khpc_s, khpc_d + khpc_o);
    const size_t khpc_left = khpc_len - khpc_full;
    const uint32_t khpc_bits = static_cast<uint32_t>(khpc_len * 8u);
    uint8_t khpc_blk[64];
    memset(khpc_blk, 0, sizeof(khpc_blk));
    if (khpc_left >= 56u) {
        memcpy(khpc_blk, khpc_d + khpc_full, khpc_left);
        khpc_blk[khpc_left] = 0x80u;
        kh_psv_md5(khpc_s, khpc_blk);
        memset(khpc_blk, 0, sizeof(khpc_blk));
        kh_psv_put32(khpc_blk, khpc_bits);
        kh_psv_put32(khpc_blk + 60, (khpc_bits >> 2) | 1u);
    } else {
        kh_psv_put32(khpc_blk, khpc_bits);
        memcpy(khpc_blk + 4, khpc_d + khpc_full, khpc_left);
        khpc_blk[4 + khpc_left] = 0x80u;
        kh_psv_put32(khpc_blk + 60, (khpc_bits >> 2) | 1u);
    }
    kh_psv_md5(khpc_s, khpc_blk);
    for (int khpc_i = 0; khpc_i < 4; ++khpc_i) khpc_out[khpc_i] = khpc_s[khpc_i];
}
// One operand's length in dwords at khpo_t[khpo_i] (its extended tokens, indices and relative-index operands
// included); 0 = malformed (past khpo_end, or an index representation SM4 / SM5 does not define).
inline size_t kh_psv_oplen(const uint32_t* khpo_t, size_t khpo_i, size_t khpo_end, int khpo_depth = 0) {
    if (khpo_i >= khpo_end || khpo_depth > 4) return 0;
    const uint32_t khpo_tk = khpo_t[khpo_i];
    size_t khpo_n = 1;
    bool khpo_ext = (khpo_tk >> 31) != 0u;
    while (khpo_ext) {
        if (khpo_i + khpo_n >= khpo_end) return 0;
        khpo_ext = (khpo_t[khpo_i + khpo_n] >> 31) != 0u;
        ++khpo_n;
    }
    const uint32_t khpo_type = (khpo_tk >> 12) & 0xFFu;
    const uint32_t khpo_nc = khpo_tk & 3u;
    if (khpo_type == 4u) return khpo_i + khpo_n + (khpo_nc == 1u ? 1u : 4u) <= khpo_end ?
                                khpo_n + (khpo_nc == 1u ? 1u : 4u) : 0;   // Immediate 32-bit.
    if (khpo_type == 5u) return khpo_i + khpo_n + (khpo_nc == 1u ? 2u : 4u) <= khpo_end ?
                                khpo_n + (khpo_nc == 1u ? 2u : 4u) : 0;   // Immediate 64-bit: one double, or two
                                                                           // (xy / zw) in four dwords.
    const uint32_t khpo_dim = (khpo_tk >> 20) & 3u;
    for (uint32_t khpo_k = 0; khpo_k < khpo_dim; ++khpo_k) {
        const uint32_t khpo_rep = (khpo_tk >> (22u + 3u * khpo_k)) & 7u;
        if (khpo_rep == 0u) {
            khpo_n += 1;
        } else if (khpo_rep == 1u) {
            khpo_n += 2;
        } else if (khpo_rep >= 2u && khpo_rep <= 4u) {
            khpo_n += khpo_rep == 2u ? 0u : khpo_rep == 3u ? 1u : 2u;
            const size_t khpo_r = kh_psv_oplen(khpo_t, khpo_i + khpo_n, khpo_end, khpo_depth + 1);
            if (khpo_r == 0) return 0;
            khpo_n += khpo_r;
        } else {
            return 0;
        }
        if (khpo_i + khpo_n > khpo_end) return 0;
    }
    return khpo_n;
}
// The output operand at khpr_t[khpr_i] (khpr_end its end): the position of its one register index - an immediate
// below 8 past its extended tokens -, else 0 (indexed at run time, two-dimensional, or past the end).
inline size_t kh_psv_reg0(const uint32_t* khpr_t, size_t khpr_i, size_t khpr_end) {
    const uint32_t khpr_tk = khpr_t[khpr_i];
    if (((khpr_tk >> 20) & 3u) != 1u || ((khpr_tk >> 22) & 7u) != 0u) return 0;
    size_t khpr_x = khpr_i + 1;
    bool khpr_ext = (khpr_tk >> 31) != 0u;
    while (khpr_ext) {
        if (khpr_x >= khpr_end) return 0;
        khpr_ext = (khpr_t[khpr_x] >> 31) != 0u;
        ++khpr_x;
    }
    return khpr_x < khpr_end && khpr_t[khpr_x] < 8u ? khpr_x : 0;
}
// The variant of the container khpp_b (khpp_n bytes) for khpp_mode into khpp_out (KH_PSV_OK), KH_PSV_NOPS (nw: no
// pixel shader needed; khpp_out untouched), or a refusal (KH_PSV_R_*; khpp_out untouched). The chunks are copied in
// their order and SHEX / SHDR replaced; the header's size, offsets and checksum are rebuilt.
inline uint32_t kh_psv_patch(const uint8_t* khpp_b, size_t khpp_n, uint32_t khpp_mode,
                             std::vector<uint8_t>& khpp_out) {
    if (!khpp_b || khpp_n < 32u || khpp_n > 0x4000000u || memcmp(khpp_b, "DXBC", 4) != 0 ||
        kh_psv_get32(khpp_b + 24) != khpp_n) return KH_PSV_R_CONTAINER;
    const uint32_t khpp_cn = kh_psv_get32(khpp_b + 28);
    if (khpp_cn == 0u || khpp_cn > 64u || 32u + 4u * static_cast<size_t>(khpp_cn) > khpp_n) return KH_PSV_R_CONTAINER;
    size_t khpp_sx = 0, khpp_sxn = 0;   // The program chunk's payload.
    uint32_t khpp_sxi = khpp_cn;
    for (uint32_t khpp_c = 0; khpp_c < khpp_cn; ++khpp_c) {
        const size_t khpp_o = kh_psv_get32(khpp_b + 32 + 4 * khpp_c);
        if (khpp_o + 8u > khpp_n) return KH_PSV_R_CONTAINER;
        const size_t khpp_cs = kh_psv_get32(khpp_b + khpp_o + 4);
        if (khpp_cs > khpp_n - khpp_o - 8u || (khpp_cs & 3u) != 0u) return KH_PSV_R_CONTAINER;
        if (memcmp(khpp_b + khpp_o, "SHEX", 4) == 0 || memcmp(khpp_b + khpp_o, "SHDR", 4) == 0) {
            if (khpp_sxi != khpp_cn) return KH_PSV_R_CONTAINER;   // Two programs.
            khpp_sxi = khpp_c;
            khpp_sx = khpp_o + 8u;
            khpp_sxn = khpp_cs;
        }
    }
    if (khpp_sxi == khpp_cn || khpp_sxn < 8u) return KH_PSV_R_CONTAINER;
    std::vector<uint32_t> khpp_t(khpp_sxn / 4u);
    for (size_t khpp_i = 0; khpp_i < khpp_t.size(); ++khpp_i) {
        khpp_t[khpp_i] = kh_psv_get32(khpp_b + khpp_sx + 4 * khpp_i);
    }
    const size_t khpp_len = khpp_t[1];
    if (((khpp_t[0] >> 16) & 0xFFFFu) != 0u || khpp_len < 2u || khpp_len > khpp_t.size()) return KH_PSV_R_PROGRAM;
    std::vector<uint32_t> khpp_o;   // The new program.
    khpp_o.reserve(khpp_len + 16u);
    khpp_o.push_back(khpp_t[0]);
    khpp_o.push_back(0u);   // Its length, below.
    uint32_t khpp_dmask[8] = {};   // Declared colour outputs' masks (o0 - o7).
    bool khpp_dz = false;   // A depth / coverage / stencil output declared.
    bool khpp_disc = false;
    size_t khpp_body = 0;   // Where the first instruction past the declarations went in khpp_o (0 = none yet).
    size_t khpp_i = 2;
    while (khpp_i < khpp_len) {
        const uint32_t khpp_tk = khpp_t[khpp_i];
        const uint32_t khpp_op = khpp_tk & 0x7FFu;
        size_t khpp_ln = 0;
        if (khpp_op == 53u) {   // customdata (the immediate constant buffer): its length in the next dword.
            if (khpp_i + 1u >= khpp_len) return KH_PSV_R_TOKENS;
            khpp_ln = khpp_t[khpp_i + 1];
            if (khpp_ln < 2u) return KH_PSV_R_TOKENS;
        } else {
            khpp_ln = (khpp_tk >> 24) & 0x7Fu;
        }
        if (khpp_ln == 0u || khpp_ln > khpp_len - khpp_i) return KH_PSV_R_TOKENS;
        const bool khpp_dcl = khpp_op == 53u || (khpp_op >= 88u && khpp_op <= 106u) ||
                              (khpp_op >= 143u && khpp_op <= 162u) || (khpp_op >= 113u && khpp_op <= 116u) ||
                              khpp_op == 206u;   // dcl_gs_instance_count (not a PS's; copied if met).
        if (khpp_dcl) {
            if (khpp_op == 144u || khpp_op == 145u || khpp_op == 146u) return KH_PSV_R_LINKAGE;   // Function bodies.
            if (khpp_op >= 156u && khpp_op <= 160u) return KH_PSV_R_UAV;   // UAVs, group-shared memory.
            if ((khpp_op >= 101u && khpp_op <= 103u) || khpp_op == 91u) {   // dcl_output*, dcl_index_range.
                if (khpp_ln < 2u) return KH_PSV_R_TOKENS;
                const uint32_t khpp_d = khpp_t[khpp_i + 1];
                const uint32_t khpp_ty = (khpp_d >> 12) & 0xFFu;
                if (khpp_ty == 2u) {
                    if (khpp_op == 91u) return KH_PSV_R_RELOUT;   // An output range indexed at run time.
                    const size_t khpp_x = kh_psv_reg0(khpp_t.data(), khpp_i + 1, khpp_i + khpp_ln);
                    if (khpp_x == 0u) return KH_PSV_R_RELOUT;
                    khpp_dmask[khpp_t[khpp_x]] |= (khpp_d & 3u) == 2u && ((khpp_d >> 2) & 3u) == 0u ?
                                                  (khpp_d >> 4) & 0xFu : 0xFu;
                } else if (khpp_ty == 12u || khpp_ty == 15u || khpp_ty == 38u || khpp_ty == 39u || khpp_ty == 41u) {
                    khpp_dz = true;   // oDepth (any), oMask, oStencilRef.
                }
            }
            khpp_o.insert(khpp_o.end(), khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_i),
                          khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_i + khpp_ln));
            // KH_PSV: nw drops dcl_globalFlags' forceEarlyDepthStencil (bit 13) - the NW pass writes depth, and forced
            // early depth would write it before the shader's discard (no UAV here; an SV_Coverage input would no longer
            // reflect the early test - no engine shader seen carries the flag).
            if (khpp_mode == KH_PSV_MODE_NW && khpp_op == 106u) khpp_o[khpp_o.size() - khpp_ln] &= ~(1u << 13);
            khpp_i += khpp_ln;
            continue;
        }
        if (khpp_body == 0) khpp_body = khpp_o.size();
        if (khpp_op == 120u) return KH_PSV_R_LINKAGE;   // fcall.
        // The tiled-resource feedback ops (gather4 / ld / sample ... with a status destination, 219 - 233): a
        // dropped colour write would take their status write with it - refused whole (none in the engine's export).
        if (khpp_op >= 219u && khpp_op <= 233u) return KH_PSV_R_FEEDBACK;
        if (khpp_op == 13u) khpp_disc = true;   // discard.
        // The operands, one by one, to the instruction's end exactly; the colour outputs among them noted.
        size_t khpp_j = khpp_i + 1;
        if ((khpp_tk >> 31) != 0u) {
            bool khpp_e = true;
            while (khpp_e) {
                if (khpp_j >= khpp_i + khpp_ln) return KH_PSV_R_TOKENS;
                khpp_e = (khpp_t[khpp_j] >> 31) != 0u;
                ++khpp_j;
            }
        }
        // Two-destination instructions: sincos, udiv, umul, imul, uaddc, usubb, swapc.
        const bool khpp_two = khpp_op == 77u || khpp_op == 78u || khpp_op == 81u || khpp_op == 38u ||
                              khpp_op == 132u || khpp_op == 133u || khpp_op == 142u;
        size_t khpp_dp[2] = { 0, 0 };   // The destinations' positions (an instruction has at most two) and lengths.
        size_t khpp_dl[2] = { 0, 0 };
        uint32_t khpp_dty[2] = { 0xFFu, 0xFFu };   // Their operand types.
        uint32_t khpp_reg[2] = { 0u, 0u };   // A colour output's register and mask.
        uint32_t khpp_msk[2] = { 0u, 0u };
        bool khpp_any = false;
        int khpp_k = 0;
        while (khpp_j < khpp_i + khpp_ln) {
            const size_t khpp_ol = kh_psv_oplen(khpp_t.data(), khpp_j, khpp_i + khpp_ln);
            if (khpp_ol == 0u) return KH_PSV_R_TOKENS;
            const uint32_t khpp_q = khpp_t[khpp_j];
            const uint32_t khpp_qt = (khpp_q >> 12) & 0xFFu;
            if (khpp_k < 2) {
                khpp_dp[khpp_k] = khpp_j;
                khpp_dl[khpp_k] = khpp_ol;
                khpp_dty[khpp_k] = khpp_qt;
            }
            if (khpp_qt == 2u) {   // An output register: written (outputs are write-only), so a destination.
                if (khpp_k > 1 || (khpp_k == 1 && !khpp_two)) return KH_PSV_R_OUTREAD;
                const size_t khpp_x = kh_psv_reg0(khpp_t.data(), khpp_j, khpp_j + khpp_ol);
                if (khpp_x == 0u) return KH_PSV_R_RELOUT;
                if ((khpp_q & 3u) != 2u || ((khpp_q >> 2) & 3u) != 0u) return KH_PSV_R_TOKENS;   // Not a mask.
                khpp_reg[khpp_k] = khpp_t[khpp_x];
                khpp_msk[khpp_k] = (khpp_q >> 4) & 0xFu;
                khpp_any = true;
            }
            khpp_j += khpp_ol;
            ++khpp_k;
        }
        if (khpp_j != khpp_i + khpp_ln) return KH_PSV_R_TOKENS;
        if (!khpp_any) {   // No colour destination: copied whole.
            khpp_o.insert(khpp_o.end(), khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_i),
                          khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_i + khpp_ln));
            khpp_i += khpp_ln;
            continue;
        }
        // Each colour destination kept as o0.w (alpha, o0 written with .w) or dropped; a dropped one becomes the null
        // operand where the instruction's other destination still writes, else the instruction goes.
        const int khpp_nd = khpp_two ? 2 : 1;
        bool khpp_keep[2] = { false, false };
        bool khpp_live = false;   // A destination still written.
        for (int khpp_d = 0; khpp_d < khpp_nd; ++khpp_d) {
            if (khpp_dty[khpp_d] == 2u) {
                khpp_keep[khpp_d] = khpp_mode == KH_PSV_MODE_ALPHA && khpp_reg[khpp_d] == 0u &&
                                    (khpp_msk[khpp_d] & 8u) != 0u;
                if (khpp_keep[khpp_d]) khpp_live = true;
            } else if (khpp_dty[khpp_d] != 13u) {
                khpp_live = true;   // Not null: a register written.
            }
        }
        if (khpp_live) {
            const size_t khpp_at = khpp_o.size();
            khpp_o.push_back(0u);   // The opcode token, its length set below.
            for (size_t khpp_x = khpp_i + 1; khpp_x < khpp_i + khpp_ln;) {
                int khpp_d = -1;
                if (khpp_x == khpp_dp[0] && khpp_dty[0] == 2u) khpp_d = 0;
                if (khpp_two && khpp_x == khpp_dp[1] && khpp_dty[1] == 2u) khpp_d = 1;
                if (khpp_d < 0) {
                    khpp_o.push_back(khpp_t[khpp_x]);
                    ++khpp_x;
                } else if (khpp_keep[khpp_d]) {
                    khpp_o.push_back((khpp_t[khpp_x] & ~(0xFu << 4)) | (8u << 4));   // o0.w alone.
                    khpp_o.insert(khpp_o.end(), khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_x + 1),
                                  khpp_t.begin() + static_cast<std::ptrdiff_t>(khpp_x + khpp_dl[khpp_d]));
                    khpp_x += khpp_dl[khpp_d];
                } else {
                    khpp_o.push_back(13u << 12);   // null.
                    khpp_x += khpp_dl[khpp_d];
                }
            }
            const size_t khpp_nl = khpp_o.size() - khpp_at;
            khpp_o[khpp_at] = (khpp_tk & ~(0x7Fu << 24)) | (static_cast<uint32_t>(khpp_nl) << 24);
        }
        khpp_i += khpp_ln;
    }
    if (khpp_i != khpp_len) return KH_PSV_R_TOKENS;
    if (khpp_body == 0) khpp_body = khpp_o.size();
    if (khpp_mode == KH_PSV_MODE_NW && !khpp_disc && !khpp_dz) return KH_PSV_NOPS;
    if (khpp_mode != KH_PSV_MODE_NW) {
        if ((khpp_dmask[0] & 8u) == 0u) return KH_PSV_R_NOALPHA;   // alpha.
        // mov o0.<mask>, l(0, 0, 0, 0): xyz (the declared ones).
        const uint32_t khpp_mm = khpp_dmask[0] & 7u;
        if (khpp_mm != 0u) {
            const uint32_t khpp_mov[8] = { 54u | (8u << 24), 2u | (khpp_mm << 4) | (2u << 12) | (1u << 20), 0u,
                                           2u | (4u << 12), 0u, 0u, 0u, 0u };
            khpp_o.insert(khpp_o.begin() + static_cast<std::ptrdiff_t>(khpp_body), khpp_mov, khpp_mov + 8);
        }
    }
    khpp_o[1] = static_cast<uint32_t>(khpp_o.size());
    // The container again: the chunks in their order, the program replaced.
    size_t khpp_total = 32u + 4u * khpp_cn;
    for (uint32_t khpp_c = 0; khpp_c < khpp_cn; ++khpp_c) {
        const size_t khpp_co = kh_psv_get32(khpp_b + 32 + 4 * khpp_c);
        khpp_total += 8u + (khpp_c == khpp_sxi ? 4u * khpp_o.size() : kh_psv_get32(khpp_b + khpp_co + 4));
    }
    std::vector<uint8_t> khpp_nb(khpp_total, 0u);
    memcpy(khpp_nb.data(), khpp_b, 20);
    kh_psv_put32(khpp_nb.data() + 20, kh_psv_get32(khpp_b + 20));
    kh_psv_put32(khpp_nb.data() + 24, static_cast<uint32_t>(khpp_total));
    kh_psv_put32(khpp_nb.data() + 28, khpp_cn);
    size_t khpp_w = 32u + 4u * khpp_cn;
    for (uint32_t khpp_c = 0; khpp_c < khpp_cn; ++khpp_c) {
        const size_t khpp_co = kh_psv_get32(khpp_b + 32 + 4 * khpp_c);
        kh_psv_put32(khpp_nb.data() + 32 + 4 * khpp_c, static_cast<uint32_t>(khpp_w));
        memcpy(khpp_nb.data() + khpp_w, khpp_b + khpp_co, 4);
        if (khpp_c == khpp_sxi) {
            kh_psv_put32(khpp_nb.data() + khpp_w + 4, static_cast<uint32_t>(4u * khpp_o.size()));
            for (size_t khpp_q = 0; khpp_q < khpp_o.size(); ++khpp_q) {
                kh_psv_put32(khpp_nb.data() + khpp_w + 8 + 4 * khpp_q, khpp_o[khpp_q]);
            }
            khpp_w += 8u + 4u * khpp_o.size();
        } else {
            const size_t khpp_cs = kh_psv_get32(khpp_b + khpp_co + 4);
            memcpy(khpp_nb.data() + khpp_w + 4, khpp_b + khpp_co + 4, 4u + khpp_cs);
            khpp_w += 8u + khpp_cs;
        }
    }
    uint32_t khpp_sum[4];
    kh_psv_checksum(khpp_nb.data(), khpp_nb.size(), khpp_sum);
    for (int khpp_q = 0; khpp_q < 4; ++khpp_q) kh_psv_put32(khpp_nb.data() + 4 + 4 * khpp_q, khpp_sum[khpp_q]);
    khpp_out.swap(khpp_nb);
    return KH_PSV_OK;
}
// KH_PSB - the one pass: a third variant of the same bytes that draws the particle-type record in ONE single-sample
// draw with no depth view (kh_psb_post). Prologue (before the engine's code): z = SV_Position.z; the pushes
// z_i = z m_i + a_i (the viewport shift's algebra, (m_i, a_i) = (1 - TOL_i, TOL_i Hi) for TOL 0.02 / 0.2 / 0.5 where
// the draw's shift stood, else (1, 0); cb at a free slot); the reference r = the largest sample 0 of the block's s x s
// pixels (the depth's MS view at a free t slot, s = 1 << KH_PSB_LS = 2; the loop stops once r reaches every bound it is
// needed for, Zb bounding it while cb3.w stands); verdicts z_i <= r (or all passed for the test-off kind), far = z > D
// and r >= Dq; a fragment no verdict passes whose NW write is a no-op (z_1 >= 1) is discarded. Then the alpha program
// with o0.w into a temp, and before every ret the epilogue: o0 = (aT, aP1, aP2, aF) (a where the verdict passed, +0
// where not), o1.x = z_1. The blend states pick the phase's channels (C++). Signatures: SV_Position's used mask gains
// .xyz; the output signature becomes SV_Target0 - 1 (the input signature's layout is unchanged: the engine's VS still
// links). KH_PSB_LORES: the engine's own SV_Position reads take a real pixel of the frame from a temp the prologue
// fills (a soft particle's depth fetch, a screen-space buffer, a dither cell address the frame as at full size; the
// formula at the prologue's end), so the one pass is drawn at 1 / s of the frame (kh_psb_post scales the viewports)
// and the constant buffer's fourth row says s and the reads.
// Refused (KH_PSV_R_* / KH_PSB_R_*; the alpha / nw passes draw instead): every alpha refusal; an output other than
// one float SV_Target0 with .w; no SV_Position xyz input; a read of SV_Position.z (its alpha would differ from the
// passes', each pushed its own); a per-sample / coverage input or an eval / samplepos / samplecount op (one sample);
// subroutines; more than KH_PSB_RET_MAX rets; no free cb / t slot; a depth / coverage output (the tail's PSMain /
// PSComposite); an input register SV_Position shares with another element or read by a run-time index.
static constexpr uint32_t KH_PSB_R_SIG = 13u;     // The signatures (ISGN / OSGN) are not as the one pass needs.
static constexpr uint32_t KH_PSB_R_NOPOS = 14u;   // No float SV_Position with .xyz in the input signature.
static constexpr uint32_t KH_PSB_R_POSZ = 15u;    // The shader reads SV_Position.z itself.
static constexpr uint32_t KH_PSB_R_RATE = 16u;    // A per-sample / coverage input, an eval or sample-position op.
static constexpr uint32_t KH_PSB_R_SUB = 17u;     // Subroutines of its own (label / call).
static constexpr uint32_t KH_PSB_R_RETS = 19u;    // More rets than KH_PSB_RET_MAX (each gets the epilogue inlined).
static constexpr uint32_t KH_PSB_RET_MAX = 16u;   // Rets a variant may inline the epilogue at.
static constexpr uint32_t KH_PSB_R_SLOT = 18u;    // No free constant-buffer or resource slot.
static constexpr uint32_t KH_PSB_R_POSX = 20u;    // KH_PSB_LORES: an input read by a run-time index.
static constexpr uint32_t KH_PSB_TEMPS = 4u;      // The temps the variant adds (alpha, pushes, loop, address).
// One SM5 operand token of the epilogue: kind 0 = a destination mask, 1 = a four-component swizzle, 2 = one
// component; khpo_dim the index dimension.
inline uint32_t kh_psb_op(uint32_t khpo_type, uint32_t khpo_kind, uint32_t khpo_sel, uint32_t khpo_dim) {
    return 2u | (khpo_kind << 2) | (khpo_sel << 4) | (khpo_type << 12) | (khpo_dim << 20);
}
// KH_PSB_LORES: the operand at khpf_t[khpf_i] (khpf_end its end) and every index operand inside it, in place - a read
// of the input register khpf_pr (SV_Position's) becomes a read of temp khpf_rd (its type and index alone: components,
// swizzle and modifiers kept). False: an input read by a run-time or two-dimensional index (a pixel shader needs
// dcl_indexRange for that, which the patcher refuses anyway) or a malformed operand.
inline bool kh_psb_pos_fix(uint32_t* khpf_t, size_t khpf_i, size_t khpf_end, uint32_t khpf_pr, uint32_t khpf_rd,
                           int khpf_depth = 0) {
    if (khpf_i >= khpf_end || khpf_depth > 4) return false;
    const uint32_t khpf_tk = khpf_t[khpf_i];
    size_t khpf_n = 1;
    bool khpf_ext = (khpf_tk >> 31) != 0u;
    while (khpf_ext) {
        if (khpf_i + khpf_n >= khpf_end) return false;
        khpf_ext = (khpf_t[khpf_i + khpf_n] >> 31) != 0u;
        ++khpf_n;
    }
    const uint32_t khpf_type = (khpf_tk >> 12) & 0xFFu;
    if (khpf_type == 4u || khpf_type == 5u) return true;   // Immediates.
    const uint32_t khpf_dim = (khpf_tk >> 20) & 3u;
    if (khpf_type == 1u) {   // An input: one immediate index.
        if (khpf_dim != 1u || ((khpf_tk >> 22) & 7u) != 0u || khpf_i + khpf_n >= khpf_end) return false;
        if (khpf_t[khpf_i + khpf_n] == khpf_pr) {
            khpf_t[khpf_i] = khpf_tk & ~(0xFFu << 12);   // TEMP (type 0).
            khpf_t[khpf_i + khpf_n] = khpf_rd;
        }
        return true;
    }
    for (uint32_t khpf_k = 0; khpf_k < khpf_dim; ++khpf_k) {
        const uint32_t khpf_rep = (khpf_tk >> (22u + 3u * khpf_k)) & 7u;
        if (khpf_rep == 0u) {
            khpf_n += 1;
        } else if (khpf_rep == 1u) {
            khpf_n += 2;
        } else if (khpf_rep >= 2u && khpf_rep <= 4u) {
            khpf_n += khpf_rep == 2u ? 0u : khpf_rep == 3u ? 1u : 2u;
            const size_t khpf_r = kh_psv_oplen(khpf_t, khpf_i + khpf_n, khpf_end, khpf_depth + 1);
            if (khpf_r == 0 ||
                !kh_psb_pos_fix(khpf_t, khpf_i + khpf_n, khpf_i + khpf_n + khpf_r, khpf_pr, khpf_rd, khpf_depth + 1)) {
                return false;
            }
            khpf_n += khpf_r;
        } else {
            return false;
        }
        if (khpf_i + khpf_n > khpf_end) return false;
    }
    return true;
}
// The variant "one" of the container khpb_b (khpb_n bytes) into khpb_out (KH_PSV_OK), its constant-buffer slot in
// khpb_cbs and its depth view's slot in khpb_ts (KH_PSB_ZB: Zb's, the next free below it, in khpb_tzs); or a refusal
// (KH_PSV_R_* / KH_PSB_R_*; the outputs untouched).
inline uint32_t kh_psb_patch(const uint8_t* khpb_b, size_t khpb_n, std::vector<uint8_t>& khpb_out,
                             uint32_t& khpb_cbs, uint32_t& khpb_ts, uint32_t& khpb_tzs) {
    if (!khpb_b || khpb_n < 32u || khpb_n > 0x4000000u || memcmp(khpb_b, "DXBC", 4) != 0 ||
        kh_psv_get32(khpb_b + 24) != khpb_n) return KH_PSV_R_CONTAINER;
    const uint32_t khpb_cn = kh_psv_get32(khpb_b + 28);
    if (khpb_cn == 0u || khpb_cn > 64u || 32u + 4u * static_cast<size_t>(khpb_cn) > khpb_n) return KH_PSV_R_CONTAINER;
    uint32_t khpb_xi = khpb_cn, khpb_ii = khpb_cn, khpb_oi = khpb_cn;   // SHEX, ISGN, OSGN.
    for (uint32_t khpb_c = 0; khpb_c < khpb_cn; ++khpb_c) {
        const size_t khpb_o = kh_psv_get32(khpb_b + 32 + 4 * khpb_c);
        if (khpb_o + 8u > khpb_n) return KH_PSV_R_CONTAINER;
        const size_t khpb_cs = kh_psv_get32(khpb_b + khpb_o + 4);
        if (khpb_cs > khpb_n - khpb_o - 8u || (khpb_cs & 3u) != 0u) return KH_PSV_R_CONTAINER;
        const uint8_t* const khpb_g = khpb_b + khpb_o;
        if (memcmp(khpb_g, "SHEX", 4) == 0 || memcmp(khpb_g, "SHDR", 4) == 0) {
            if (khpb_xi != khpb_cn) return KH_PSV_R_CONTAINER;   // Two programs.
            khpb_xi = khpb_c;
        } else if (memcmp(khpb_g, "ISGN", 4) == 0) {
            if (khpb_ii != khpb_cn) return KH_PSB_R_SIG;
            khpb_ii = khpb_c;
        } else if (memcmp(khpb_g, "OSGN", 4) == 0) {
            if (khpb_oi != khpb_cn) return KH_PSB_R_SIG;
            khpb_oi = khpb_c;
        } else if (memcmp(khpb_g, "ISG1", 4) == 0 || memcmp(khpb_g, "OSG1", 4) == 0 ||
                   memcmp(khpb_g, "OSG5", 4) == 0 || memcmp(khpb_g, "PCSG", 4) == 0) {
            return KH_PSB_R_SIG;
        }
    }
    if (khpb_xi == khpb_cn) return KH_PSV_R_CONTAINER;
    const size_t khpb_xo = kh_psv_get32(khpb_b + 32 + 4 * khpb_xi) + 8u;
    const size_t khpb_xn = kh_psv_get32(khpb_b + khpb_xo - 4u);
    if (khpb_xn < 8u) return KH_PSV_R_CONTAINER;
    std::vector<uint32_t> khpb_t(khpb_xn / 4u);
    for (size_t khpb_w0 = 0; khpb_w0 < khpb_t.size(); ++khpb_w0) {
        khpb_t[khpb_w0] = kh_psv_get32(khpb_b + khpb_xo + 4 * khpb_w0);
    }
    const size_t khpb_len = khpb_t[1];
    // A pixel shader of shader model 5 (the one pass's ldms / ld_indexable).
    if (((khpb_t[0] >> 16) & 0xFFFFu) != 0u || ((khpb_t[0] >> 4) & 0xFu) != 5u || khpb_len < 3u ||
        khpb_len > khpb_t.size()) return KH_PSV_R_PROGRAM;
    // The input signature's SV_Position (system value 1): its register and the element's used mask.
    if (khpb_ii == khpb_cn) return KH_PSB_R_NOPOS;
    const size_t khpb_io = kh_psv_get32(khpb_b + 32 + 4 * khpb_ii) + 8u;
    const size_t khpb_in = kh_psv_get32(khpb_b + khpb_io - 4u);
    if (khpb_in < 8u || kh_psv_get32(khpb_b + khpb_io + 4) != 8u) return KH_PSB_R_SIG;
    const uint32_t khpb_ie = kh_psv_get32(khpb_b + khpb_io);
    if (khpb_ie > 64u || 8u + 24u * static_cast<size_t>(khpb_ie) > khpb_in) return KH_PSB_R_SIG;
    uint32_t khpb_pr = 0xFFFFFFFFu;   // SV_Position's register.
    size_t khpb_pe = 0;               // Its element's offset in the chunk's payload.
    for (uint32_t khpb_e = 0; khpb_e < khpb_ie; ++khpb_e) {
        const size_t khpb_eo = khpb_io + 8u + 24u * khpb_e;
        if (kh_psv_get32(khpb_b + khpb_eo + 8) != 1u) continue;
        if (khpb_pr != 0xFFFFFFFFu) return KH_PSB_R_SIG;
        if (kh_psv_get32(khpb_b + khpb_eo + 12) != 3u || (khpb_b[khpb_eo + 20] & 7u) != 7u ||
            kh_psv_get32(khpb_b + khpb_eo + 16) >= 32u) return KH_PSB_R_NOPOS;
        khpb_pr = kh_psv_get32(khpb_b + khpb_eo + 16);
        khpb_pe = khpb_eo - khpb_io;
    }
    if (khpb_pr == 0xFFFFFFFFu) return KH_PSB_R_NOPOS;
    for (uint32_t khpb_e = 0; khpb_e < khpb_ie; ++khpb_e) {   // KH_PSB_LORES: SV_Position's register its own.
        const size_t khpb_eo = khpb_io + 8u + 24u * khpb_e;
        if (khpb_eo - khpb_io != khpb_pe && kh_psv_get32(khpb_b + khpb_eo + 16) == khpb_pr) return KH_PSB_R_SIG;
    }
    // The declarations: their spans, the slots they take, the temps, the position input and o0.
    struct KhPsbDcl { size_t at; size_t ln; uint32_t op; };
    std::vector<KhPsbDcl> khpb_dl;
    bool khpb_cbu[14] = {};
    bool khpb_tu[128] = {};
    size_t khpb_tmp = 0;      // dcl_temps' count position (0 = none).
    size_t khpb_pos = 0;      // The position input's operand token (0 = not declared).
    size_t khpb_o0 = 0;       // o0's operand token.
    size_t khpb_i = 2;
    while (khpb_i < khpb_len) {
        const uint32_t khpb_tk = khpb_t[khpb_i];
        const uint32_t khpb_op = khpb_tk & 0x7FFu;
        size_t khpb_ln = 0;
        if (khpb_op == 53u) {
            if (khpb_i + 1u >= khpb_len) return KH_PSV_R_TOKENS;
            khpb_ln = khpb_t[khpb_i + 1];
            if (khpb_ln < 2u) return KH_PSV_R_TOKENS;
        } else {
            khpb_ln = (khpb_tk >> 24) & 0x7Fu;
        }
        if (khpb_ln == 0u || khpb_ln > khpb_len - khpb_i) return KH_PSV_R_TOKENS;
        const bool khpb_dcl = khpb_op == 53u || (khpb_op >= 88u && khpb_op <= 106u) ||
                              (khpb_op >= 143u && khpb_op <= 162u) || (khpb_op >= 113u && khpb_op <= 116u) ||
                              khpb_op == 206u;
        if (!khpb_dcl) break;
        if (khpb_op == 144u || khpb_op == 145u || khpb_op == 146u) return KH_PSV_R_LINKAGE;
        if (khpb_op >= 156u && khpb_op <= 160u) return KH_PSV_R_UAV;
        const uint32_t khpb_d = khpb_ln >= 2u ? khpb_t[khpb_i + 1] : 0u;
        const uint32_t khpb_ty = (khpb_d >> 12) & 0xFFu;
        if (khpb_op == 89u || khpb_op == 88u || khpb_op == 161u || khpb_op == 162u) {
            // A slot taken (constant buffer: [slot][size]; resource: [slot]) - an immediate index past no
            // extended operand token.
            if (khpb_ln < 3u || (khpb_d >> 31) != 0u || ((khpb_d >> 22) & 7u) != 0u) return KH_PSV_R_TOKENS;
            const uint32_t khpb_s = khpb_t[khpb_i + 2];
            if (khpb_op == 89u) {
                if (((khpb_d >> 20) & 3u) != 2u) return KH_PSV_R_TOKENS;
                if (khpb_s < 14u) khpb_cbu[khpb_s] = true;
            } else {
                if (((khpb_d >> 20) & 3u) != 1u) return KH_PSV_R_TOKENS;
                if (khpb_s < 128u) khpb_tu[khpb_s] = true;
            }
        } else if (khpb_op == 104u) {
            if (khpb_ln != 2u || khpb_tmp != 0u) return KH_PSV_R_TOKENS;
            khpb_tmp = khpb_i + 1;
        } else if (khpb_op == 95u) {
            if (khpb_ty == 35u || khpb_ty == 42u) return KH_PSB_R_RATE;   // vCoverage, vInnerCoverage.
        } else if (khpb_op >= 98u && khpb_op <= 100u) {
            const uint32_t khpb_im = (khpb_tk >> 11) & 0xFu;
            if (khpb_im == 6u || khpb_im == 7u) return KH_PSB_R_RATE;   // Interpolated at each sample.
            if (khpb_ln < 3u || khpb_ty != 1u || (khpb_d >> 31) != 0u || ((khpb_d >> 20) & 3u) != 1u ||
                ((khpb_d >> 22) & 7u) != 0u || (khpb_d & 3u) != 2u || ((khpb_d >> 2) & 3u) != 0u) {
                return KH_PSV_R_TOKENS;
            }
            if (khpb_op == 99u && khpb_ln >= 4u && khpb_t[khpb_i + 3] == 10u) return KH_PSB_R_RATE;   // SampleIndex.
            if (khpb_t[khpb_i + 2] == khpb_pr) {
                if (khpb_op != 100u || khpb_ln < 4u || khpb_t[khpb_i + 3] != 1u || khpb_pos != 0u) return KH_PSB_R_SIG;
                if (((khpb_d >> 4) & 4u) != 0u) return KH_PSB_R_POSZ;
                khpb_pos = khpb_i + 1;
            }
        } else if (khpb_op == 91u) {
            if (khpb_ty == 2u) return KH_PSV_R_RELOUT;
            if (khpb_ty == 1u) return KH_PSB_R_SIG;   // An input range: the position's register may lie in it.
        } else if (khpb_op >= 101u && khpb_op <= 103u) {
            if (khpb_ty == 12u || khpb_ty == 15u || khpb_ty == 38u || khpb_ty == 39u || khpb_ty == 41u) {
                return KH_PSV_R_DEPTHOUT;
            }
            if (khpb_ty != 2u || khpb_op != 101u || khpb_ln != 3u || khpb_o0 != 0u) return KH_PSB_R_SIG;
            if (khpb_t[khpb_i + 2] != 0u || (khpb_d & 3u) != 2u || ((khpb_d >> 2) & 3u) != 0u ||
                (khpb_d >> 31) != 0u) return KH_PSB_R_SIG;
            if (((khpb_d >> 4) & 8u) == 0u) return KH_PSV_R_NOALPHA;
            khpb_o0 = khpb_i + 1;
        }
        khpb_dl.push_back(KhPsbDcl{ khpb_i, khpb_ln, khpb_op });
        khpb_i += khpb_ln;
    }
    if (khpb_o0 == 0u) return KH_PSV_R_NOALPHA;
    // The free slots: the highest of each (cb13, t127 where the engine leaves them).
    uint32_t khpb_k = 14u, khpb_ts0 = 128u;
    for (uint32_t khpb_s = 14u; khpb_s-- > 0u;) {
        if (!khpb_cbu[khpb_s]) { khpb_k = khpb_s; break; }
    }
    for (uint32_t khpb_s = 128u; khpb_s-- > 0u;) {
        if (!khpb_tu[khpb_s]) { khpb_ts0 = khpb_s; break; }
    }
    uint32_t khpb_tz0 = 128u;   // KH_PSB_ZB: Zb's, the next free below.
    for (uint32_t khpb_s = khpb_ts0 < 128u ? khpb_ts0 : 0u; khpb_s-- > 0u;) {
        if (!khpb_tu[khpb_s]) { khpb_tz0 = khpb_s; break; }
    }
    if (khpb_k >= 14u || khpb_ts0 >= 128u || khpb_tz0 >= 128u) return KH_PSB_R_SLOT;
    const uint32_t khpb_nt = khpb_tmp ? khpb_t[khpb_tmp] : 0u;
    if (khpb_nt > 4096u - KH_PSB_TEMPS) return KH_PSV_R_TOKENS;
    const uint32_t khpb_ra = khpb_nt, khpb_rb = khpb_nt + 1u, khpb_rc = khpb_nt + 2u, khpb_rd = khpb_nt + 3u;
    // The body: o0.w redirected into ra.w, o0.xyz and every other colour write dropped (the alpha variant's rule,
    // a dropped destination of a two-destination op nulled), the epilogue inlined before every ret (no SM5
    // subroutine: fxc emits none without [call], so drivers rarely meet them) and a retc made "if c - epilogue - ret
    // - endif" (the same test, the same c). The epilogue writes the verdicts the prologue made (below):
    //   and o0.xyzw, ra.wwww, rc.yzwx / mov o1.x, rb.xxxx (the phase's (T, 25 % band, 100 % band, Tf) and its NW).
    const uint32_t khpb_ep[12] = {
        1u | (7u << 24), kh_psb_op(2u, 0u, 0xFu, 1u), 0u, kh_psb_op(0u, 1u, 0xFFu, 1u), khpb_ra,
        kh_psb_op(0u, 1u, 0x39u, 1u), khpb_rc,
        54u | (5u << 24), kh_psb_op(2u, 0u, 0x1u, 1u), 1u, kh_psb_op(0u, 1u, 0x00u, 1u), khpb_rb };
    uint32_t khpb_nret = 0u;
    std::vector<uint32_t> khpb_x;
    khpb_x.reserve(khpb_len - khpb_i + 64u);
    uint32_t khpb_last = 0u;   // The last instruction's opcode (the program must end with ret).
    while (khpb_i < khpb_len) {
        const uint32_t khpb_tk = khpb_t[khpb_i];
        const uint32_t khpb_op = khpb_tk & 0x7FFu;
        size_t khpb_ln = 0;
        if (khpb_op == 53u) {
            if (khpb_i + 1u >= khpb_len) return KH_PSV_R_TOKENS;
            khpb_ln = khpb_t[khpb_i + 1];
            if (khpb_ln < 2u) return KH_PSV_R_TOKENS;
        } else {
            khpb_ln = (khpb_tk >> 24) & 0x7Fu;
        }
        if (khpb_ln == 0u || khpb_ln > khpb_len - khpb_i) return KH_PSV_R_TOKENS;
        const bool khpb_dcl = khpb_op == 53u || (khpb_op >= 88u && khpb_op <= 106u) ||
                              (khpb_op >= 143u && khpb_op <= 162u) || (khpb_op >= 113u && khpb_op <= 116u) ||
                              khpb_op == 206u;
        if (khpb_dcl) return KH_PSV_R_TOKENS;   // A declaration past the first instruction.
        if (khpb_op == 120u) return KH_PSV_R_LINKAGE;
        if (khpb_op == 4u || khpb_op == 5u || khpb_op == 44u) return KH_PSB_R_SUB;
        if (khpb_op >= 219u && khpb_op <= 233u) return KH_PSV_R_FEEDBACK;
        if (khpb_op == 110u || khpb_op == 111u || (khpb_op >= 203u && khpb_op <= 205u)) return KH_PSB_R_RATE;
        khpb_last = khpb_op;
        size_t khpb_j = khpb_i + 1;
        if ((khpb_tk >> 31) != 0u) {
            bool khpb_e = true;
            while (khpb_e) {
                if (khpb_j >= khpb_i + khpb_ln) return KH_PSV_R_TOKENS;
                khpb_e = (khpb_t[khpb_j] >> 31) != 0u;
                ++khpb_j;
            }
        }
        // KH_PSB_LORES: the engine's own reads of the position register take rd (SV_Position at the frame's
        // resolution: the prologue's end), in the token copy, before the rules below read the instruction.
        if (khpb_pos != 0u) {
            for (size_t khpb_f = khpb_j; khpb_f < khpb_i + khpb_ln;) {
                const size_t khpb_fl = kh_psv_oplen(khpb_t.data(), khpb_f, khpb_i + khpb_ln);
                if (khpb_fl == 0u) return KH_PSV_R_TOKENS;
                if (!kh_psb_pos_fix(khpb_t.data(), khpb_f, khpb_f + khpb_fl, khpb_pr, khpb_rd)) return KH_PSB_R_POSX;
                khpb_f += khpb_fl;
            }
        }
        if (khpb_op == 62u || khpb_op == 63u) {
            if (++khpb_nret > KH_PSB_RET_MAX) return KH_PSB_R_RETS;
        }
        if (khpb_op == 62u) {   // ret: the epilogue first.
            if (khpb_ln != 1u) return KH_PSV_R_TOKENS;
            khpb_x.insert(khpb_x.end(), khpb_ep, khpb_ep + 12);
            khpb_x.push_back(khpb_tk);
            khpb_i += khpb_ln;
            continue;
        }
        if (khpb_op == 63u) {   // retc_z / retc_nz c: if_z / if_nz c, the epilogue, ret, endif.
            if (khpb_j != khpb_i + 1u) return KH_PSV_R_TOKENS;
            const size_t khpb_cl = kh_psv_oplen(khpb_t.data(), khpb_j, khpb_i + khpb_ln);
            if (khpb_cl == 0u || khpb_j + khpb_cl != khpb_i + khpb_ln) return KH_PSV_R_TOKENS;
            khpb_x.push_back(31u | (khpb_tk & (1u << 18)) | (static_cast<uint32_t>(1u + khpb_cl) << 24));
            khpb_x.insert(khpb_x.end(), khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_j),
                          khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_j + khpb_cl));
            khpb_x.insert(khpb_x.end(), khpb_ep, khpb_ep + 12);
            khpb_x.push_back(62u | (1u << 24));
            khpb_x.push_back(21u | (1u << 24));
            khpb_i += khpb_ln;
            continue;
        }
        const bool khpb_two = khpb_op == 77u || khpb_op == 78u || khpb_op == 81u || khpb_op == 38u ||
                              khpb_op == 132u || khpb_op == 133u || khpb_op == 142u;
        size_t khpb_dp[2] = { 0, 0 };
        size_t khpb_dln[2] = { 0, 0 };
        uint32_t khpb_dty[2] = { 0xFFu, 0xFFu };
        uint32_t khpb_reg[2] = { 0u, 0u };
        uint32_t khpb_msk[2] = { 0u, 0u };
        size_t khpb_rx[2] = { 0, 0 };   // The register index's position.
        bool khpb_any = false;
        int khpb_kk = 0;
        while (khpb_j < khpb_i + khpb_ln) {
            const size_t khpb_ol = kh_psv_oplen(khpb_t.data(), khpb_j, khpb_i + khpb_ln);
            if (khpb_ol == 0u) return KH_PSV_R_TOKENS;
            const uint32_t khpb_q = khpb_t[khpb_j];
            const uint32_t khpb_qt = (khpb_q >> 12) & 0xFFu;
            if (khpb_kk < 2) {
                khpb_dp[khpb_kk] = khpb_j;
                khpb_dln[khpb_kk] = khpb_ol;
                khpb_dty[khpb_kk] = khpb_qt;
            }
            if (khpb_qt == 2u) {
                if (khpb_kk > 1 || (khpb_kk == 1 && !khpb_two)) return KH_PSV_R_OUTREAD;
                const size_t khpb_x0 = kh_psv_reg0(khpb_t.data(), khpb_j, khpb_j + khpb_ol);
                if (khpb_x0 == 0u) return KH_PSV_R_RELOUT;
                if ((khpb_q & 3u) != 2u || ((khpb_q >> 2) & 3u) != 0u) return KH_PSV_R_TOKENS;
                khpb_reg[khpb_kk] = khpb_t[khpb_x0];
                khpb_msk[khpb_kk] = (khpb_q >> 4) & 0xFu;
                khpb_rx[khpb_kk] = khpb_x0;
                khpb_any = true;
            }
            khpb_j += khpb_ol;
            ++khpb_kk;
        }
        if (khpb_j != khpb_i + khpb_ln) return KH_PSV_R_TOKENS;
        if (!khpb_any) {
            khpb_x.insert(khpb_x.end(), khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_i),
                          khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_i + khpb_ln));
            khpb_i += khpb_ln;
            continue;
        }
        const int khpb_nd = khpb_two ? 2 : 1;
        bool khpb_keep[2] = { false, false };
        bool khpb_live = false;
        for (int khpb_d = 0; khpb_d < khpb_nd; ++khpb_d) {
            if (khpb_dty[khpb_d] == 2u) {
                khpb_keep[khpb_d] = khpb_reg[khpb_d] == 0u && (khpb_msk[khpb_d] & 8u) != 0u;
                if (khpb_keep[khpb_d]) khpb_live = true;
            } else if (khpb_dty[khpb_d] != 13u) {
                khpb_live = true;
            }
        }
        if (khpb_live) {
            const size_t khpb_at = khpb_x.size();
            khpb_x.push_back(0u);
            for (size_t khpb_y = khpb_i + 1; khpb_y < khpb_i + khpb_ln;) {
                int khpb_d = -1;
                if (khpb_y == khpb_dp[0] && khpb_dty[0] == 2u) khpb_d = 0;
                if (khpb_two && khpb_y == khpb_dp[1] && khpb_dty[1] == 2u) khpb_d = 1;
                if (khpb_d < 0) {
                    khpb_x.push_back(khpb_t[khpb_y]);
                    ++khpb_y;
                } else if (khpb_keep[khpb_d]) {
                    // o0.w -> ra.w: the operand's type and mask, then its extended tokens, then the register.
                    khpb_x.push_back((khpb_t[khpb_y] & ~(0xFFu << 12) & ~(0xFu << 4)) | (8u << 4));
                    for (size_t khpb_z = khpb_y + 1; khpb_z < khpb_rx[khpb_d]; ++khpb_z) {
                        khpb_x.push_back(khpb_t[khpb_z]);
                    }
                    khpb_x.push_back(khpb_ra);
                    if (khpb_rx[khpb_d] + 1u != khpb_y + khpb_dln[khpb_d]) return KH_PSV_R_TOKENS;
                    khpb_y += khpb_dln[khpb_d];
                } else {
                    khpb_x.push_back(13u << 12);   // null.
                    khpb_y += khpb_dln[khpb_d];
                }
            }
            const size_t khpb_nl = khpb_x.size() - khpb_at;
            khpb_x[khpb_at] = (khpb_tk & ~(0x7Fu << 24)) | (static_cast<uint32_t>(khpb_nl) << 24);
        }
        khpb_i += khpb_ln;
    }
    if (khpb_i != khpb_len || khpb_last != 62u) return KH_PSV_R_PROGRAM;
    // The output signature: one float SV_Target0 (register 0) with .w.
    if (khpb_oi == khpb_cn) return KH_PSB_R_SIG;
    {
        const size_t khpb_oo = kh_psv_get32(khpb_b + 32 + 4 * khpb_oi) + 8u;
        const size_t khpb_on = kh_psv_get32(khpb_b + khpb_oo - 4u);
        if (khpb_on < 32u || kh_psv_get32(khpb_b + khpb_oo) != 1u || kh_psv_get32(khpb_b + khpb_oo + 4) != 8u) {
            return KH_PSB_R_SIG;
        }
        const size_t khpb_nm = kh_psv_get32(khpb_b + khpb_oo + 8);
        if (kh_psv_get32(khpb_b + khpb_oo + 12) != 0u || kh_psv_get32(khpb_b + khpb_oo + 16) != 0u ||
            kh_psv_get32(khpb_b + khpb_oo + 20) != 3u || kh_psv_get32(khpb_b + khpb_oo + 24) != 0u ||
            khpb_nm + 10u > khpb_on) return KH_PSB_R_SIG;
        if ((khpb_b[khpb_oo + 28] & 8u) == 0u) return KH_PSV_R_NOALPHA;
        static const char khpb_sv[10] = { 'S', 'V', '_', 'T', 'A', 'R', 'G', 'E', 'T', 0 };
        for (size_t khpb_c = 0; khpb_c < 10u; ++khpb_c) {
            const char khpb_ch = static_cast<char>(khpb_b[khpb_oo + khpb_nm + khpb_c]);
            const char khpb_up = khpb_ch >= 'a' && khpb_ch <= 'z' ? static_cast<char>(khpb_ch - 32) : khpb_ch;
            if (khpb_up != khpb_sv[khpb_c]) return KH_PSB_R_SIG;
        }
    }
    khpb_cbs = khpb_k;
    khpb_ts = khpb_ts0;
    khpb_tzs = khpb_tz0;   // KH_PSB_ZB.
    // Our declarations, each where fxc puts its kind (constant buffers, resources, inputs, outputs, temps).
    const uint32_t khpb_dcb[4] = { 89u | (4u << 24), kh_psb_op(8u, 1u, 0xE4u, 2u), khpb_k, 4u };   // KH_PSB_LORES: 4.
    const uint32_t khpb_dt[4] = { 88u | (4u << 11) | (4u << 24), (7u << 12) | (1u << 20), khpb_ts0, 0x5555u };
    // KH_PSB_ZB: dcl_resource_texture2d (float,float,float,float) tZ - Zb.
    const uint32_t khpb_dz[4] = { 88u | (3u << 11) | (4u << 24), (7u << 12) | (1u << 20), khpb_tz0, 0x5555u };
    // The position's x, y and z (the prologue reads all three: the pixel's address and its depth).
    const uint32_t khpb_dp0[4] = { 100u | (4u << 11) | (4u << 24), kh_psb_op(1u, 0u, 7u, 1u), khpb_pr, 1u };
    const uint32_t khpb_do[3] = { 101u | (3u << 24), kh_psb_op(2u, 0u, 0x1u, 1u), 1u };   // KH_PSB_PHASE: o1.x.
    const uint32_t khpb_dtm[2] = { 104u | (2u << 24), khpb_nt + KH_PSB_TEMPS };
    std::vector<uint32_t> khpb_p;   // The new program.
    khpb_p.reserve(khpb_len + 160u);
    khpb_p.push_back(khpb_t[0]);
    khpb_p.push_back(0u);
    bool khpb_w[5] = { false, false, khpb_pos != 0u, false, khpb_tmp != 0u };   // cb, t, position, outputs, temps.
    auto khpb_rank = [](uint32_t khrk_op) -> int {
        if (khrk_op == 106u) return 0;
        if (khrk_op == 53u) return 1;
        if (khrk_op == 89u) return 2;
        if (khrk_op == 90u) return 3;
        if (khrk_op == 88u || khrk_op == 161u || khrk_op == 162u) return 4;
        if (khrk_op >= 95u && khrk_op <= 100u) return 5;
        if (khrk_op >= 101u && khrk_op <= 103u) return 6;
        if (khrk_op == 104u) return 7;
        return 8;
    };
    auto khpb_put = [&](int khpu_r) {   // Ours whose kind ranks before khpu_r, not yet written.
        if (!khpb_w[0] && khpu_r > 2) { khpb_p.insert(khpb_p.end(), khpb_dcb, khpb_dcb + 4); khpb_w[0] = true; }
        if (!khpb_w[1] && khpu_r > 4) {   // KH_PSB_ZB: Zb's first (the lower slot), then the depth's.
            khpb_p.insert(khpb_p.end(), khpb_dz, khpb_dz + 4);
            khpb_p.insert(khpb_p.end(), khpb_dt, khpb_dt + 4);
            khpb_w[1] = true;
        }
        if (!khpb_w[2] && khpu_r >= 5) { khpb_p.insert(khpb_p.end(), khpb_dp0, khpb_dp0 + 4); khpb_w[2] = true; }
        if (!khpb_w[3] && khpu_r > 6) { khpb_p.insert(khpb_p.end(), khpb_do, khpb_do + 3); khpb_w[3] = true; }
        if (!khpb_w[4] && khpu_r > 7) { khpb_p.insert(khpb_p.end(), khpb_dtm, khpb_dtm + 2); khpb_w[4] = true; }
    };
    for (const KhPsbDcl& khpb_e : khpb_dl) {
        const int khpb_rk = khpb_rank(khpb_e.op);
        khpb_put(khpb_rk);
        const size_t khpb_at = khpb_p.size();
        khpb_p.insert(khpb_p.end(), khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_e.at),
                      khpb_t.begin() + static_cast<std::ptrdiff_t>(khpb_e.at + khpb_e.ln));
        if (khpb_e.at + 1u == khpb_pos) khpb_p[khpb_at + 1] |= 7u << 4;                    // + .xyz
        if (khpb_e.at + 1u == khpb_o0) khpb_p[khpb_at + 1] |= 0xFu << 4;                   // o0.xyzw
        if (khpb_e.at + 1u == khpb_tmp) khpb_p[khpb_at + 1] = khpb_nt + KH_PSB_TEMPS;
        if (khpb_e.op >= 101u && khpb_e.op <= 103u && !khpb_w[3]) {   // Ours right after o0.
            khpb_p.insert(khpb_p.end(), khpb_do, khpb_do + 3);
            khpb_w[3] = true;
        }
    }
    khpb_put(9);
    // The prologue. Registers: ra.w the alpha; rb = (z_1, z_2, z_3, z); rc = (r, k, scratch, the loop's bound), then
    // the verdicts (pF, pT, p1, p2); rd the read's base (xy the block's corner - the address is ra.xyww -, z k0) and
    // scratch, after the early-out the engine's SV_Position substitute. cb[0] = (m_1, m_2, m_3, Dq), cb[1] = (a_1, a_2,
    // a_3, D raw), cb[2] = (test-off mask, Tf mask, bands mask, reads: s s), cb[3] = (log2 s, s - 1, sample mask, Zb
    // flag) - kh_psb_post uploads the same layout (TWIN).
    // r = the largest sample 0 of the block's s x s pixels (samples above 0 unread: an edge inside the block counts
    // the farther side for the whole block). The loop stops once r reaches every bound it is needed for: each tested
    // push, and Dq where z > D (pF is false at z <= D whatever r is); the test-off kind nearer than D reads nothing.
    // The verdicts stay live through the engine's code. pF = z > D (the Tf pass's GREATER) and r >= Dq (the float
    // just above D - KH_PSB_TF_EPS where Tf is drawn, 0 where not; KH_PSB_TF_EPS's note). Early-out: a fragment no
    // verdict passes with z_1 >= 1 (its NW MIN against N <= 1 is a no-op; the cloud deck's [1, 1] viewport over
    // nearer geometry) writes nothing and is discarded before the engine's code; every other fragment runs on (NW is
    // read behind the live depth too, so a hidden fragment's NW counts). KH_PSB_ZB: under cb3.w only the terms
    // t <= Zb (the block's largest S, tZ at the low-res pixel) stay in the bound and the reads start at Zb's pixel
    // k0: the main depth only comes nearer within a cycle (the live maximum M <= Zb; kh_psb_zb_trip clears the flag
    // wherever that could fail), so a term past Zb is false whatever the loop reads, and a kept term ends the loop
    // (r >= t) only where M >= t, or the loop runs out with r = M - each verdict is the full maximum's, as without.
    // No temp beyond ra.xy and rd.zw.
    {
        const uint32_t khpb_kt = 0u, khpb_ki = 1u, khpb_kl = 4u, khpb_kr = 7u, khpb_kc = 8u;
        const uint32_t khpb_cbx = kh_psb_op(khpb_kc, 1u, 0x24u, 2u);   // cb[k].xyzx
        auto khpb_c1 = [&](uint32_t khc1_e, uint32_t khc1_c) {   // cb[k][e].c (one component)
            const uint32_t khc1_t[3] = { kh_psb_op(khpb_kc, 2u, khc1_c, 2u), khpb_k, khc1_e };
            khpb_p.insert(khpb_p.end(), khc1_t, khc1_t + 3);
        };
        auto khpb_r = [&](uint32_t khr_kind, uint32_t khr_sel, uint32_t khr_reg) {
            khpb_p.push_back(kh_psb_op(khpb_kt, khr_kind, khr_sel, 1u));
            khpb_p.push_back(khr_reg);
        };
        auto khpb_ins = [&](uint32_t khi_op, uint32_t khi_len) { khpb_p.push_back(khi_op | (khi_len << 24)); };
        auto khpb_cw = [&](uint32_t khcw_e) {   // cb[k][e].wwww
            const uint32_t khcw_t[3] = { kh_psb_op(khpb_kc, 1u, 0xFFu, 2u), khpb_k, khcw_e };
            khpb_p.insert(khpb_p.end(), khcw_t, khcw_t + 3);
        };
        const uint32_t khpb_sx = 0x00u, khpb_sy = 0x55u, khpb_sz = 0xAAu, khpb_sw = 0xFFu;
        khpb_ins(54u, 5u);                                                    // mov ra.w, l(0)
        khpb_r(0u, 0x8u, khpb_ra);
        khpb_p.push_back(1u | (khpb_kl << 12));
        khpb_p.push_back(0u);
        khpb_ins(50u, 11u);                                                   // mad rb.xyz, v.zzzz, cb0.xyzx, cb1.xyzx
        khpb_r(0u, 0x7u, khpb_rb);
        khpb_p.push_back(kh_psb_op(khpb_ki, 1u, khpb_sz, 1u));
        khpb_p.push_back(khpb_pr);
        khpb_p.push_back(khpb_cbx);
        khpb_p.push_back(khpb_k);
        khpb_p.push_back(0u);
        khpb_p.push_back(khpb_cbx);
        khpb_p.push_back(khpb_k);
        khpb_p.push_back(1u);
        khpb_ins(54u, 5u);                                                    // mov rb.w, v.zzzz
        khpb_r(0u, 0x8u, khpb_rb);
        khpb_p.push_back(kh_psb_op(khpb_ki, 1u, khpb_sz, 1u));
        khpb_p.push_back(khpb_pr);
        khpb_ins(52u, 7u);                                                    // max rc.w, rb.xxxx, rb.yyyy
        khpb_r(0u, 0x8u, khpb_rc);
        khpb_r(1u, khpb_sx, khpb_rb);
        khpb_r(1u, khpb_sy, khpb_rb);
        khpb_ins(52u, 7u);                                                    // max rc.w, rc.wwww, rb.zzzz
        khpb_r(0u, 0x8u, khpb_rc);
        khpb_r(1u, khpb_sw, khpb_rc);
        khpb_r(1u, khpb_sz, khpb_rb);
        khpb_ins(1u, 8u);                                                     // and rc.w, rc.wwww, cb2.zzzz
        khpb_r(0u, 0x8u, khpb_rc);
        khpb_r(1u, khpb_sw, khpb_rc);
        khpb_p.push_back(kh_psb_op(khpb_kc, 1u, khpb_sz, 2u));
        khpb_p.push_back(khpb_k);
        khpb_p.push_back(2u);
        // Dq joins the bound only where the fragment's own far test passes (pF needs D < z; elsewhere the loop keeps
        // its band breaks, and the test-off kind reads no sample).
        khpb_ins(49u, 8u);                                                    // lt rc.z, cb1.w, rb.w (D < z)
        khpb_r(0u, 0x4u, khpb_rc);
        khpb_c1(1u, 3u);
        khpb_r(2u, 3u, khpb_rb);
        khpb_ins(1u, 8u);                                                     // and rc.z, rc.z, cb0.w (Dq or +0)
        khpb_r(0u, 0x4u, khpb_rc);
        khpb_r(2u, 2u, khpb_rc);
        khpb_c1(0u, 3u);
        khpb_ins(52u, 7u);                                                    // max rc.w, rc.wwww, rc.zzzz
        khpb_r(0u, 0x8u, khpb_rc);
        khpb_r(1u, khpb_sw, khpb_rc);
        khpb_r(1u, khpb_sz, khpb_rc);
        // KH_PSB_ZB: under cb3.w (Zb stands), the bound keeps only the terms Zb - the block's largest S, a bound on
        // the live maximum - can reach (each of z_1..3 masked by the bands, and the Dq term in rc.z), the rest decided
        // false without a read; rd.z = k0, Zb's argmax pixel, the loop's first read (0 under no flag). ra.xy and
        // rd.zw are free here (ra.w stays the address's 0).
        khpb_ins(54u, 5u);                                                    // mov rd.z, l(0)
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_p.push_back(1u | (khpb_kl << 12));
        khpb_p.push_back(0u);
        khpb_ins(31u | (1u << 18), 4u);                                       // if_nz cb3.w
        khpb_c1(3u, 3u);
        khpb_ins(28u, 5u);                                                    //   ftou ra.xy, v.xyxx
        khpb_r(0u, 0x3u, khpb_ra);
        khpb_p.push_back(kh_psb_op(khpb_ki, 1u, 0x04u, 1u));
        khpb_p.push_back(khpb_pr);
        // ld_indexable(texture2d)(float,float,float,float) ra.xy, ra.xyww, tZ.xyxx (Zb's texel: max S, k0)
        khpb_p.push_back(45u | (9u << 24) | (1u << 31));
        khpb_p.push_back(0x800000C2u);
        khpb_p.push_back(0x00155543u);
        khpb_r(0u, 0x3u, khpb_ra);
        khpb_r(1u, 0xF4u, khpb_ra);
        khpb_p.push_back(kh_psb_op(khpb_kr, 1u, 0x04u, 1u));
        khpb_p.push_back(khpb_tz0);
        khpb_ins(29u, 7u);                                                    //   ge rd.zw, ra.xxxx, rb.xxxy
        khpb_r(0u, 0xCu, khpb_rd);
        khpb_r(1u, khpb_sx, khpb_ra);
        khpb_r(1u, 0x40u, khpb_rb);
        khpb_ins(1u, 7u);                                                     //   and rd.zw, rd.zzzw, rb.xxxy
        khpb_r(0u, 0xCu, khpb_rd);
        khpb_r(1u, 0xEAu, khpb_rd);
        khpb_r(1u, 0x40u, khpb_rb);
        khpb_ins(52u, 7u);                                                    //   max rd.z, rd.z, rd.w
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 2u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_ins(29u, 7u);                                                    //   ge rd.w, ra.x, rb.z
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 0u, khpb_ra);
        khpb_r(2u, 2u, khpb_rb);
        khpb_ins(1u, 7u);                                                     //   and rd.w, rd.w, rb.z
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_r(2u, 2u, khpb_rb);
        khpb_ins(52u, 7u);                                                    //   max rd.z, rd.z, rd.w
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 2u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_ins(1u, 8u);                                                     //   and rd.z, rd.z, cb2.z (bands)
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 2u, khpb_rd);
        khpb_c1(2u, 2u);
        khpb_ins(29u, 7u);                                                    //   ge rd.w, ra.x, rc.z (Dq term)
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 0u, khpb_ra);
        khpb_r(2u, 2u, khpb_rc);
        khpb_ins(1u, 7u);                                                     //   and rd.w, rd.w, rc.z
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_r(2u, 2u, khpb_rc);
        khpb_ins(52u, 7u);                                                    //   max rc.w, rd.z, rd.w (the bound)
        khpb_r(0u, 0x8u, khpb_rc);
        khpb_r(2u, 2u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_ins(28u, 5u);                                                    //   ftou rd.z, ra.y (k0)
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 1u, khpb_ra);
        khpb_ins(21u, 1u);                                                    // endif
        khpb_ins(28u, 5u);                                                    // ftou rd.xy, v.xyxx
        khpb_r(0u, 0x3u, khpb_rd);
        khpb_p.push_back(kh_psb_op(khpb_ki, 1u, 0x04u, 1u));
        khpb_p.push_back(khpb_pr);
        // KH_PSB_LORES: the block's corner at the frame's resolution (cb3.x = log2 s; s = 1: the pixel itself); the
        // address is ra.xyww (ra.w is 0 until the engine's code).
        khpb_ins(41u, 8u);                                                    // ishl rd.xy, rd.xyxx, cb3.xxxx
        khpb_r(0u, 0x3u, khpb_rd);
        khpb_r(1u, 0x04u, khpb_rd);
        khpb_p.push_back(kh_psb_op(khpb_kc, 1u, khpb_sx, 2u));
        khpb_p.push_back(khpb_k);
        khpb_p.push_back(3u);
        khpb_ins(54u, 8u);                                                    // mov rc.xy, l(0, 0, 0, 0)
        khpb_r(0u, 0x3u, khpb_rc);
        khpb_p.push_back(2u | (khpb_kl << 12));
        for (int khpb_q = 0; khpb_q < 4; ++khpb_q) khpb_p.push_back(0u);
        khpb_ins(48u, 1u);                                                    // loop
        khpb_ins(29u, 7u);                                                    // ge rc.z, rc.xxxx, rc.wwww
        khpb_r(0u, 0x4u, khpb_rc);
        khpb_r(1u, khpb_sx, khpb_rc);
        khpb_r(1u, khpb_sw, khpb_rc);
        khpb_p.push_back(3u | (1u << 18) | (3u << 24));                       // breakc_nz rc.z
        khpb_r(2u, 2u, khpb_rc);
        khpb_ins(80u, 8u);                                                    // uge rc.z, rc.yyyy, cb2.w
        khpb_r(0u, 0x4u, khpb_rc);
        khpb_r(1u, khpb_sy, khpb_rc);
        khpb_c1(2u, 3u);
        khpb_p.push_back(3u | (1u << 18) | (3u << 24));                       // breakc_nz rc.z
        khpb_r(2u, 2u, khpb_rc);
        // KH_PSB_LORES: the k-th read's pixel and sample - cb3 = (log2 s, s - 1, the sample mask, Zb's flag): pixel
        // corner + (k & (s - 1), (k >> log2 s) & (s - 1)), sample k & mask - s = 1: the pixel, sample k; s > 1: the
        // block's s x s pixels, sample 0 each (cb2.w = s s reads at most). KH_PSB_ZB: k + k0 in place of k (ra.z) - the
        // column / row masks take it mod s s, so k = 0 .. s s - 1 still reads every pixel, Zb's argmax first.
        khpb_ins(30u, 7u);                                                    // iadd ra.z, rc.y, rd.z
        khpb_r(0u, 0x4u, khpb_ra);
        khpb_r(2u, 1u, khpb_rc);
        khpb_r(2u, 2u, khpb_rd);
        khpb_ins(1u, 8u);                                                     // and ra.x, ra.z, cb3.y
        khpb_r(0u, 0x1u, khpb_ra);
        khpb_r(2u, 2u, khpb_ra);
        khpb_c1(3u, 1u);
        khpb_ins(85u, 8u);                                                    // ushr ra.y, ra.z, cb3.x
        khpb_r(0u, 0x2u, khpb_ra);
        khpb_r(2u, 2u, khpb_ra);
        khpb_c1(3u, 0u);
        khpb_ins(1u, 8u);                                                     // and ra.y, ra.y, cb3.y
        khpb_r(0u, 0x2u, khpb_ra);
        khpb_r(2u, 1u, khpb_ra);
        khpb_c1(3u, 1u);
        khpb_ins(30u, 7u);                                                    // iadd ra.xy, ra.xyxx, rd.xyxx
        khpb_r(0u, 0x3u, khpb_ra);
        khpb_r(1u, 0x04u, khpb_ra);
        khpb_r(1u, 0x04u, khpb_rd);
        khpb_ins(1u, 8u);                                                     // and ra.z, ra.z, cb3.z
        khpb_r(0u, 0x4u, khpb_ra);
        khpb_r(2u, 2u, khpb_ra);
        khpb_c1(3u, 2u);
        // ldms_indexable(texture2dms)(float,float,float,float) rc.z, ra.xyww, t.xxxx, ra.z (KH_PSB_LORES)
        khpb_p.push_back(46u | (11u << 24) | (1u << 31));
        khpb_p.push_back(0x80000102u);
        khpb_p.push_back(0x00155543u);
        khpb_r(0u, 0x4u, khpb_rc);
        khpb_r(1u, 0xF4u, khpb_ra);
        khpb_p.push_back(kh_psb_op(khpb_kr, 1u, khpb_sx, 1u));
        khpb_p.push_back(khpb_ts0);
        khpb_r(2u, 2u, khpb_ra);
        khpb_ins(52u, 7u);                                                    // max rc.x, rc.xxxx, rc.zzzz
        khpb_r(0u, 0x1u, khpb_rc);
        khpb_r(1u, khpb_sx, khpb_rc);
        khpb_r(1u, khpb_sz, khpb_rc);
        khpb_ins(30u, 7u);                                                    // iadd rc.y, rc.yyyy, l(1)
        khpb_r(0u, 0x2u, khpb_rc);
        khpb_r(1u, khpb_sy, khpb_rc);
        khpb_p.push_back(1u | (khpb_kl << 12));
        khpb_p.push_back(1u);
        khpb_ins(22u, 1u);                                                    // endloop
        khpb_ins(29u, 8u);                                                    // ge rd.z, rc.xxxx, cb0.wwww (r >= Dq)
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(1u, khpb_sx, khpb_rc);
        khpb_cw(0u);
        khpb_ins(49u, 8u);                                                    // lt rd.w, cb1.w, rb.w (D < z)
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_c1(1u, 3u);
        khpb_r(2u, 3u, khpb_rb);
        khpb_ins(29u, 7u);                                                    // ge rc.yzw, rc.xxxx, rb.xxyz
        khpb_r(0u, 0xEu, khpb_rc);
        khpb_r(1u, khpb_sx, khpb_rc);
        khpb_r(1u, 0x90u, khpb_rb);
        khpb_ins(60u, 8u);                                                    // or rc.yzw, rc.yyzw, cb2.xxxx
        khpb_r(0u, 0xEu, khpb_rc);
        khpb_r(1u, 0xE5u, khpb_rc);
        khpb_p.push_back(kh_psb_op(khpb_kc, 1u, khpb_sx, 2u));
        khpb_p.push_back(khpb_k);
        khpb_p.push_back(2u);
        khpb_ins(1u, 7u);                                                     // and rd.z, rd.z, rd.w
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 2u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_ins(1u, 8u);                                                     // and rc.x, rd.z, cb2.y (pF)
        khpb_r(0u, 0x1u, khpb_rc);
        khpb_r(2u, 2u, khpb_rd);
        khpb_c1(2u, 1u);
        khpb_ins(60u, 7u);                                                    // or rd.w, rc.y, rc.z
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 1u, khpb_rc);
        khpb_r(2u, 2u, khpb_rc);
        khpb_ins(60u, 7u);                                                    // or rd.w, rd.w, rc.w
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_r(2u, 3u, khpb_rc);
        khpb_ins(60u, 7u);                                                    // or rd.w, rd.w, rc.x
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_r(2u, 0u, khpb_rc);
        khpb_ins(29u, 7u);                                                    // ge rd.z, rb.x, l(1.0) (z_1 >= 1)
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 0u, khpb_rb);
        khpb_p.push_back(1u | (khpb_kl << 12));
        khpb_p.push_back(0x3F800000u);
        khpb_ins(59u, 5u);                                                    // not rd.w, rd.w
        khpb_r(0u, 0x8u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_ins(1u, 7u);                                                     // and rd.z, rd.z, rd.w
        khpb_r(0u, 0x4u, khpb_rd);
        khpb_r(2u, 2u, khpb_rd);
        khpb_r(2u, 3u, khpb_rd);
        khpb_p.push_back(13u | (1u << 18) | (3u << 24));                      // discard_nz rd.z
        khpb_r(2u, 2u, khpb_rd);
        // KH_PSB_LORES: where the engine's code reads SV_Position (its body reads rd in its place - above), rd =
        // (x, y, -, w) of a real pixel of the frame: s = 1 the fragment's own (v.xy); s > 1 the centre of the block's
        // pixel (ox, oy), ox = ((X ^ (Y >> 1)) & 1 | (X & 2)) & (s - 1), oy = ((X ^ Y ^ (Y >> 1)) & 1 | ((Y >> 1) & 2))
        // & (s - 1) (X, Y the block), so a screen-space read (a soft particle's depth, a lighting buffer) lands on a
        // texel centre and any 2 x 2 blocks hold one pixel of each quarter of the engine's 8 x 8 ordered dither, as
        // 2 x 2 pixels do at s = 1. A uniform branch: s = 1 runs the one mov (else).
        if (khpb_pos != 0u) {
            // Scratch: rb.yzw (free past the verdicts - the epilogue reads rb.x), ra.xyz (past the loop).
            auto khpb_i1 = [&](uint32_t khi1_v) {   // l(v), one component
                khpb_p.push_back(1u | (khpb_kl << 12));
                khpb_p.push_back(khi1_v);
            };
            khpb_ins(31u, 4u);                                                // if_z cb3.z (s > 1; rd.xy the corner)
            khpb_c1(3u, 2u);
            khpb_ins(85u, 8u);                                                // ushr ra.xy, rd.xyxx, cb3.xxxx
            khpb_r(0u, 0x3u, khpb_ra);
            khpb_r(1u, 0x04u, khpb_rd);
            khpb_p.push_back(kh_psb_op(khpb_kc, 1u, khpb_sx, 2u));
            khpb_p.push_back(khpb_k);
            khpb_p.push_back(3u);
            khpb_ins(85u, 7u);                                                // ushr ra.z, ra.y, l(1)
            khpb_r(0u, 0x4u, khpb_ra);
            khpb_r(2u, 1u, khpb_ra);
            khpb_i1(1u);
            khpb_ins(87u, 7u);                                                // xor rb.y, ra.x, ra.z
            khpb_r(0u, 0x2u, khpb_rb);
            khpb_r(2u, 0u, khpb_ra);
            khpb_r(2u, 2u, khpb_ra);
            khpb_ins(1u, 7u);                                                 // and rb.y, rb.y, l(1)
            khpb_r(0u, 0x2u, khpb_rb);
            khpb_r(2u, 1u, khpb_rb);
            khpb_i1(1u);
            khpb_ins(1u, 7u);                                                 // and rb.z, ra.x, l(2)
            khpb_r(0u, 0x4u, khpb_rb);
            khpb_r(2u, 0u, khpb_ra);
            khpb_i1(2u);
            khpb_ins(60u, 7u);                                                // or rb.y, rb.y, rb.z (ox)
            khpb_r(0u, 0x2u, khpb_rb);
            khpb_r(2u, 1u, khpb_rb);
            khpb_r(2u, 2u, khpb_rb);
            khpb_ins(87u, 7u);                                                // xor rb.z, ra.x, ra.y
            khpb_r(0u, 0x4u, khpb_rb);
            khpb_r(2u, 0u, khpb_ra);
            khpb_r(2u, 1u, khpb_ra);
            khpb_ins(87u, 7u);                                                // xor rb.z, rb.z, ra.z
            khpb_r(0u, 0x4u, khpb_rb);
            khpb_r(2u, 2u, khpb_rb);
            khpb_r(2u, 2u, khpb_ra);
            khpb_ins(1u, 7u);                                                 // and rb.z, rb.z, l(1)
            khpb_r(0u, 0x4u, khpb_rb);
            khpb_r(2u, 2u, khpb_rb);
            khpb_i1(1u);
            khpb_ins(1u, 7u);                                                 // and rb.w, ra.z, l(2)
            khpb_r(0u, 0x8u, khpb_rb);
            khpb_r(2u, 2u, khpb_ra);
            khpb_i1(2u);
            khpb_ins(60u, 7u);                                                // or rb.z, rb.z, rb.w (oy)
            khpb_r(0u, 0x4u, khpb_rb);
            khpb_r(2u, 2u, khpb_rb);
            khpb_r(2u, 3u, khpb_rb);
            khpb_ins(1u, 8u);                                                 // and rb.yz, rb.xyzy, cb3.yyyy
            khpb_r(0u, 0x6u, khpb_rb);
            khpb_r(1u, 0x64u, khpb_rb);
            khpb_p.push_back(kh_psb_op(khpb_kc, 1u, khpb_sy, 2u));
            khpb_p.push_back(khpb_k);
            khpb_p.push_back(3u);
            khpb_ins(30u, 7u);                                                // iadd ra.xy, rd.xyxx, rb.yzyy
            khpb_r(0u, 0x3u, khpb_ra);
            khpb_r(1u, 0x04u, khpb_rd);
            khpb_r(1u, 0x59u, khpb_rb);
            khpb_ins(86u, 5u);                                                // utof ra.xy, ra.xyxx
            khpb_r(0u, 0x3u, khpb_ra);
            khpb_r(1u, 0x04u, khpb_ra);
            khpb_ins(0u, 10u);                                                // add rd.xy, ra.xyxx, l(0.5, 0.5, 0, 0)
            khpb_r(0u, 0x3u, khpb_rd);
            khpb_r(1u, 0x04u, khpb_ra);
            khpb_p.push_back(2u | (khpb_kl << 12));
            khpb_p.push_back(0x3F000000u);
            khpb_p.push_back(0x3F000000u);
            khpb_p.push_back(0u);
            khpb_p.push_back(0u);
            khpb_ins(18u, 1u);                                                // else (s = 1)
            khpb_ins(54u, 5u);                                                //   mov rd.xy, v.xyxx
            khpb_r(0u, 0x3u, khpb_rd);
            khpb_p.push_back(kh_psb_op(khpb_ki, 1u, 0x04u, 1u));
            khpb_p.push_back(khpb_pr);
            khpb_ins(21u, 1u);                                                // endif
            if (((khpb_t[khpb_pos] >> 4) & 8u) != 0u) {
                khpb_ins(54u, 5u);                                            // mov rd.w, v.wwww
                khpb_r(0u, 0x8u, khpb_rd);
                khpb_p.push_back(kh_psb_op(khpb_ki, 1u, khpb_sw, 1u));
                khpb_p.push_back(khpb_pr);
            }
        }
    }
    khpb_p.insert(khpb_p.end(), khpb_x.begin(), khpb_x.end());
    khpb_p[1] = static_cast<uint32_t>(khpb_p.size());
    // The signatures: SV_Position's used mask + .xyz; the output signature SV_Target0 - 1 (fxc's own encoding of a
    // float4 and a float target: 68 bytes, the name at 56, o1's masks 1 / 0xE).
    std::vector<uint8_t> khpb_isg(khpb_b + khpb_io, khpb_b + khpb_io + khpb_in);
    khpb_isg[khpb_pe + 21] = static_cast<uint8_t>(khpb_isg[khpb_pe + 21] | 7u);   // .xyz read.
    std::vector<uint8_t> khpb_osg(68u, 0u);
    kh_psv_put32(khpb_osg.data(), 2u);
    kh_psv_put32(khpb_osg.data() + 4, 8u);
    for (uint32_t khpb_e = 0; khpb_e < 2u; ++khpb_e) {
        uint8_t* const khpb_q = khpb_osg.data() + 8 + 24 * khpb_e;
        kh_psv_put32(khpb_q, 56u);
        kh_psv_put32(khpb_q + 4, khpb_e);
        kh_psv_put32(khpb_q + 12, 3u);
        kh_psv_put32(khpb_q + 16, khpb_e);
        khpb_q[20] = khpb_e == 1u ? 0x01u : 0x0Fu;
        khpb_q[21] = khpb_e == 1u ? 0x0Eu : 0x00u;
    }
    memcpy(khpb_osg.data() + 56, "SV_Target\0\xAB\xAB", 12);
    // The container again: the chunks in their order, the program and the signatures replaced.
    size_t khpb_total = 32u + 4u * khpb_cn;
    for (uint32_t khpb_c = 0; khpb_c < khpb_cn; ++khpb_c) {
        const size_t khpb_co = kh_psv_get32(khpb_b + 32 + 4 * khpb_c);
        khpb_total += 8u + (khpb_c == khpb_xi ? 4u * khpb_p.size() : khpb_c == khpb_ii ? khpb_isg.size()
                            : khpb_c == khpb_oi ? khpb_osg.size() : kh_psv_get32(khpb_b + khpb_co + 4));
    }
    std::vector<uint8_t> khpb_nb(khpb_total, 0u);
    memcpy(khpb_nb.data(), khpb_b, 24);
    kh_psv_put32(khpb_nb.data() + 24, static_cast<uint32_t>(khpb_total));
    kh_psv_put32(khpb_nb.data() + 28, khpb_cn);
    size_t khpb_wp = 32u + 4u * khpb_cn;
    for (uint32_t khpb_c = 0; khpb_c < khpb_cn; ++khpb_c) {
        const size_t khpb_co = kh_psv_get32(khpb_b + 32 + 4 * khpb_c);
        kh_psv_put32(khpb_nb.data() + 32 + 4 * khpb_c, static_cast<uint32_t>(khpb_wp));
        memcpy(khpb_nb.data() + khpb_wp, khpb_b + khpb_co, 4);
        if (khpb_c == khpb_xi) {
            kh_psv_put32(khpb_nb.data() + khpb_wp + 4, static_cast<uint32_t>(4u * khpb_p.size()));
            for (size_t khpb_q = 0; khpb_q < khpb_p.size(); ++khpb_q) {
                kh_psv_put32(khpb_nb.data() + khpb_wp + 8 + 4 * khpb_q, khpb_p[khpb_q]);
            }
            khpb_wp += 8u + 4u * khpb_p.size();
        } else if (khpb_c == khpb_ii || khpb_c == khpb_oi) {
            const std::vector<uint8_t>& khpb_s = khpb_c == khpb_ii ? khpb_isg : khpb_osg;
            kh_psv_put32(khpb_nb.data() + khpb_wp + 4, static_cast<uint32_t>(khpb_s.size()));
            memcpy(khpb_nb.data() + khpb_wp + 8, khpb_s.data(), khpb_s.size());
            khpb_wp += 8u + khpb_s.size();
        } else {
            const size_t khpb_cs = kh_psv_get32(khpb_b + khpb_co + 4);
            memcpy(khpb_nb.data() + khpb_wp + 4, khpb_b + khpb_co + 4, 4u + khpb_cs);
            khpb_wp += 8u + khpb_cs;
        }
    }
    uint32_t khpb_sum[4];
    kh_psv_checksum(khpb_nb.data(), khpb_nb.size(), khpb_sum);
    for (int khpb_q = 0; khpb_q < 4; ++khpb_q) kh_psv_put32(khpb_nb.data() + 4 + 4 * khpb_q, khpb_sum[khpb_q]);
    khpb_out.swap(khpb_nb);
    return KH_PSV_OK;
}
// KH_PSV - the capture. kh_psv_hook_ensure detours ID3D11Device::CreatePixelShader (vtable slot 15; ID3D11Device1 - 5's
// entry too where it is other code): the original runs, then a successful creation's bytecode is copied into the
// store keyed by the returned object. Skipped: our own creations (kh_psv_ours; the tail's builtins register by
// kh_psv_own), class-linkage creations, a non-DXBC blob or one over KH_PSV_BLOB_MAX, a store at KH_PSV_STORE_MAX
// (reported once). Each kept blob's object carries the blob's serial as private data (KH_PSV_STAMP): a bound shader's
// serial names its bytes, so an object at a reused address that came another way never takes a dead one's blob. The
// store is process-scoped and the engine's blobs are never reset (its shaders outlive missions and device resets); a
// blob goes when its shader is destroyed (KH_PSV_TOMB) or a creation lands at its address (our builtin taken by the
// detour and then by kh_psv_own is that case: the overwrite releases the old sentinel); a builtin's also when its
// slot is made again and at Resources::release / the session's globals reset (kh_psv_device_lost). Any thread:
// g_psv_mx guards the store and no D3D call is made under it (SetPrivateData after). Installed at the earliest device
// reach: the extension's load (main.cpp's pre_start calls kh_psv_hook_ensure), else a mission edge
// (rendering_integration_reset), else the first Draw3D frame (flush_frame).
// A shader created before the install has no bytecode here: its draws keep today's passes.
static constexpr size_t KH_PSV_BLOB_MAX = static_cast<size_t>(1u) << 20;    // 1 MB (the engine's particle PS: < 64 KB).
static constexpr size_t KH_PSV_STORE_MAX = static_cast<size_t>(64u) << 20;  // 64 MB in all.
static const GUID KH_PSV_STAMP = { 0x6E8A0F3Bu, 0x5C2D, 0x4B1E, { 0x9A, 0x47, 0x3D, 0x5F, 0x1C, 0x2B, 0x7E, 0x90 } };
struct KhPsvBlob {
    uint64_t ser = 0;   // Its serial (the stamp its shader carries).
    std::vector<uint8_t> b;
    int own = -1;   // One of our builtin mesh shaders (kh_psv_own's slot, 0 - 5), else -1 (the engine's).
};
struct KhPsvBooks {   // Under g_psv_mx (the store's own books, kept since the hook went in).
    uint64_t bytes = 0;    // Bytes held (KH_PSV_STORE_MAX's test).
};
static std::mutex g_psv_mx;
static std::unordered_map<const void*, KhPsvBlob> g_psv_store;   // By the created ID3D11PixelShader (an identity).
static KhPsvBooks g_psv_books;
static uint64_t g_psv_ser = 0;   // The last serial given (under g_psv_mx).
static std::atomic<bool> g_psv_cap_on{ false };   // The detour copies (cleared at process detach: a pass-through).
static thread_local int g_psv_self = 0;   // Our own variants' creation on this thread: not captured.
// KH_PSV_TOMB - each held blob's shader carries a sentinel as private data (SetPrivateDataInterface); the runtime
// releases it when the shader is destroyed and its final Release drops the blob if the entry at that address still
// holds its serial. Any thread, possibly inside one of our hooks: g_psv_mx is a leaf lock (no other lock, D3D call or
// COM release under it by any holder), so the sentinel never waits on a lock its own thread holds. Gated on
// g_psv_cap_on (cleared at process detach, by g_psv_gate when this unit's statics end, and at a refused install): a
// sentinel released after that touches nothing, itself included. A refused allocation or attach leaves the blob held
// without one (dropped only by a creation at its address). HARD REQUIREMENT: this DLL is never unloaded at runtime -
// every held engine shader carries a vtable in this module and the store holds no reference to withdraw them by.
static const GUID KH_PSV_TOMB = { 0x1A5A5FCBu, 0x8F7B, 0x4E1C, { 0x9C, 0x0F, 0x56, 0x84, 0xFE, 0xF6, 0xF7, 0xC2 } };
static struct KhPsvGate {   // Declared after the store, so it ends before it: the sentinels' gate closes first.
    ~KhPsvGate() { g_psv_cap_on.store(false, std::memory_order_relaxed); }
} g_psv_gate;
inline void kh_psv_tomb_drop(const void* khtd_key, uint64_t khtd_ser) noexcept {
    try {
        std::lock_guard<std::mutex> khtd_g(g_psv_mx);
        const auto khtd_it = g_psv_store.find(khtd_key);
        if (khtd_it == g_psv_store.end() || khtd_it->second.ser != khtd_ser) return;
        g_psv_books.bytes -= khtd_it->second.b.size();
        g_psv_store.erase(khtd_it);
    } catch (...) {
        // The lock refused: the blob stays (dropped by a creation at its address).
    }
}
struct KhPsvTomb final : public IUnknown {
    KhPsvTomb(const void* khpt_k, uint64_t khpt_s) : key(khpt_k), ser(khpt_s) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID khpt_iid, void** khpt_out) override {
        if (!khpt_out) return E_POINTER;
        if (IsEqualIID(khpt_iid, __uuidof(IUnknown))) {
            *khpt_out = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        *khpt_out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return rc.fetch_add(1u, std::memory_order_relaxed) + 1u; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG khpt_n = rc.fetch_sub(1u, std::memory_order_acq_rel) - 1u;
        if (khpt_n == 0u && g_psv_cap_on.load(std::memory_order_relaxed)) {
            if (armed) kh_psv_tomb_drop(key, ser);
            delete this;
        }
        return khpt_n;
    }
    std::atomic<ULONG> rc{ 1u };   // The creator's reference (the capture releases it once attached).
    const void* key;               // The shader (an identity) and its blob's serial.
    uint64_t ser;
    bool armed = true;             // False: never attached (released at once, nothing dropped).
};
// khpc_own: kh_psv_own's slot (our builtin), else -1 (a creation the detour took for the engine's).
inline void kh_psv_capture(const void* khpc_bc, SIZE_T khpc_n, ID3D11ClassLinkage* khpc_cl,
                           ID3D11PixelShader* khpc_ps, int khpc_own = -1) {
    if (!khpc_ps || g_psv_self > 0 || !g_psv_cap_on.load(std::memory_order_relaxed)) return;
    try {
        const bool khpc_fit = !khpc_cl && khpc_bc && khpc_n >= 32u && khpc_n <= KH_PSV_BLOB_MAX &&
                              memcmp(khpc_bc, "DXBC", 4) == 0;
        std::vector<uint8_t> khpc_v;   // Copied before the lock.
        if (khpc_fit) {
            khpc_v.assign(static_cast<const uint8_t*>(khpc_bc), static_cast<const uint8_t*>(khpc_bc) + khpc_n);
        }
        uint64_t khpc_ser = 0;
        bool khpc_full = false;   // KH_PSV_TOMB: refused by the cap (reported once, after the lock).
        {
            std::lock_guard<std::mutex> khpc_g(g_psv_mx);
            const auto khpc_it = g_psv_store.find(khpc_ps);
            if (khpc_it != g_psv_store.end()) {   // A dead shader's blob at this address.
                g_psv_books.bytes -= khpc_it->second.b.size();
                g_psv_store.erase(khpc_it);
            }
            if (khpc_own >= 0) {   // The slot's previous builtin (made again: a sample-count change) goes.
                for (auto khpc_o = g_psv_store.begin(); khpc_o != g_psv_store.end();) {
                    if (khpc_o->second.own != khpc_own) {
                        ++khpc_o;
                        continue;
                    }
                    g_psv_books.bytes -= khpc_o->second.b.size();
                    khpc_o = g_psv_store.erase(khpc_o);
                }
            }
            if (khpc_cl) {
                // KH_PSV: class linkage - not held.
            } else if (!khpc_fit || g_psv_books.bytes + khpc_n > KH_PSV_STORE_MAX) {
                khpc_full = khpc_fit;
            } else {
                KhPsvBlob& khpc_e = g_psv_store[khpc_ps];
                khpc_ser = ++g_psv_ser;
                khpc_e.ser = khpc_ser;
                khpc_e.own = khpc_own;
                khpc_e.b.swap(khpc_v);
                g_psv_books.bytes += khpc_n;
            }
        }
        if (khpc_ser != 0) {
            khpc_ps->SetPrivateData(KH_PSV_STAMP, sizeof(khpc_ser), &khpc_ser);
            // KH_PSV_TOMB: the sentinel (the runtime AddRefs it; ours is released here). The shader cannot die before
            // this: its creator holds it until the capture returns.
            KhPsvTomb* const khpc_t = new (std::nothrow) KhPsvTomb(khpc_ps, khpc_ser);
            if (khpc_t) {
                if (FAILED(khpc_ps->SetPrivateDataInterface(KH_PSV_TOMB, khpc_t))) khpc_t->armed = false;
                khpc_t->Release();
            }
        }
        if (khpc_full) {   // KH_PSV_TOMB: the creating thread, outside g_psv_mx (any thread; deduped).
            // KH_EA_ERR_CLASS: a cap by design, not a fault - a warning (logged, never thrown).
            kh_log_once_safe("KH RenderIntegration: world see-through: the particle shader store is full (64 MB of "
                             "live pixel shaders); particle shaders created while it stays full keep the earlier "
                             "see-through passes");
        }
    } catch (...) {
        // An allocation refused: the shader is not held (its draws keep today's passes).
    }
}
// The stamp of a bound shader (0: none - created before the hook, or not held).
inline uint64_t kh_psv_stamp(ID3D11PixelShader* khps_ps) {
    uint64_t khps_v = 0;
    UINT khps_n = sizeof(khps_v);
    if (FAILED(khps_ps->GetPrivateData(KH_PSV_STAMP, &khps_n, &khps_v)) || khps_n != sizeof(khps_v)) return 0;
    return khps_v;
}
using KhPsvCreate = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*,
                                                 ID3D11PixelShader**);
static constexpr int KH_PSV_HOOK_N = 6;   // ID3D11Device - ID3D11Device5: at most six distinct entries.
static KhPsvCreate g_psv_orig[KH_PSV_HOOK_N] = {};
// Our own creations are not captured (this renderer remakes its shaders every mission; their dead blobs would fill
// the store): a creation whose return address lies in this module's image (g_psv_base, set at the install) is ours.
// The tail's builtins register at their creation instead (kh_psv_own: six slots, each holding its newest blob; all
// dropped at Resources::release, where they die - kh_psv_device_lost). Only the outermost of nested hooked entries
// decides (g_psv_in: an entry whose original reaches another hooked one). A tool that hooks this entry after us
// makes our creations look like the engine's: those blobs are held and go at their shader's release (KH_PSV_TOMB).
static const void* g_psv_base = nullptr;
static thread_local int g_psv_in = 0;
inline bool kh_psv_ours(const void* khpo_ret) {
    MEMORY_BASIC_INFORMATION khpo_m = {};
    return g_psv_base && VirtualQuery(khpo_ret, &khpo_m, sizeof(khpo_m)) == sizeof(khpo_m) &&
           khpo_m.AllocationBase == g_psv_base;
}
template <int KhPsvI>
static HRESULT STDMETHODCALLTYPE kh_psv_create(ID3D11Device* khpd_self, const void* khpd_bc, SIZE_T khpd_n,
                                               ID3D11ClassLinkage* khpd_cl, ID3D11PixelShader** khpd_ps) {
    const void* const khpd_ret = _ReturnAddress();
    const bool khpd_outer = g_psv_in == 0;
    ++g_psv_in;
    const HRESULT khpd_hr = g_psv_orig[KhPsvI](khpd_self, khpd_bc, khpd_n, khpd_cl, khpd_ps);
    --g_psv_in;
    if (khpd_outer && SUCCEEDED(khpd_hr) && khpd_ps && !kh_psv_ours(khpd_ret)) {
        kh_psv_capture(khpd_bc, khpd_n, khpd_cl, *khpd_ps);
    }
    return khpd_hr;
}
// A builtin mesh pixel shader the tail binds (ensure_resources / ensure_composite_shader: 0 PSMain, 1 its textured
// twin, 2 PSComposite, 3 its arb twin, 4 / 5 their textured twins), just made from khpo_bc: held as the engine's are.
// None with the system built out (KH_WSEE_ON false: no store, no hook).
inline void kh_psv_own(int khpo_slot, const void* khpo_bc, SIZE_T khpo_n, ID3D11PixelShader* khpo_ps) {
    if constexpr (KH_WSEE_ON) kh_psv_capture(khpo_bc, khpo_n, nullptr, khpo_ps, khpo_slot);
}
// The hook's state: 0 not yet / 1 installed / -1 refused (MinHook; reported once) - kh_psv_pick reads it.
static std::atomic<int> g_psv_hook_state{ 0 };
// Game thread. Once a process (process-scoped, as the context hooks: no session reset, and never unhooked - process
// detach leaves the detour a pass-through into this module, g_psv_cap_on false, the context hooks' contract); a
// device not reachable yet leaves it for the next point. Never installed with the system built out (KH_WSEE_ON
// false: no detour, no blob store; g_psv_hook_state stays 0).
inline void kh_psv_hook_ensure() {
    if constexpr (KH_WSEE_ON) {   // The body whole (no code left unreachable with it out: C4702).
        if (g_psv_hook_state.load(std::memory_order_relaxed) != 0 || !kh_render_on()) return;
        if (!RVExtBridge::is_initialized()) return;   // flush_frame's rule before the device is asked for.
        ID3D11Device* const khph_dev = RVExtBridge::get_d3d_device();
        if (!khph_dev || !ensure_minhook()) return;
        // ID3D11Device1 - 5 (d3d11_1.h - d3d11_4.h's IIDs): one implementation normally serves them all with one table,
        // so their slot 15 is ID3D11Device's - an interface whose entry is other code is hooked too.
        static const GUID khph_iid[5] = {
            { 0xA04BFB29u, 0x08EF, 0x43D6, { 0xA4, 0x9C, 0xA9, 0xBD, 0xBD, 0xCB, 0xE6, 0x86 } },
            { 0x9D06DFFAu, 0xD1E5, 0x4D07, { 0x83, 0xA8, 0x1B, 0xB1, 0x23, 0xF2, 0xF8, 0x41 } },
            { 0xA05C8C37u, 0xD2C6, 0x4732, { 0xB3, 0xA0, 0x9C, 0xE0, 0xB0, 0xDC, 0x9A, 0xE6 } },
            { 0x8992AB71u, 0x02E6, 0x4B8D, { 0xBA, 0x48, 0xB0, 0x56, 0xDC, 0xDA, 0x42, 0xC4 } },
            { 0x8FFDE202u, 0xA0E7, 0x45DF, { 0x9E, 0x01, 0xE8, 0x37, 0x80, 0x1B, 0x5E, 0xA0 } } };
        void* khph_at[KH_PSV_HOOK_N] = {};
        int khph_n = 0;
        auto khph_add = [&](void* khph_obj) {
            void* const khph_fn = (*reinterpret_cast<void***>(khph_obj))[15];   // CreatePixelShader.
            for (int khph_i = 0; khph_i < khph_n; ++khph_i) {
                if (khph_at[khph_i] == khph_fn) return;
            }
            if (khph_n < KH_PSV_HOOK_N) khph_at[khph_n++] = khph_fn;
        };
        khph_add(khph_dev);
        for (const GUID& khph_g : khph_iid) {
            void* khph_q = nullptr;
            if (SUCCEEDED(khph_dev->QueryInterface(khph_g, &khph_q)) && khph_q) {
                khph_add(khph_q);
                static_cast<IUnknown*>(khph_q)->Release();
            }
        }
        void* const khph_det[KH_PSV_HOOK_N] = {
            reinterpret_cast<void*>(&kh_psv_create<0>), reinterpret_cast<void*>(&kh_psv_create<1>),
            reinterpret_cast<void*>(&kh_psv_create<2>), reinterpret_cast<void*>(&kh_psv_create<3>),
            reinterpret_cast<void*>(&kh_psv_create<4>), reinterpret_cast<void*>(&kh_psv_create<5>) };
        {
            MEMORY_BASIC_INFORMATION khph_m = {};
            if (VirtualQuery(reinterpret_cast<const void*>(&kh_psv_ours), &khph_m, sizeof(khph_m)) == sizeof(khph_m)) {
                g_psv_base = khph_m.AllocationBase;   // This module's image (kh_psv_ours).
            }
        }
        g_psv_cap_on.store(true, std::memory_order_relaxed);
        int khph_ok = 0;
        for (; khph_ok < khph_n; ++khph_ok) {
            if (MH_CreateHook(khph_at[khph_ok], khph_det[khph_ok],
                              reinterpret_cast<void**>(&g_psv_orig[khph_ok])) != MH_OK) break;
            if (MH_EnableHook(khph_at[khph_ok]) != MH_OK) {
                MH_RemoveHook(khph_at[khph_ok]);
                break;
            }
        }
        if (khph_ok != khph_n) {
            // KH_PSV: the entries already enabled are disabled, never removed - a thread already inside
            // kh_psv_create may still call its trampoline (g_psv_orig), which a removal would free; they stay created
            // for the process, g_psv_cap_on false (the one created but not enabled above is removed: no thread can
            // have entered it).
            for (int khph_i = 0; khph_i < khph_ok; ++khph_i) MH_DisableHook(khph_at[khph_i]);
            g_psv_cap_on.store(false, std::memory_order_relaxed);
            g_psv_hook_state.store(-1, std::memory_order_relaxed);
            report_error_once_safe("KH RenderIntegration: world see-through: the pixel-shader creation hook could "
                                   "not be installed (particles keep the engine's own shaders in the see-through "
                                   "record)");
            return;
        }
        g_psv_hook_state.store(1, std::memory_order_relaxed);
    }
}

// KH_PSV - the variants by shader (render thread; the flush's tail may be the game thread's under a park, which
// stalls it). kh_wsee_post and kh_wsee_tail find or admit the bound shader (kh_psv_pick); the main depth's clear makes
// what is queued (kh_psv_cycle_head: device Create* alone, outside every draw, at most KH_PSV_MAKE_N entries a clear,
// each up to three CreatePixelShader calls); Resources::release (a device reset, and every session's teardown) and the
// session's globals reset drop them all (kh_psv_device_lost). key: the shader object - an identity, never dereferenced;
// its stamp, read from the bound object, names it. Past KH_PSV_ENT_N shaders the entry least recently picked goes
// (kh_psv_admit).
static constexpr uint8_t KH_PSV_ST_NONE = 0u, KH_PSV_ST_QUEUED = 1u, KH_PSV_ST_MADE = 2u, KH_PSV_ST_REFUSED = 3u,
                         KH_PSV_ST_NOBLOB = 4u;
static constexpr uint32_t KH_PSV_R_DEVICE = 11u;   // The device refused a variant (CreatePixelShader failed).
struct KhPsvEnt {
    const void* key = nullptr;
    uint64_t ser = 0;
    uint8_t st = KH_PSV_ST_NONE;    // alpha + nw.
    bool own = false;               // Our own (the tail's): never a one pass.
    bool nw_none = false;           // NW binds no pixel shader.
    ID3D11PixelShader* alpha = nullptr;
    ID3D11PixelShader* nw = nullptr;
    ID3D11PixelShader* one = nullptr;   // KH_PSB: the one pass's variant (null: refused - today's passes).
    uint32_t one_cb = 0, one_t = 0;     // Its constant-buffer and depth-view slots.
    uint32_t one_tz = 128u;             // KH_PSB_ZB: and Zb's slot.
    uint64_t pick = 0;                  // KH_PSV: its last pick's serial (g_psv_pick_n; the eviction's order).
};
static constexpr int KH_PSV_ENT_N = 64;
// Entries made a clear: up to three creates each (alpha, nw, KH_PSB's one) - the one variant is a full driver compile
// of the whole engine shader on the render thread, a hitch per shader at its first bind in each session and after
// each device reset (kh_psv_device_lost drops the variants).
static constexpr int KH_PSV_MAKE_N = 2;
static KhPsvEnt g_psv_ent[KH_PSV_ENT_N];
// KH_PSV: the picks' serial (render thread; 0 again with every entry - kh_psv_device_lost). Per pick, not per cycle:
// a cycle stamp would tie a hot entry not yet picked this cycle with the last cycle's dead ones.
static uint64_t g_psv_pick_n = 0;
static bool g_psb_have = false;                  // KH_PSB: some entry holds one (the capture makes the record).
// g_psb_have (render thread): whether any engine entry holds a one-pass variant.
inline void kh_psv_publish() {
    bool khpb_o = false;
    for (const KhPsvEnt& khpb_e : g_psv_ent) {
        if (!khpb_e.key || khpb_e.own) continue;   // KH_PSV: the engine's alone (our tail's never hold a one pass).
        if (khpb_e.st == KH_PSV_ST_MADE && khpb_e.one) khpb_o = true;   // KH_PSB.
    }
    g_psb_have = khpb_o;
}
inline void kh_psv_ent_drop(KhPsvEnt& khed_e) {
    KH_SAFE_RELEASE(khed_e.alpha);
    KH_SAFE_RELEASE(khed_e.nw);
    KH_SAFE_RELEASE(khed_e.one);   // KH_PSB.
    khed_e = KhPsvEnt();
}
// A shader first bound (render thread): queued where the store holds its bytes (one lock a new shader), else
// no-blob. A slot that held the same address (a dead shader) is reused first, then a free one, then the one least
// recently picked (KH_PSV: a dead or cold entry before a hot one; the scan runs only at a new shader's admission).
// Past KH_PSV_ENT_N distinct shaders in one frame the entries still cycle (the cache's size).
inline KhPsvEnt& kh_psv_admit(const void* khpa_key, uint64_t khpa_ser, bool khpa_own) {
    int khpa_k = -1;
    for (int khpa_i = 0; khpa_i < KH_PSV_ENT_N && khpa_k < 0; ++khpa_i) {
        if (g_psv_ent[khpa_i].key == khpa_key) khpa_k = khpa_i;
    }
    for (int khpa_i = 0; khpa_i < KH_PSV_ENT_N && khpa_k < 0; ++khpa_i) {
        if (!g_psv_ent[khpa_i].key) khpa_k = khpa_i;
    }
    if (khpa_k < 0) {
        khpa_k = 0;
        for (int khpa_i = 1; khpa_i < KH_PSV_ENT_N; ++khpa_i) {
            if (g_psv_ent[khpa_i].pick < g_psv_ent[khpa_k].pick) khpa_k = khpa_i;
        }
    }
    KhPsvEnt& khpa_e = g_psv_ent[khpa_k];
    kh_psv_ent_drop(khpa_e);   // No variant is bound here: every pass's shader is set back before a post ends.
    khpa_e.key = khpa_key;
    khpa_e.ser = khpa_ser;
    khpa_e.own = khpa_own;
    khpa_e.st = KH_PSV_ST_NOBLOB;
    if (khpa_ser != 0u) {
        std::lock_guard<std::mutex> khpa_g(g_psv_mx);
        const auto khpa_it = g_psv_store.find(khpa_key);
        if (khpa_it != g_psv_store.end() && khpa_it->second.ser == khpa_ser) {
            khpa_e.st = KH_PSV_ST_QUEUED;
        }
    }
    kh_psv_publish();
    return khpa_e;
}
// One re-issue's choice: on - the passes bind alpha (T, Tp, Tp2, Tf) and nw (NW; null: no pixel shader), and orig
// holds the bound shader (PSGetShader's reference) for the set-back. Off - nothing taken, nothing changes.
struct KhPsvPick {
    bool on = false;
    ID3D11PixelShader* alpha = nullptr;
    ID3D11PixelShader* nw = nullptr;
    ID3D11PixelShader* orig = nullptr;
    ID3D11PixelShader* one = nullptr;   // KH_PSB: the one pass's variant where made (with alpha / nw's), and its
    uint32_t one_cb = 0, one_t = 0;     // slots.
    uint32_t one_tz = 128u;             // KH_PSB_ZB: Zb's slot (128: none).
};
// khpk_tail: our own tail's part (kh_wsee_tail). No variant (hook not in, no shader, class instances bound, no stamp,
// not made yet, refused): off.
inline void kh_psv_pick(ID3D11DeviceContext* ctx, KhPsvPick& khpk, bool khpk_tail) {
    if (g_psv_hook_state.load(std::memory_order_relaxed) != 1) return;   // No bytecode reaches us: not even a read.
    ID3D11PixelShader* khpk_ps = nullptr;
    ID3D11ClassInstance* khpk_ci[8] = {};
    UINT khpk_cn = 8;
    ctx->PSGetShader(&khpk_ps, khpk_ci, &khpk_cn);
    bool khpk_cls = khpk_cn > 0;
    for (ID3D11ClassInstance*& khpk_c : khpk_ci) {
        if (khpk_c) khpk_cls = true;
        KH_SAFE_RELEASE(khpk_c);
    }
    if (khpk_ps && !khpk_cls) {
        const uint64_t khpk_ser = kh_psv_stamp(khpk_ps);
        KhPsvEnt* khpk_e = nullptr;
        for (KhPsvEnt& khpk_x : g_psv_ent) {
            if (khpk_x.key == khpk_ps && khpk_x.ser == khpk_ser) {
                khpk_e = &khpk_x;
                break;
            }
        }
        if (!khpk_e) khpk_e = &kh_psv_admit(khpk_ps, khpk_ser, khpk_tail);
        khpk_e->pick = ++g_psv_pick_n;   // KH_PSV: kh_psv_admit's eviction order.
        if (khpk_e->st == KH_PSV_ST_MADE) {
            khpk.on = true;
            khpk.alpha = khpk_e->alpha;
            khpk.nw = khpk_e->nw_none ? nullptr : khpk_e->nw;
            khpk.one = khpk_e->one;   // KH_PSB.
            khpk.one_cb = khpk_e->one_cb;
            khpk.one_t = khpk_e->one_t;
            khpk.one_tz = khpk_e->one_tz;   // KH_PSB_ZB.
        }
    }
    if (khpk.on) {
        khpk.orig = khpk_ps;
        return;
    }
    KH_SAFE_RELEASE(khpk_ps);
}
// One variant from its patched bytes (kh_psv_make's alpha / nw, kh_psb_make_one's one; the clear, device Create*
// alone). g_psv_self keeps it out of the store.
inline uint32_t kh_psv_create_one(ID3D11Device* dev, const std::vector<uint8_t>& khpo_v, ID3D11PixelShader** khpo_out) {
    struct KhPsvSelfScope {   // Held over the create alone, whatever leaves it.
        KhPsvSelfScope() { ++g_psv_self; }
        ~KhPsvSelfScope() { --g_psv_self; }
    } khpo_scope;
    const HRESULT khpo_hr = dev->CreatePixelShader(khpo_v.data(), khpo_v.size(), nullptr, khpo_out);
    if (SUCCEEDED(khpo_hr) && *khpo_out) return KH_PSV_OK;
    KH_SAFE_RELEASE(*khpo_out);
    return KH_PSV_R_DEVICE;
}
// KH_PSB: the one pass's variant of an entry whose alpha / nw variants stand (kh_psv_make). Refused or not created:
// none - the shader's re-issues keep the two variants' passes; the code is named once in the RPT
// (KH_PSV_REFUSE_REPORT).
inline void kh_psb_make_one(ID3D11Device* dev, KhPsvEnt& khmo_e, const std::vector<uint8_t>& khmo_b) {
    std::vector<uint8_t> khmo_v;
    uint32_t khmo_cb = 0u, khmo_tt = 0u, khmo_tz = 128u;
    uint32_t khmo_ro = kh_psb_patch(khmo_b.data(), khmo_b.size(), khmo_v, khmo_cb, khmo_tt, khmo_tz);
    if (khmo_ro == KH_PSV_OK) khmo_ro = kh_psv_create_one(dev, khmo_v, &khmo_e.one);
    if (khmo_ro == KH_PSV_OK) {
        khmo_e.one_cb = khmo_cb;
        khmo_e.one_t = khmo_tt;
        khmo_e.one_tz = khmo_tz;   // KH_PSB_ZB.
        return;
    }
    // KH_PSV_REFUSE_REPORT: the refusal is named once per code (the code is KH_PSV_R_* / KH_PSB_R_*'s value, above);
    // the string is built at the make alone, never per frame. KH_EA_ERR_CLASS: the patcher's refusal of an engine
    // shader is expected (a warning, logged); the device's (KH_PSV_R_DEVICE) is an error.
    const std::string khmo_msg = "KH RenderIntegration: world see-through: an engine pixel shader has no one-pass "
                                 "variant (refusal " + std::to_string(khmo_ro) +
                                 "); its re-draws take the alpha and coverage passes";
    if (khmo_ro == KH_PSV_R_DEVICE) report_error_once_safe(khmo_msg);
    else kh_log_once_safe(khmo_msg);
}
inline void kh_psv_make(ID3D11Device* dev, KhPsvEnt& khpm_e, const std::vector<uint8_t>& khpm_b) {
    std::vector<uint8_t> khpm_v;
    uint32_t khpm_r = kh_psv_patch(khpm_b.data(), khpm_b.size(), KH_PSV_MODE_ALPHA, khpm_v);
    if (khpm_r == KH_PSV_OK) khpm_r = kh_psv_create_one(dev, khpm_v, &khpm_e.alpha);
    if (khpm_r == KH_PSV_OK) {
        khpm_r = kh_psv_patch(khpm_b.data(), khpm_b.size(), KH_PSV_MODE_NW, khpm_v);
        if (khpm_r == KH_PSV_NOPS) {
            khpm_e.nw_none = true;
            khpm_r = KH_PSV_OK;
        } else if (khpm_r == KH_PSV_OK) {
            khpm_r = kh_psv_create_one(dev, khpm_v, &khpm_e.nw);
        }
    }
    if (khpm_r == KH_PSV_OK) {
        // KH_PSB: the one pass's variant beside them (not our tail's builtins, which write a second target and a
        // depth or coverage: refused by the patcher anyway).
        if (!khpm_e.own) kh_psb_make_one(dev, khpm_e, khpm_b);
        khpm_e.st = KH_PSV_ST_MADE;
        return;
    }
    KH_SAFE_RELEASE(khpm_e.alpha);
    KH_SAFE_RELEASE(khpm_e.nw);
    khpm_e.nw_none = false;
    khpm_e.st = KH_PSV_ST_REFUSED;
    // KH_PSV_REFUSE_REPORT: likewise, once per distinct message (an engine shader's: the code; ours: the bytes and
    // the code). KH_EA_ERR_CLASS: an engine shader the patcher refuses is a warning (logged); a device refusal
    // (KH_PSV_R_DEVICE) or a refused shader of ours (our builtins must reduce) is an error.
    const std::string khpm_msg = std::string("KH RenderIntegration: world see-through: ") +
                                 (khpm_e.own ? "a tail pixel shader of ours, " + std::to_string(khpm_b.size()) +
                                                   " bytes, is not reduced"
                                             : std::string("an engine pixel shader is not reduced")) +
                                 " (refusal " + std::to_string(khpm_r) + "); its re-issues keep the whole shader";
    if (khpm_e.own || khpm_r == KH_PSV_R_DEVICE) report_error_once_safe(khpm_msg);
    else kh_log_once_safe(khpm_msg);
}

// The main depth's clear (render thread, after kh_wsee_cycle_note): the queued variants (KH_PSV_TOMB: no device
// prune - a dead device's shaders die first and their blobs go at their release).
inline void kh_psv_cycle_head(ID3D11DeviceContext* ctx) {
    ID3D11Device* khch_dev = nullptr;
    ctx->GetDevice(&khch_dev);
    if (!khch_dev) return;
    int khch_made = 0;
    for (KhPsvEnt& khch_e : g_psv_ent) {
        if (khch_made >= KH_PSV_MAKE_N) break;
        if (khch_e.st != KH_PSV_ST_QUEUED) continue;
        std::vector<uint8_t> khch_b;
        try {
            std::lock_guard<std::mutex> khch_g(g_psv_mx);
            const auto khch_it = g_psv_store.find(khch_e.key);
            if (khch_it != g_psv_store.end() && khch_it->second.ser == khch_e.ser) khch_b = khch_it->second.b;
        } catch (...) {
            khch_b.clear();
        }
        ++khch_made;
        if (khch_b.empty()) {   // Gone from the store since (a dead shader's address reused).
            kh_psv_ent_drop(khch_e);
            continue;
        }
        try {   // An allocation refused leaves the entry refused, nothing half-made, the clear's work intact.
            kh_psv_make(khch_dev, khch_e, khch_b);
        } catch (...) {
            KH_SAFE_RELEASE(khch_e.alpha);
            KH_SAFE_RELEASE(khch_e.nw);
            KH_SAFE_RELEASE(khch_e.one);   // KH_PSB.
            khch_e.nw_none = false;
            khch_e.st = KH_PSV_ST_REFUSED;
        }
    }
    khch_dev->Release();
    if (khch_made > 0) kh_psv_publish();
}
// Resources::release (a device reset, and every session's teardown - kh_session_teardown's g_res.release()) and the
// session's globals reset (kh_rp_globals_reset; idempotent): every variant, our builtins' blobs (the engine's stay:
// the shaders they describe are not ours to forget).
inline void kh_psv_device_lost() {
    for (KhPsvEnt& khdl_e : g_psv_ent) kh_psv_ent_drop(khdl_e);
    {   // Our builtins' blobs: their shaders die in the same Resources::release (made again with the next ones).
        std::lock_guard<std::mutex> khdl_g(g_psv_mx);
        for (auto khdl_it = g_psv_store.begin(); khdl_it != g_psv_store.end();) {
            if (khdl_it->second.own < 0) {
                ++khdl_it;
                continue;
            }
            g_psv_books.bytes -= khdl_it->second.b.size();
            khdl_it = g_psv_store.erase(khdl_it);
        }
    }
    g_psv_pick_n = 0;   // KH_PSV: every entry dropped above.
    kh_psv_publish();
}
}