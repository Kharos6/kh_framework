#pragma once

// The injection half (rendering_integration_injection.hpp): WSEE's particle-shader variants (PSV / PSB): their
// state, the DXBC patching, capture and creation. Never include it on its own.
namespace RenderIntegration {

// WSEE's re-issue saves: the engine's output-merger, pixel-shader and t0 state saved around a re-issued draw (null
// outside one wsee_re_save / _restore pair).
// Owner: the render thread.
struct WseeSaveState {
    // WSEE - the see-through record: the effect chain and our sorted translucents see through the world's blended
    // draws (fences, foliage, glass, particles) to the surface behind. Built in (WSEE_ON;
    // rendering/see_through_state.hpp's KH_WSEE_BUILD 0 builds it out: no capture, no hook work beyond early-out
    // tests); no script switch, every part of it always on. Threads: the pre-draw, the re-issues, the cycle's verdict,
    // the back-off and the releases are the render thread's; our tail is the flush's (render thread, or a park that
    // stalls it); the chain may be the game thread's late one; the want / landed / tainted / used / colour stamps are
    // atomics. WHY. A blended draw (DestBlend INV_SRC_ALPHA) leaves the scene's colour behind it in RT0 and either its
    // own depth (fence-type: it writes depth) or none (particle-type). Every depth reader of the chain then takes a
    // faint wire for a wall, and the distance fog paints the sky's fog over a particle. WANTED CYCLE (g_wsee.on,
    // wsee_cycle_note at the main depth's clear): WSEE_ON, a want stamped by a chain run that would arm a pass
    // (fx_chain_run, wsee_arm) at most WSEE_WANT_SPAN (2) cycles ago, no back-off, no WSEE_FLUSHW_PRED
    // stand-down. WSEE_WASTE_RUN (8) tainted or refused attempts in a row stand the capture down WSEE_BACKOFF
    // (120) cycles (the WSEE_FLUSHW_PRED hold where our flush's depth write ended the run). WSEE_IDLE_FREE (600)
    // cycles without a want release every target (VMSEE_LAZY: the next capture remakes them). A refused target make
    // is latched at its key and asked again WSEE_HOLD_FAIL_RETRY cycles later (WSEE_FAIL_RETRY). QUALIFYING DRAW
    // (wsee_pre_draw, reorder_pre_draw's call): an engine draw on the tracked context whose depth state writes or
    // tests without writing (the DSS hook's dss_write / dss_nowrite), RT0 = the scene texture, blend ADD with DestBlend
    // INV_SRC_ALPHA, SrcBlend ONE / SRC_ALPHA / ZERO, colour written, sample mask != 0, no alpha-to-coverage (an A2C
    // draw writes depth at its covered samples alone). Light-adding (DestBlend ONE, a source factor reading no dest)
    // and test-off over-blends count separately (below). Any other draw that really writes depth after the capture
    // (unblended, or blended but not qualifying, alpha-to-coverage included) is a late opaque (RE_OPAQUE). Draws below
    // the world's range (the view-model slice, our in-front meshes) are never tracked. A tainted cycle reads no further
    // draw's state (WSEE_TAINT_EARLY). CAPTURE (wsee_snapshot; once a cycle) at the cycle's first qualifying draw
    // once the world pass has begun (reorder_world_vp, reorder_min_opaques) and no injection can still come
    // (attempts spent, no eligible mesh, anomalous cycle, composite path dead: wsee_pre_draw's gate); else at the
    // injection's translucent tail (wsee_inj_capture), S and B then holding our solids. Targets (the main depth's
    // size unless said; made at the first capture that needs them, remade on a size / sample-count / quality / format
    // change; Resources' notes name each): S R32F 1x: sample 0 of the main depth, raw (PSDepthS0). B scene RT0 resolved
    // (its view format), only with a colour want (WSEE_BWANT: g_wsee.col_want_cycle within WANT_SPAN; the distance
    // fog, a user effect that names the record, the sort's modes 2 / 3); else left stale and not bound. T RGBA8 at the
    // depth's sample count, cleared (1, 0, 1, 1): R Tw (fence-type transmittance), G late-opaque flag (0 / 1), B Tpre,
    // A Tpost (particle-type, before / after B2). NW / NW2 R32 typeless, D32 DSV + R32F SRV, 1x, cleared 1: the nearest
    // particle-type depth, pre / post. Tp RGBA8 MS, cleared 1: particle-type again under a 25 % near push (R pre, G
    // post) and a 100 % push (B / A). Tf R8G8 1x over its own D32 cleared to D's raw (D = far / 4, wsee_far_raw):
    // fragments beyond D, pre / post. D3 / D3m D3 R32F 1x (SRV, UAV u3, RTV): a late opaque's own sample-0 depth; D3m
    // R32F MS: the post replays' z. P0 / P1 RGBA8, N / N2 R32F, at ceil(size / 2) (PSB_LS = 1): the one pass's
    // record, pre / post pair. Zb R32G32F at P's size (PSPsbZb right after S): the block's largest S and which block
    // pixel holds it. E R32_UINT frame size + a D24S8 stencil: the chain's class word (drawn once a run,
    // wsee_agree_draw). C Texture2DMS view of the scene the chain captures from (its sample colours). Q B's size /
    // format: the scene resolved before the flush's first compositing part. hold VB the deferred parts' geometry. B2
    // MOMENT (wsee_b2_take; once a cycle, at the first fence-type draw after the capture, before it): S2 = sample 0
    // raw then (R32F 1x; drawn where B2 is takeable or a pre late opaque was marked), B2 = the scene resolved then
    // (only in a cycle whose capture resolved B). Particle-type layers before this moment are "pre", after it "post".
    // RE-ISSUES (the draw hooks' post, wsee_post: the engine's draw again right after it, under its own predicate on
    // purpose - PRED_ASIDE; one wsee_re_save / wsee_re_restore pair a post). Kind by the pre-draw's mark (each
    // pass's targets, states and blends: the PASS_* table, "The re-issues' passes", TWIN of wsee_re_set's binds):
    // RE_BLEND (fence-type) PASS_T_EQ: T.R *= 1 - a under EQUAL (no write), the engine's sample mask: exactly the
    // samples it wrote, stencil or discard included. RE_NOWRITE (particle) the one pass (PSB) where its variant
    // exists, else PASS_T (T.B or .A), PASS_NW (NW / NW2), PASS_TP / TP2 (Tp R|G / B|A), PASS_TF (Tf beyond D). T, NW,
    // Tp, Tp2 take WseeSurfVp's MinDepth push (TOL 0.02 / 0.2 / 0.5 of the range): a fragment within 2 % of the
    // surface behind it is the surface's, within 25 % / 100 % in T but not Tp / Tp2. RE_NOWRITE_OFF the same with
    // dss_off on T / Tp / Tp2 (the engine drew it with the depth test off). RE_NWONLY (light-adding) PASS_NW alone: its
    // light is in S beyond T B, its depth is a front, T unchanged. RE_OPAQUE, post only PASS_ONE under EQUAL: T = (1,
    // 1, 1, 1), Tp = 1 (PSB_TPA's rule), D3m = its own sample-0 z (WSEE_PXZ; else the store path into D3 through
    // u3, D3m written -1 - wsee_d3m_path). A pre late opaque (marked before the B2 attempt) is NOT replayed
    // (WSEE_NOPRE): the shaders derive it (sample 0 is pre where S2 != S in the world's range; lane bits 8 / 9).
    // Past WSEE_OPAQUE_CAP (4096) replays the cycle is tainted. Particle-type shaders (PSV, PSB): the variants
    // "alpha", "nw" and "one", patched from the engine's PS bytecode copied at CreatePixelShader (a process-wide
    // MinHook detour; PSV_TOMB: this DLL must never unload), made at the clear (psv_cycle_head, <= PSV_MAKE_N
    // a clear: a one-time driver-compile hitch per shader per session and after each device reset). Refused or absent:
    // the engine's own shader draws today's passes (PSV_REFUSE_REPORT). THE ONE PASS (PSB, psb_post): one
    // draw, single sample, no depth view, into its phase's pair (P0 / N or P1 / N2) at 1 / 2 of the frame. The
    // variant's prologue makes the five passes' verdicts against the largest sample 0 of the block's 2 x 2 pixels
    // (bounded by Zb until psb_zb_trip), discards a fragment that writes nothing, and writes o0 = (aT, aP1, aP2,
    // aF), o1.x = z_1 under ZERO / INV_SRC_COLOR (N: MIN). Not taken (today's passes): psb_post's refusals, and our
    // own tail. OUR TAIL (wsee_tail): a normal-blend part that tests without writing (the injection's, or the
    // flush's) goes into T and NW (and Tp / Tp2 / Tf) as RE_NOWRITE's passes do, with PSMain / PSComposite's PSV
    // tail variants. SORT (wsee_sort_mode, WseeSortScope, cb.hlsl KhWseeMeshCut / KhWseeMeshOut): the injection
    // defers its translucent parts of a landed, untainted cycle to the flush's tail (g_wsee.defer_objs,
    // WSEE_DEFER_FLAG; a deformed part's vertex runs copied into the hold VB, a static part's entry naming the
    // mesh's own buffer - WSEE_OWN_STATIC; a lost or tainted deferral backs off WSEE_DEFER_BACKOFF cycles -
    // WSEE_DEFER_TAINT), where they draw far to near ahead of the flush's own meshes, with no hardware depth test:
    // mode 1 = the shader's occlusion test against the record, plain blend; 2 / 3 = that and the layer-aware
    // dual-source composite (3: a non-float target, inputs held to [0, 1]); 0 = the hardware test (the capture not
    // owed, a missing view, or a partitioned cycle: the engine's latched pair's near differs from the flush's by >
    // 2e-3, KH_WSEE_MS_REL). THE CHAIN (fx_chain_run; cb.hlsl KhWseeRule / KhWseeMix): a run whose cycle's capture
    // is valid (wsee_valid: landed, untainted, this depth) binds the record and arms each pass that uses it
    // (fx_tex[1].z = 1, wsee_mode_of; not an unmasked Outline nor a user effect reading the depth alone); without a
    // valid capture nothing is bound (WSEE_BIND_ARMED). Per pixel: front = min(r, NWpre, NWpost), behind = G ? D3 :
    // S (S2 for a derived pre); covered where front >= depthParams.z and behind > front; the pass blends what it
    // derives from depth by the layers' transmittance, front to back by layer group (fence-type at r, pre / post at
    // their NW, the near-band and far parts at the band's middle / at D). Per sample where the samples disagree (E: up
    // to four classes a pixel; a sited pass is drawn twice under E's stencil, ref 0 / 1 - wsee_split_draw).
    // Uncovered pixels, and every pass of a run without a valid capture (mode 0, nulls bound): the pre-WSEE picture. D3
    // is resolved from D3m once before a reader binds it (wsee_d3m_resolve, PSWseeD3Res; in_injection only on the
    // render thread - WSEE_D3M_RT): D3 = r where the opaque's z lies within KH_WSEE_D3_TOL (8 D24 steps) of the live
    // sample 0, else z. REGISTERS (the FX unit; StateBackup saves t0 - t69): t53 S, t54 T, t55 B, t56 R (the chain's
    // input), t57 NWpre, t58 E, t59 C, t60 B2, t61 NWpost, t62 S2, t63 Tp, t64 Tf, t65 D3, t66 P0, t67 P1, t68 N, t69
    // N2. The mesh unit (the sort scope): t53 S, t54 T, t55 B, t56 the live depth (MS), t57 NWpre, t58 Q, t59 NWpost,
    // t60 B2, t62 S2, t65 D3, t66 - t69 P0 / P1 / N / N2. depth_resolve: u3 D3 (store path); PSWseeOneZ* targets T, Tp,
    // D3m. LANES (C++ fx_tex[1][2..3] = HLSL khFxTexLay[1].zw): .z = 1 armed (an effect pass) / the sort mode (a mesh
    // draw); .w bits: 0 E bound, 1 C bound, 2 per-sample path allowed, 3 the B2 moment stood (S2 bound; B2 too where a
    // colour want resolved it), 4 Tp / Tf, 5 D3, 6 the one pass's record drew this cycle, 7 the old record holds no
    // particle layer, 8 a pre late opaque was skipped (derived), 9 S2 bound as the derivation's reference. Bits 10 - 11
    // retired (s fixed). fx_fog[0] on a sorted draw = the depth decode (m22, m32, the world's MinDepth / MaxDepth).
    // TWINS: KH_WSEE_SURF_TOL 0.02, KH_WSEE_NEAR_TOL 0.2, KH_WSEE_NEAR_TOL2 0.5 = cb.hlsl's; PSB_LS 1 =
    // KH_WSEE_PSB_LS = KH_PSB_ZB_LS (static_assert); D = far / 4 = KhWseeFarRaw; the one pass's cb layout = the
    // patcher's prologue (psb_post's note); the PASS_* table ("The re-issues' passes") = wsee_re_set's binds.
    // TAINT (wsee_taint: the capture stays unused this cycle): a depth write of ours on the main depth after it
    // landed (a late injection, the flush's solids, white_draw); a re-issue refused or impossible (UAV or SO target
    // bound, the late opaque's PS bound with class instances, a target or the bound depth gone); > WSEE_OPAQUE_CAP
    // replays; an unblended colour draw with the depth test off (overwrites B without a depth); a DrawAuto / indirect
    // draw writing the depth (WSEE_XDRAW); S2 not takeable in a cycle that skipped a pre replay. NOT COVERED:
    // several layers of one draw overlapping (T counts each fragment, NW the nearest); two layers of one phase both off
    // any surface share the nearer's depth; a no-write blended draw that tests stencil (left out); a late opaque whose
    // PS writes its own depth (EQUAL fails: G stays 0); a layer over a late opaque not coplanar with it (behind = r);
    // multiply / screen / subtractive / MIN-MAX blends (not in the record); a particle-aware layer drawn after B2 but
    // behind a later fence (post: fogged at the fence); the effect meshes (unarmed); FSAA 1x (the hooks do not take the
    // world's draws there); the one pass's 2 x 2 blocks (an edge, a dithered fade or a spark narrower than 2 px moves
    // at block steps; the last W mod 2 pixels read their neighbour block); our dynamic-light world shadow
    // (dls_world_pass multiplies the scene at the resolve, after B / B2 were taken) is not in B / B2, so under a
    // layer the distance fog's exact stages and KhWseeBehindColor rebuild a world surface our meshes shadow from a
    // dynamic light unshadowed, and our sorted parts' composite (modes 2 / 3: B / B2 against Q, which holds the shadow)
    // darkens that surface further behind them (held by the stacking cap). COST, wanted cycles: the capture (one R32F
    // pass, one resolve when wanted, clears, Zb), each qualifying draw once more (the one pass at 1 / 4 the fragments;
    // else two to five passes), each post late opaque's vertex work again, per run E and, where the last pass reads C,
    // one scene copy; the D3 resolve. VRAM at 3840x2160 8x: about 1.2 GB (T 265 MB, Tp 265, D3m 265, S / NW / NW2 / S2
    // / D3 / Tf's depth 33 each, Tf 17, B / B2 33 - 66, E 33 + stencil 33, P / N 33, Zb 17), plus Q and the hold once
    // the sort runs; nothing checks it against the card's memory. Unwanted: one bool a draw. The re-issues' saves, null
    // outside one wsee_re_save / wsee_re_restore pair (render thread; the tail keeps its own).
    ID3D11RenderTargetView*  rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView*  dsv = nullptr;
    ID3D11BlendState*        bs = nullptr;
    FLOAT                    bf[4] = {};
    UINT                     mask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     ref = 0;
    ID3D11PixelShader*       ps = nullptr;      // The late opaque's, or a PSV pick's original.
    bool                     has_ps = false;    // g_wsee_sv.ps was taken (null is a valid shader).
    // The engine's PS t0, taken with the pixel shader where the late-opaque pass binds the main depth's view there
    // (wsee_od_on; null is a valid binding).
    ID3D11ShaderResourceView* srv0 = nullptr;
    bool                      has_srv0 = false;
    // WSEE_OQ2: this post late opaque's replay takes the D3m path (wsee_d3m_path at the save: no t0 taken then).
    bool                      d3m = false;
};
static WseeSaveState g_wsee_sv;
// The tracked draw's viewports, read once in wsee_pre_draw (the whole bound array; the slice and world tests take
// entry 0) and used by its post (WseeSurfVp, wsee_vp_world / wsee_vp_beyond, psb_post). Valid because
// nothing sets a viewport between the two: the snapshot / B2 inside the pre-draw set theirs back (StateBackup), the
// engine's own draw is the one call in between, the post's OM binds run under in_injection (the OM hooks stand aside)
// and RSSetViewports is not hooked. g_wsee_pd_vp_ok: set by the read, cleared where the pre-draw begins and where the
// post takes it; the flush's tail (wsee_tail) never reads it. Render thread.
static D3D11_VIEWPORT           g_wsee_pd_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
static UINT                     g_wsee_pd_vpn = 0;
static bool                     g_wsee_pd_vp_ok = false;
inline void wsee_pd_vp_read(ID3D11DeviceContext* ctx) {
    g_wsee_pd_vpn = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetViewports(&g_wsee_pd_vpn, g_wsee_pd_vp);
    g_wsee_pd_vp_ok = g_wsee_pd_vpn >= 1 && g_wsee_pd_vpn <= D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
}
static constexpr uint32_t WSEE_WASTE_RUN = 8u;      // Tainted or refused attempts in a row before a back-off.
static constexpr uint64_t WSEE_BACKOFF = 120u;      // Cycles a back-off stands the capture down.
static constexpr uint64_t WSEE_DEFER_BACKOFF = 120u;   // Cycles a lost deferral stands the deferral down.
static constexpr uint64_t WSEE_IDLE_FREE = 600u;    // Cycles without a want before the targets are released.
static uint32_t g_wsee_waste_n = 0;        // Render thread: the current run of wasted attempts.
static uint64_t g_wsee_backoff_end = 0;    // Render thread: no capture before this cycle.
// WSEE_FLAP - a capture used on some cycles and wasted on others, cycle about (an opaque count about the cap, a
// flush depth write every other frame, a re-issue refused on alternate draws) never reaches WSEE_WASTE_RUN, so the
// chain would arm and disarm frame by frame (the fog through a fence, our glass under its layers, on and off at frame
// rate: worse than either look held). Verdict changes (used <-> wasted, between consecutive attempted cycles) are
// counted over WSEE_FLAP_WIN cycles; WSEE_FLAP_N in a window begin the plain back-off (WSEE_BACKOFF: the
// pre-record picture, held), then the counter starts afresh. A run of used cycles, or of wasted ones, counts nothing.
// Render thread (wsee_cycle_note); reset with g_wsee_waste_n.
static constexpr uint64_t WSEE_FLAP_WIN = 64u;
static constexpr uint32_t WSEE_FLAP_N = 16u;
static int      g_wsee_flap_prev = -1;     // The last attempted cycle's verdict (1 used, 0 wasted; -1 none yet).
static uint64_t g_wsee_flap_win_start = 0;
static uint32_t g_wsee_flap_n = 0;
// WSEE_FLUSHW_PRED - a flush solid of ours that writes the main depth (flush_locked's dss write arm) taints every
// capture it follows; where one draws frame after frame the plain back-off pays 8 captures in every 128 cycles for
// nothing. g_wsee_flushw_cycle: the last cycle such a write followed a landed capture, or fell in a cycle this rule
// stood down. Where a waste run (WSEE_WASTE_RUN) ends with the ended cycle's flush write, the hold
// (g_wsee_flushw_hold) replaces the fixed back-off: a cycle after a stamped one captures nothing, but for one probe
// WSEE_FLUSHW_PROBE (= WSEE_BACKOFF) cycles after the last capture the cause spoiled (g_wsee_flushw_probe); a
// cycle after an unstamped one captures; a used capture ends the hold. The picture of a stood-down cycle is the
// backed-off one (the live depth). Trade against the fixed back-off: the cause stopping resumes the capture the next
// write-free cycle instead of after up to 120, and a persistent cause costs one capture per probe instead of 8; but the
// first write-free cycle after the cause stops is stood down where the back-off's 8-capture window would have taken it
// (one frame later), and a park writing before the capture point in stood-down cycles keeps them down until the probe.
static constexpr uint64_t WSEE_FLUSHW_PROBE = WSEE_BACKOFF;
static std::atomic<uint64_t> g_wsee_flushw_cycle{ ~0ull };   // Either thread (the flush; a park's).
static std::atomic<uint64_t> g_wsee_flushw_off{ ~0ull };     // Render thread writes (the clear); the flush reads.
static uint64_t g_wsee_flushw_probe = 0u;                     // Render thread.
static bool     g_wsee_flushw_hold = false;                   // Render thread.

// PSV - the engine's particle pixel shader, reduced for the record's passes (wsee_post: RE_NOWRITE,
// RE_NOWRITE_OFF, RE_NWONLY; wsee_tail). D3D11 cannot read a created shader's bytecode back, so it is copied at
// CreatePixelShader (the hook below) and patched in place: "alpha" keeps only the .w lane of every o0 write, removes
// every other colour write and sets o0.xyz = 0 once after the declarations (the passes' source factor is ZERO); "nw"
// removes every colour write (discards, oDepth, oMask and what feeds them stay) - a shader with no discard and no
// depth / coverage output binds no PS at all for the NW pass. Declarations (nw: minus forceEarlyDepthStencil, below),
// control flow, derivatives, helper lanes and both signatures are unchanged (the engine's VS still links). o0.w
// is computed by the same instructions in the same order, so every pass of one re-issue takes one alpha; the driver
// may fuse differently in the reduced program (refactoringAllowed): at worst one UNORM8 step at a rounding boundary,
// and a cut-out within one step of its threshold may be decided the other way in NW (a second program) than in T.
// A two-destination op (sincos, udiv) whose colour destination goes writes null there. Refused (the engine's shader
// draws today's passes; the code named once in the RPT - PSV_REFUSE_REPORT, psv_make): a malformed container /
// program, an output written through a relative index, class linkage, a UAV, a read of an output, a tiled-resource
// feedback op, (alpha) an o0 without .w - PSV_R_* below.
// The patcher is pure (bytes in, bytes out; no D3D call, no global): it runs standalone on a blob.
static constexpr uint32_t PSV_MODE_ALPHA = 0u;   // T, Tp, Tp2, Tf.
static constexpr uint32_t PSV_MODE_NW = 1u;      // NW.
static constexpr uint32_t PSV_OK = 0u;           // psv_patch's out holds the variant.
static constexpr uint32_t PSV_NOPS = 1u;         // nw: bind no pixel shader (psv_patch's out untouched).
static constexpr uint32_t PSV_R_CONTAINER = 2u;  // Refusals.
static constexpr uint32_t PSV_R_PROGRAM = 3u;
static constexpr uint32_t PSV_R_TOKENS = 4u;
static constexpr uint32_t PSV_R_RELOUT = 5u;
static constexpr uint32_t PSV_R_LINKAGE = 6u;
static constexpr uint32_t PSV_R_UAV = 7u;
static constexpr uint32_t PSV_R_NOALPHA = 8u;
static constexpr uint32_t PSV_R_OUTREAD = 9u;
static constexpr uint32_t PSV_R_DEPTHOUT = 10u;  // A depth / coverage output (psb_patch's refusal).
static constexpr uint32_t PSV_R_FEEDBACK = 12u;  // A tiled-resource feedback op (a status destination).
// The container's checksum (bytes 4 - 19): MD5's transform over bytes 20 - end with DXBC's own last block - the bit
// count in its first dword and (bits >> 2) | 1 in its last (a block of its own when 56 bytes or more remain). The
// runtime refuses a blob whose checksum does not match; this one reproduces fxc's on its blobs.
inline void psv_md5(uint32_t s[4], const uint8_t* b) {
    static const uint32_t k[64] = {
        0xD76AA478u, 0xE8C7B756u, 0x242070DBu, 0xC1BDCEEEu, 0xF57C0FAFu, 0x4787C62Au, 0xA8304613u, 0xFD469501u,
        0x698098D8u, 0x8B44F7AFu, 0xFFFF5BB1u, 0x895CD7BEu, 0x6B901122u, 0xFD987193u, 0xA679438Eu, 0x49B40821u,
        0xF61E2562u, 0xC040B340u, 0x265E5A51u, 0xE9B6C7AAu, 0xD62F105Du, 0x02441453u, 0xD8A1E681u, 0xE7D3FBC8u,
        0x21E1CDE6u, 0xC33707D6u, 0xF4D50D87u, 0x455A14EDu, 0xA9E3E905u, 0xFCEFA3F8u, 0x676F02D9u, 0x8D2A4C8Au,
        0xFFFA3942u, 0x8771F681u, 0x6D9D6122u, 0xFDE5380Cu, 0xA4BEEA44u, 0x4BDECFA9u, 0xF6BB4B60u, 0xBEBFBC70u,
        0x289B7EC6u, 0xEAA127FAu, 0xD4EF3085u, 0x04881D05u, 0xD9D4D039u, 0xE6DB99E5u, 0x1FA27CF8u, 0xC4AC5665u,
        0xF4292244u, 0x432AFF97u, 0xAB9423A7u, 0xFC93A039u, 0x655B59C3u, 0x8F0CCC92u, 0xFFEFF47Du, 0x85845DD1u,
        0x6FA87E4Fu, 0xFE2CE6E0u, 0xA3014314u, 0x4E0811A1u, 0xF7537E82u, 0xBD3AF235u, 0x2AD7D2BBu, 0xEB86D391u };
    static const uint32_t r[16] = { 7u, 12u, 17u, 22u, 5u, 9u, 14u, 20u, 4u, 11u, 16u, 23u, 6u, 10u, 15u, 21u };
    uint32_t m[16];
    for (int i = 0; i < 16; ++i) {
        m[i] = static_cast<uint32_t>(b[4 * i]) |
               (static_cast<uint32_t>(b[4 * i + 1]) << 8) |
               (static_cast<uint32_t>(b[4 * i + 2]) << 16) |
               (static_cast<uint32_t>(b[4 * i + 3]) << 24);
    }
    uint32_t a = s[0], bb = s[1], c = s[2], d = s[3];
    for (int i = 0; i < 64; ++i) {
        uint32_t f = 0u;
        int g = 0;
        if (i < 16) {
            f = (bb & c) | (~bb & d);
            g = i;
        } else if (i < 32) {
            f = (d & bb) | (~d & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = bb ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (bb | ~d);
            g = (7 * i) % 16;
        }
        f = f + a + k[i] + m[g];
        a = d;
        d = c;
        c = bb;
        const uint32_t sh = r[(i / 16) * 4 + (i % 4)];
        bb = bb + ((f << sh) | (f >> (32u - sh)));
    }
    s[0] += a;
    s[1] += bb;
    s[2] += c;
    s[3] += d;
}
inline void psv_put32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}
inline uint32_t psv_get32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
inline void psv_checksum(const uint8_t* b, size_t n, uint32_t out[4]) {
    uint32_t s[4] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };
    const uint8_t* const d = b + 20;
    const size_t len = n - 20u;
    const size_t full = len - len % 64u;
    for (size_t o = 0; o < full; o += 64u) psv_md5(s, d + o);
    const size_t left = len - full;
    const uint32_t bits = static_cast<uint32_t>(len * 8u);
    uint8_t blk[64];
    memset(blk, 0, sizeof(blk));
    if (left >= 56u) {
        memcpy(blk, d + full, left);
        blk[left] = 0x80u;
        psv_md5(s, blk);
        memset(blk, 0, sizeof(blk));
        psv_put32(blk, bits);
        psv_put32(blk + 60, (bits >> 2) | 1u);
    } else {
        psv_put32(blk, bits);
        memcpy(blk + 4, d + full, left);
        blk[4 + left] = 0x80u;
        psv_put32(blk + 60, (bits >> 2) | 1u);
    }
    psv_md5(s, blk);
    for (int i = 0; i < 4; ++i) out[i] = s[i];
}
// One operand's length in dwords at t[i] (its extended tokens, indices and relative-index operands
// included); 0 = malformed (past end, or an index representation SM4 / SM5 does not define).
inline size_t psv_oplen(const uint32_t* t, size_t i, size_t end, int depth = 0) {
    if (i >= end || depth > 4) return 0;
    const uint32_t tk = t[i];
    size_t n = 1;
    bool ext = (tk >> 31) != 0u;
    while (ext) {
        if (i + n >= end) return 0;
        ext = (t[i + n] >> 31) != 0u;
        ++n;
    }
    const uint32_t type = (tk >> 12) & 0xFFu;
    const uint32_t nc = tk & 3u;
    if (type == 4u) return i + n + (nc == 1u ? 1u : 4u) <= end ?
                           n + (nc == 1u ? 1u : 4u) : 0;   // Immediate 32-bit.
    if (type == 5u) return i + n + (nc == 1u ? 2u : 4u) <= end ?
                           n + (nc == 1u ? 2u : 4u) : 0;   // Immediate 64-bit: one double, or two
                                                                           // (xy / zw) in four dwords.
    const uint32_t dim = (tk >> 20) & 3u;
    for (uint32_t k = 0; k < dim; ++k) {
        const uint32_t rep = (tk >> (22u + 3u * k)) & 7u;
        if (rep == 0u) {
            n += 1;
        } else if (rep == 1u) {
            n += 2;
        } else if (rep >= 2u && rep <= 4u) {
            n += rep == 2u ? 0u : rep == 3u ? 1u : 2u;
            const size_t r = psv_oplen(t, i + n, end, depth + 1);
            if (r == 0) return 0;
            n += r;
        } else {
            return 0;
        }
        if (i + n > end) return 0;
    }
    return n;
}
// The output operand at t[i] (end its end): the position of its one register index - an immediate
// below 8 past its extended tokens -, else 0 (indexed at run time, two-dimensional, or past the end).
inline size_t psv_reg0(const uint32_t* t, size_t i, size_t end) {
    const uint32_t tk = t[i];
    if (((tk >> 20) & 3u) != 1u || ((tk >> 22) & 7u) != 0u) return 0;
    size_t x = i + 1;
    bool ext = (tk >> 31) != 0u;
    while (ext) {
        if (x >= end) return 0;
        ext = (t[x] >> 31) != 0u;
        ++x;
    }
    return x < end && t[x] < 8u ? x : 0;
}
// The variant of the container b (n bytes) for mode into out (PSV_OK), PSV_NOPS (nw: no
// pixel shader needed; out untouched), or a refusal (PSV_R_*; out untouched). The chunks are copied in
// their order and SHEX / SHDR replaced; the header's size, offsets and checksum are rebuilt.
inline uint32_t psv_patch(const uint8_t* b, size_t n, uint32_t mode,
                          std::vector<uint8_t>& out) {
    if (!b || n < 32u || n > 0x4000000u || memcmp(b, "DXBC", 4) != 0 ||
        psv_get32(b + 24) != n) return PSV_R_CONTAINER;
    const uint32_t cn = psv_get32(b + 28);
    if (cn == 0u || cn > 64u || 32u + 4u * static_cast<size_t>(cn) > n) return PSV_R_CONTAINER;
    size_t sx = 0, sxn = 0;   // The program chunk's payload.
    uint32_t sxi = cn;
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t o = psv_get32(b + 32 + 4 * c);
        if (o + 8u > n) return PSV_R_CONTAINER;
        const size_t cs = psv_get32(b + o + 4);
        if (cs > n - o - 8u || (cs & 3u) != 0u) return PSV_R_CONTAINER;
        if (memcmp(b + o, "SHEX", 4) == 0 || memcmp(b + o, "SHDR", 4) == 0) {
            if (sxi != cn) return PSV_R_CONTAINER;   // Two programs.
            sxi = c;
            sx = o + 8u;
            sxn = cs;
        }
    }
    if (sxi == cn || sxn < 8u) return PSV_R_CONTAINER;
    std::vector<uint32_t> t(sxn / 4u);
    for (size_t i = 0; i < t.size(); ++i) {
        t[i] = psv_get32(b + sx + 4 * i);
    }
    const size_t len = t[1];
    if (((t[0] >> 16) & 0xFFFFu) != 0u || len < 2u || len > t.size()) return PSV_R_PROGRAM;
    std::vector<uint32_t> o;   // The new program.
    o.reserve(len + 16u);
    o.push_back(t[0]);
    o.push_back(0u);   // Its length, below.
    uint32_t dmask[8] = {};   // Declared colour outputs' masks (o0 - o7).
    bool dz = false;   // A depth / coverage / stencil output declared.
    bool disc = false;
    size_t body = 0;   // Where the first instruction past the declarations went in o (0 = none yet).
    size_t i = 2;
    while (i < len) {
        const uint32_t tk = t[i];
        const uint32_t op = tk & 0x7FFu;
        size_t ln = 0;
        if (op == 53u) {   // customdata (the immediate constant buffer): its length in the next dword.
            if (i + 1u >= len) return PSV_R_TOKENS;
            ln = t[i + 1];
            if (ln < 2u) return PSV_R_TOKENS;
        } else {
            ln = (tk >> 24) & 0x7Fu;
        }
        if (ln == 0u || ln > len - i) return PSV_R_TOKENS;
        const bool dcl = op == 53u || (op >= 88u && op <= 106u) ||
                         (op >= 143u && op <= 162u) || (op >= 113u && op <= 116u) ||
                         op == 206u;   // dcl_gs_instance_count (not a PS's; copied if met).
        if (dcl) {
            if (op == 144u || op == 145u || op == 146u) return PSV_R_LINKAGE;   // Function bodies.
            if (op >= 156u && op <= 160u) return PSV_R_UAV;   // UAVs, group-shared memory.
            if ((op >= 101u && op <= 103u) || op == 91u) {   // dcl_output*, dcl_index_range.
                if (ln < 2u) return PSV_R_TOKENS;
                const uint32_t d = t[i + 1];
                const uint32_t ty = (d >> 12) & 0xFFu;
                if (ty == 2u) {
                    if (op == 91u) return PSV_R_RELOUT;   // An output range indexed at run time.
                    const size_t x = psv_reg0(t.data(), i + 1, i + ln);
                    if (x == 0u) return PSV_R_RELOUT;
                    dmask[t[x]] |= (d & 3u) == 2u && ((d >> 2) & 3u) == 0u ?
                                   (d >> 4) & 0xFu : 0xFu;
                } else if (ty == 12u || ty == 15u || ty == 38u || ty == 39u || ty == 41u) {
                    dz = true;   // oDepth (any), oMask, oStencilRef.
                }
            }
            o.insert(o.end(), t.begin() + static_cast<std::ptrdiff_t>(i),
                     t.begin() + static_cast<std::ptrdiff_t>(i + ln));
            // PSV: nw drops dcl_globalFlags' forceEarlyDepthStencil (bit 13) - the NW pass writes depth, and forced
            // early depth would write it before the shader's discard (no UAV here; an SV_Coverage input would no longer
            // reflect the early test - no engine shader seen carries the flag).
            if (mode == PSV_MODE_NW && op == 106u) o[o.size() - ln] &= ~(1u << 13);
            i += ln;
            continue;
        }
        if (body == 0) body = o.size();
        if (op == 120u) return PSV_R_LINKAGE;   // fcall.
        // The tiled-resource feedback ops (gather4 / ld / sample ... with a status destination, 219 - 233): a
        // dropped colour write would take their status write with it - refused whole (none in the engine's export).
        if (op >= 219u && op <= 233u) return PSV_R_FEEDBACK;
        if (op == 13u) disc = true;   // discard.
        // The operands, one by one, to the instruction's end exactly; the colour outputs among them noted.
        size_t j = i + 1;
        if ((tk >> 31) != 0u) {
            bool e = true;
            while (e) {
                if (j >= i + ln) return PSV_R_TOKENS;
                e = (t[j] >> 31) != 0u;
                ++j;
            }
        }
        // Two-destination instructions: sincos, udiv, umul, imul, uaddc, usubb, swapc.
        const bool two = op == 77u || op == 78u || op == 81u || op == 38u ||
                         op == 132u || op == 133u || op == 142u;
        size_t dp[2] = { 0, 0 };   // The destinations' positions (an instruction has at most two) and lengths.
        size_t dl[2] = { 0, 0 };
        uint32_t dty[2] = { 0xFFu, 0xFFu };   // Their operand types.
        uint32_t reg[2] = { 0u, 0u };   // A colour output's register and mask.
        uint32_t msk[2] = { 0u, 0u };
        bool any = false;
        int k = 0;
        while (j < i + ln) {
            const size_t ol = psv_oplen(t.data(), j, i + ln);
            if (ol == 0u) return PSV_R_TOKENS;
            const uint32_t q = t[j];
            const uint32_t qt = (q >> 12) & 0xFFu;
            if (k < 2) {
                dp[k] = j;
                dl[k] = ol;
                dty[k] = qt;
            }
            if (qt == 2u) {   // An output register: written (outputs are write-only), so a destination.
                if (k > 1 || (k == 1 && !two)) return PSV_R_OUTREAD;
                const size_t x = psv_reg0(t.data(), j, j + ol);
                if (x == 0u) return PSV_R_RELOUT;
                if ((q & 3u) != 2u || ((q >> 2) & 3u) != 0u) return PSV_R_TOKENS;   // Not a mask.
                reg[k] = t[x];
                msk[k] = (q >> 4) & 0xFu;
                any = true;
            }
            j += ol;
            ++k;
        }
        if (j != i + ln) return PSV_R_TOKENS;
        if (!any) {   // No colour destination: copied whole.
            o.insert(o.end(), t.begin() + static_cast<std::ptrdiff_t>(i),
                     t.begin() + static_cast<std::ptrdiff_t>(i + ln));
            i += ln;
            continue;
        }
        // Each colour destination kept as o0.w (alpha, o0 written with .w) or dropped; a dropped one becomes the null
        // operand where the instruction's other destination still writes, else the instruction goes.
        const int nd = two ? 2 : 1;
        bool keep[2] = { false, false };
        bool live = false;   // A destination still written.
        for (int d = 0; d < nd; ++d) {
            if (dty[d] == 2u) {
                keep[d] = mode == PSV_MODE_ALPHA && reg[d] == 0u &&
                          (msk[d] & 8u) != 0u;
                if (keep[d]) live = true;
            } else if (dty[d] != 13u) {
                live = true;   // Not null: a register written.
            }
        }
        if (live) {
            const size_t at = o.size();
            o.push_back(0u);   // The opcode token, its length set below.
            for (size_t x = i + 1; x < i + ln;) {
                int d = -1;
                if (x == dp[0] && dty[0] == 2u) d = 0;
                if (two && x == dp[1] && dty[1] == 2u) d = 1;
                if (d < 0) {
                    o.push_back(t[x]);
                    ++x;
                } else if (keep[d]) {
                    o.push_back((t[x] & ~(0xFu << 4)) | (8u << 4));   // o0.w alone.
                    o.insert(o.end(), t.begin() + static_cast<std::ptrdiff_t>(x + 1),
                             t.begin() + static_cast<std::ptrdiff_t>(x + dl[d]));
                    x += dl[d];
                } else {
                    o.push_back(13u << 12);   // null.
                    x += dl[d];
                }
            }
            const size_t nl = o.size() - at;
            o[at] = (tk & ~(0x7Fu << 24)) | (static_cast<uint32_t>(nl) << 24);
        }
        i += ln;
    }
    if (i != len) return PSV_R_TOKENS;
    if (body == 0) body = o.size();
    if (mode == PSV_MODE_NW && !disc && !dz) return PSV_NOPS;
    if (mode != PSV_MODE_NW) {
        if ((dmask[0] & 8u) == 0u) return PSV_R_NOALPHA;   // alpha.
        // mov o0.<mask>, l(0, 0, 0, 0): xyz (the declared ones).
        const uint32_t mm = dmask[0] & 7u;
        if (mm != 0u) {
            const uint32_t mov[8] = { 54u | (8u << 24), 2u | (mm << 4) | (2u << 12) | (1u << 20), 0u,
                                      2u | (4u << 12), 0u, 0u, 0u, 0u };
            o.insert(o.begin() + static_cast<std::ptrdiff_t>(body), mov, mov + 8);
        }
    }
    o[1] = static_cast<uint32_t>(o.size());
    // The container again: the chunks in their order, the program replaced.
    size_t total = 32u + 4u * cn;
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t co = psv_get32(b + 32 + 4 * c);
        total += 8u + (c == sxi ? 4u * o.size() : psv_get32(b + co + 4));
    }
    std::vector<uint8_t> nb(total, 0u);
    memcpy(nb.data(), b, 20);
    psv_put32(nb.data() + 20, psv_get32(b + 20));
    psv_put32(nb.data() + 24, static_cast<uint32_t>(total));
    psv_put32(nb.data() + 28, cn);
    size_t w = 32u + 4u * cn;
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t co = psv_get32(b + 32 + 4 * c);
        psv_put32(nb.data() + 32 + 4 * c, static_cast<uint32_t>(w));
        memcpy(nb.data() + w, b + co, 4);
        if (c == sxi) {
            psv_put32(nb.data() + w + 4, static_cast<uint32_t>(4u * o.size()));
            for (size_t q = 0; q < o.size(); ++q) {
                psv_put32(nb.data() + w + 8 + 4 * q, o[q]);
            }
            w += 8u + 4u * o.size();
        } else {
            const size_t cs = psv_get32(b + co + 4);
            memcpy(nb.data() + w + 4, b + co + 4, 4u + cs);
            w += 8u + cs;
        }
    }
    uint32_t sum[4];
    psv_checksum(nb.data(), nb.size(), sum);
    for (int q = 0; q < 4; ++q) psv_put32(nb.data() + 4 + 4 * q, sum[q]);
    out.swap(nb);
    return PSV_OK;
}
// PSB - the one pass: a third variant of the same bytes that draws the particle-type record in ONE single-sample
// draw with no depth view (psb_post). Prologue (before the engine's code): z = SV_Position.z; the pushes
// z_i = z m_i + a_i (the viewport shift's algebra, (m_i, a_i) = (1 - TOL_i, TOL_i Hi) for TOL 0.02 / 0.2 / 0.5 where
// the draw's shift stood, else (1, 0); cb at a free slot); the reference r = the largest sample 0 of the block's s x s
// pixels (the depth's MS view at a free t slot, s = 1 << PSB_LS = 2; the loop stops once r reaches every bound it is
// needed for, Zb bounding it while cb3.w stands); verdicts z_i <= r (or all passed for the test-off kind), far = z > D
// and r >= Dq; a fragment no verdict passes whose NW write is a no-op (z_1 >= 1) is discarded. Then the alpha program
// with o0.w into a temp, and before every ret the epilogue: o0 = (aT, aP1, aP2, aF) (a where the verdict passed, +0
// where not), o1.x = z_1. The blend states pick the phase's channels (C++). Signatures: SV_Position's used mask gains
// .xyz; the output signature becomes SV_Target0 - 1 (the input signature's layout is unchanged: the engine's VS still
// links). PSB_LORES: the engine's own SV_Position reads take a real pixel of the frame from a temp the prologue
// fills (a soft particle's depth fetch, a screen-space buffer, a dither cell address the frame as at full size; the
// formula at the prologue's end), so the one pass is drawn at 1 / s of the frame (psb_post scales the viewports)
// and the constant buffer's fourth row says s and the reads.
// Refused (PSV_R_* / PSB_R_*; the alpha / nw passes draw instead): every alpha refusal; an output other than
// one float SV_Target0 with .w; no SV_Position xyz input; a read of SV_Position.z (its alpha would differ from the
// passes', each pushed its own); a per-sample / coverage input or an eval / samplepos / samplecount op (one sample);
// subroutines; more than PSB_RET_MAX rets; no free cb / t slot; a depth / coverage output (the tail's PSMain /
// PSComposite); an input register SV_Position shares with another element or read by a run-time index.
static constexpr uint32_t PSB_R_SIG = 13u;     // The signatures (ISGN / OSGN) are not as the one pass needs.
static constexpr uint32_t PSB_R_NOPOS = 14u;   // No float SV_Position with .xyz in the input signature.
static constexpr uint32_t PSB_R_POSZ = 15u;    // The shader reads SV_Position.z itself.
static constexpr uint32_t PSB_R_RATE = 16u;    // A per-sample / coverage input, an eval or sample-position op.
static constexpr uint32_t PSB_R_SUB = 17u;     // Subroutines of its own (label / call).
static constexpr uint32_t PSB_R_RETS = 19u;    // More rets than PSB_RET_MAX (each gets the epilogue inlined).
static constexpr uint32_t PSB_RET_MAX = 16u;   // Rets a variant may inline the epilogue at.
static constexpr uint32_t PSB_R_SLOT = 18u;    // No free constant-buffer or resource slot.
static constexpr uint32_t PSB_R_POSX = 20u;    // PSB_LORES: an input read by a run-time index.
static constexpr uint32_t PSB_TEMPS = 4u;      // The temps the variant adds (alpha, pushes, loop, address).
// One SM5 operand token of the epilogue: kind 0 = a destination mask, 1 = a four-component swizzle, 2 = one
// component; dim the index dimension.
inline uint32_t psb_op(uint32_t type, uint32_t kind, uint32_t sel, uint32_t dim) {
    return 2u | (kind << 2) | (sel << 4) | (type << 12) | (dim << 20);
}
// PSB_LORES: the operand at t[i] (end its end) and every index operand inside it, in place - a read
// of the input register pr (SV_Position's) becomes a read of temp rd (its type and index alone: components,
// swizzle and modifiers kept). False: an input read by a run-time or two-dimensional index (a pixel shader needs
// dcl_indexRange for that, which the patcher refuses anyway) or a malformed operand.
inline bool psb_pos_fix(uint32_t* t, size_t i, size_t end, uint32_t pr, uint32_t rd,
                        int depth = 0) {
    if (i >= end || depth > 4) return false;
    const uint32_t tk = t[i];
    size_t n = 1;
    bool ext = (tk >> 31) != 0u;
    while (ext) {
        if (i + n >= end) return false;
        ext = (t[i + n] >> 31) != 0u;
        ++n;
    }
    const uint32_t type = (tk >> 12) & 0xFFu;
    if (type == 4u || type == 5u) return true;   // Immediates.
    const uint32_t dim = (tk >> 20) & 3u;
    if (type == 1u) {   // An input: one immediate index.
        if (dim != 1u || ((tk >> 22) & 7u) != 0u || i + n >= end) return false;
        if (t[i + n] == pr) {
            t[i] = tk & ~(0xFFu << 12);   // TEMP (type 0).
            t[i + n] = rd;
        }
        return true;
    }
    for (uint32_t k = 0; k < dim; ++k) {
        const uint32_t rep = (tk >> (22u + 3u * k)) & 7u;
        if (rep == 0u) {
            n += 1;
        } else if (rep == 1u) {
            n += 2;
        } else if (rep >= 2u && rep <= 4u) {
            n += rep == 2u ? 0u : rep == 3u ? 1u : 2u;
            const size_t r = psv_oplen(t, i + n, end, depth + 1);
            if (r == 0 ||
                !psb_pos_fix(t, i + n, i + n + r, pr, rd, depth + 1)) {
                return false;
            }
            n += r;
        } else {
            return false;
        }
        if (i + n > end) return false;
    }
    return true;
}
// The variant "one" of the container b (n bytes) into out (PSV_OK), its constant-buffer slot in
// cbs and its depth view's slot in ts (PSB_ZB: Zb's, the next free below it, in tzs); or a refusal
// (PSV_R_* / PSB_R_*; the outputs untouched).
inline uint32_t psb_patch(const uint8_t* b, size_t n, std::vector<uint8_t>& out,
                          uint32_t& cbs, uint32_t& ts, uint32_t& tzs) {
    if (!b || n < 32u || n > 0x4000000u || memcmp(b, "DXBC", 4) != 0 ||
        psv_get32(b + 24) != n) return PSV_R_CONTAINER;
    const uint32_t cn = psv_get32(b + 28);
    if (cn == 0u || cn > 64u || 32u + 4u * static_cast<size_t>(cn) > n) return PSV_R_CONTAINER;
    uint32_t xi = cn, ii = cn, oi = cn;   // SHEX, ISGN, OSGN.
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t o = psv_get32(b + 32 + 4 * c);
        if (o + 8u > n) return PSV_R_CONTAINER;
        const size_t cs = psv_get32(b + o + 4);
        if (cs > n - o - 8u || (cs & 3u) != 0u) return PSV_R_CONTAINER;
        const uint8_t* const g = b + o;
        if (memcmp(g, "SHEX", 4) == 0 || memcmp(g, "SHDR", 4) == 0) {
            if (xi != cn) return PSV_R_CONTAINER;   // Two programs.
            xi = c;
        } else if (memcmp(g, "ISGN", 4) == 0) {
            if (ii != cn) return PSB_R_SIG;
            ii = c;
        } else if (memcmp(g, "OSGN", 4) == 0) {
            if (oi != cn) return PSB_R_SIG;
            oi = c;
        } else if (memcmp(g, "ISG1", 4) == 0 || memcmp(g, "OSG1", 4) == 0 ||
                   memcmp(g, "OSG5", 4) == 0 || memcmp(g, "PCSG", 4) == 0) {
            return PSB_R_SIG;
        }
    }
    if (xi == cn) return PSV_R_CONTAINER;
    const size_t xo = psv_get32(b + 32 + 4 * xi) + 8u;
    const size_t xn = psv_get32(b + xo - 4u);
    if (xn < 8u) return PSV_R_CONTAINER;
    std::vector<uint32_t> pb_t(xn / 4u);
    for (size_t w0 = 0; w0 < pb_t.size(); ++w0) {
        pb_t[w0] = psv_get32(b + xo + 4 * w0);
    }
    const size_t pb_len = pb_t[1];
    // A pixel shader of shader model 5 (the one pass's ldms / ld_indexable).
    if (((pb_t[0] >> 16) & 0xFFFFu) != 0u || ((pb_t[0] >> 4) & 0xFu) != 5u || pb_len < 3u ||
        pb_len > pb_t.size()) return PSV_R_PROGRAM;
    // The input signature's SV_Position (system value 1): its register and the element's used mask.
    if (ii == cn) return PSB_R_NOPOS;
    const size_t io = psv_get32(b + 32 + 4 * ii) + 8u;
    const size_t in = psv_get32(b + io - 4u);
    if (in < 8u || psv_get32(b + io + 4) != 8u) return PSB_R_SIG;
    const uint32_t ie = psv_get32(b + io);
    if (ie > 64u || 8u + 24u * static_cast<size_t>(ie) > in) return PSB_R_SIG;
    uint32_t pr = 0xFFFFFFFFu;   // SV_Position's register.
    size_t pe = 0;               // Its element's offset in the chunk's payload.
    for (uint32_t e = 0; e < ie; ++e) {
        const size_t eo = io + 8u + 24u * e;
        if (psv_get32(b + eo + 8) != 1u) continue;
        if (pr != 0xFFFFFFFFu) return PSB_R_SIG;
        if (psv_get32(b + eo + 12) != 3u || (b[eo + 20] & 7u) != 7u ||
            psv_get32(b + eo + 16) >= 32u) return PSB_R_NOPOS;
        pr = psv_get32(b + eo + 16);
        pe = eo - io;
    }
    if (pr == 0xFFFFFFFFu) return PSB_R_NOPOS;
    for (uint32_t e = 0; e < ie; ++e) {   // PSB_LORES: SV_Position's register its own.
        const size_t eo = io + 8u + 24u * e;
        if (eo - io != pe && psv_get32(b + eo + 16) == pr) return PSB_R_SIG;
    }
    // The declarations: their spans, the slots they take, the temps, the position input and o0.
    struct PsbDcl { size_t at; size_t ln; uint32_t op; };
    std::vector<PsbDcl> dl;
    bool cbu[14] = {};
    bool tu[128] = {};
    size_t tmp = 0;      // dcl_temps' count position (0 = none).
    size_t pos = 0;      // The position input's operand token (0 = not declared).
    size_t o0 = 0;       // o0's operand token.
    size_t i = 2;
    while (i < pb_len) {
        const uint32_t tk = pb_t[i];
        const uint32_t pb_op = tk & 0x7FFu;
        size_t pb_ln = 0;
        if (pb_op == 53u) {
            if (i + 1u >= pb_len) return PSV_R_TOKENS;
            pb_ln = pb_t[i + 1];
            if (pb_ln < 2u) return PSV_R_TOKENS;
        } else {
            pb_ln = (tk >> 24) & 0x7Fu;
        }
        if (pb_ln == 0u || pb_ln > pb_len - i) return PSV_R_TOKENS;
        const bool dcl = pb_op == 53u || (pb_op >= 88u && pb_op <= 106u) ||
                         (pb_op >= 143u && pb_op <= 162u) || (pb_op >= 113u && pb_op <= 116u) ||
                         pb_op == 206u;
        if (!dcl) break;
        if (pb_op == 144u || pb_op == 145u || pb_op == 146u) return PSV_R_LINKAGE;
        if (pb_op >= 156u && pb_op <= 160u) return PSV_R_UAV;
        const uint32_t d = pb_ln >= 2u ? pb_t[i + 1] : 0u;
        const uint32_t ty = (d >> 12) & 0xFFu;
        if (pb_op == 89u || pb_op == 88u || pb_op == 161u || pb_op == 162u) {
            // A slot taken (constant buffer: [slot][size]; resource: [slot]) - an immediate index past no
            // extended operand token.
            if (pb_ln < 3u || (d >> 31) != 0u || ((d >> 22) & 7u) != 0u) return PSV_R_TOKENS;
            const uint32_t s = pb_t[i + 2];
            if (pb_op == 89u) {
                if (((d >> 20) & 3u) != 2u) return PSV_R_TOKENS;
                if (s < 14u) cbu[s] = true;
            } else {
                if (((d >> 20) & 3u) != 1u) return PSV_R_TOKENS;
                if (s < 128u) tu[s] = true;
            }
        } else if (pb_op == 104u) {
            if (pb_ln != 2u || tmp != 0u) return PSV_R_TOKENS;
            tmp = i + 1;
        } else if (pb_op == 95u) {
            if (ty == 35u || ty == 42u) return PSB_R_RATE;   // vCoverage, vInnerCoverage.
        } else if (pb_op >= 98u && pb_op <= 100u) {
            const uint32_t im = (tk >> 11) & 0xFu;
            if (im == 6u || im == 7u) return PSB_R_RATE;   // Interpolated at each sample.
            if (pb_ln < 3u || ty != 1u || (d >> 31) != 0u || ((d >> 20) & 3u) != 1u ||
                ((d >> 22) & 7u) != 0u || (d & 3u) != 2u || ((d >> 2) & 3u) != 0u) {
                return PSV_R_TOKENS;
            }
            if (pb_op == 99u && pb_ln >= 4u && pb_t[i + 3] == 10u) return PSB_R_RATE;   // SampleIndex.
            if (pb_t[i + 2] == pr) {
                if (pb_op != 100u || pb_ln < 4u || pb_t[i + 3] != 1u || pos != 0u) return PSB_R_SIG;
                if (((d >> 4) & 4u) != 0u) return PSB_R_POSZ;
                pos = i + 1;
            }
        } else if (pb_op == 91u) {
            if (ty == 2u) return PSV_R_RELOUT;
            if (ty == 1u) return PSB_R_SIG;   // An input range: the position's register may lie in it.
        } else if (pb_op >= 101u && pb_op <= 103u) {
            if (ty == 12u || ty == 15u || ty == 38u || ty == 39u || ty == 41u) {
                return PSV_R_DEPTHOUT;
            }
            if (ty != 2u || pb_op != 101u || pb_ln != 3u || o0 != 0u) return PSB_R_SIG;
            if (pb_t[i + 2] != 0u || (d & 3u) != 2u || ((d >> 2) & 3u) != 0u ||
                (d >> 31) != 0u) return PSB_R_SIG;
            if (((d >> 4) & 8u) == 0u) return PSV_R_NOALPHA;
            o0 = i + 1;
        }
        dl.push_back(PsbDcl{ i, pb_ln, pb_op });
        i += pb_ln;
    }
    if (o0 == 0u) return PSV_R_NOALPHA;
    // The free slots: the highest of each (cb13, t127 where the engine leaves them).
    uint32_t k = 14u, ts0 = 128u;
    for (uint32_t s = 14u; s-- > 0u;) {
        if (!cbu[s]) { k = s; break; }
    }
    for (uint32_t s = 128u; s-- > 0u;) {
        if (!tu[s]) { ts0 = s; break; }
    }
    uint32_t tz0 = 128u;   // PSB_ZB: Zb's, the next free below.
    for (uint32_t s = ts0 < 128u ? ts0 : 0u; s-- > 0u;) {
        if (!tu[s]) { tz0 = s; break; }
    }
    if (k >= 14u || ts0 >= 128u || tz0 >= 128u) return PSB_R_SLOT;
    const uint32_t nt = tmp ? pb_t[tmp] : 0u;
    if (nt > 4096u - PSB_TEMPS) return PSV_R_TOKENS;
    const uint32_t ra = nt, rb = nt + 1u, rc = nt + 2u, rd = nt + 3u;
    // The body: o0.w redirected into ra.w, o0.xyz and every other colour write dropped (the alpha variant's rule,
    // a dropped destination of a two-destination op nulled), the epilogue inlined before every ret (no SM5
    // subroutine: fxc emits none without [call], so drivers rarely meet them) and a retc made "if c - epilogue - ret
    // - endif" (the same test, the same c). The epilogue writes the verdicts the prologue made (below):
    //   and o0.xyzw, ra.wwww, rc.yzwx / mov o1.x, rb.xxxx (the phase's (T, 25 % band, 100 % band, Tf) and its NW).
    const uint32_t ep[12] = {
        1u | (7u << 24), psb_op(2u, 0u, 0xFu, 1u), 0u, psb_op(0u, 1u, 0xFFu, 1u), ra,
        psb_op(0u, 1u, 0x39u, 1u), rc,
        54u | (5u << 24), psb_op(2u, 0u, 0x1u, 1u), 1u, psb_op(0u, 1u, 0x00u, 1u), rb };
    uint32_t nret = 0u;
    std::vector<uint32_t> x;
    x.reserve(pb_len - i + 64u);
    uint32_t last = 0u;   // The last instruction's opcode (the program must end with ret).
    while (i < pb_len) {
        const uint32_t tk = pb_t[i];
        const uint32_t pb_op = tk & 0x7FFu;
        size_t pb_ln = 0;
        if (pb_op == 53u) {
            if (i + 1u >= pb_len) return PSV_R_TOKENS;
            pb_ln = pb_t[i + 1];
            if (pb_ln < 2u) return PSV_R_TOKENS;
        } else {
            pb_ln = (tk >> 24) & 0x7Fu;
        }
        if (pb_ln == 0u || pb_ln > pb_len - i) return PSV_R_TOKENS;
        const bool dcl = pb_op == 53u || (pb_op >= 88u && pb_op <= 106u) ||
                         (pb_op >= 143u && pb_op <= 162u) || (pb_op >= 113u && pb_op <= 116u) ||
                         pb_op == 206u;
        if (dcl) return PSV_R_TOKENS;   // A declaration past the first instruction.
        if (pb_op == 120u) return PSV_R_LINKAGE;
        if (pb_op == 4u || pb_op == 5u || pb_op == 44u) return PSB_R_SUB;
        if (pb_op >= 219u && pb_op <= 233u) return PSV_R_FEEDBACK;
        if (pb_op == 110u || pb_op == 111u || (pb_op >= 203u && pb_op <= 205u)) return PSB_R_RATE;
        last = pb_op;
        size_t j = i + 1;
        if ((tk >> 31) != 0u) {
            bool e = true;
            while (e) {
                if (j >= i + pb_ln) return PSV_R_TOKENS;
                e = (pb_t[j] >> 31) != 0u;
                ++j;
            }
        }
        // PSB_LORES: the engine's own reads of the position register take rd (SV_Position at the frame's
        // resolution: the prologue's end), in the token copy, before the rules below read the instruction.
        if (pos != 0u) {
            for (size_t f = j; f < i + pb_ln;) {
                const size_t fl = psv_oplen(pb_t.data(), f, i + pb_ln);
                if (fl == 0u) return PSV_R_TOKENS;
                if (!psb_pos_fix(pb_t.data(), f, f + fl, pr, rd)) return PSB_R_POSX;
                f += fl;
            }
        }
        if (pb_op == 62u || pb_op == 63u) {
            if (++nret > PSB_RET_MAX) return PSB_R_RETS;
        }
        if (pb_op == 62u) {   // ret: the epilogue first.
            if (pb_ln != 1u) return PSV_R_TOKENS;
            x.insert(x.end(), ep, ep + 12);
            x.push_back(tk);
            i += pb_ln;
            continue;
        }
        if (pb_op == 63u) {   // retc_z / retc_nz c: if_z / if_nz c, the epilogue, ret, endif.
            if (j != i + 1u) return PSV_R_TOKENS;
            const size_t cl = psv_oplen(pb_t.data(), j, i + pb_ln);
            if (cl == 0u || j + cl != i + pb_ln) return PSV_R_TOKENS;
            x.push_back(31u | (tk & (1u << 18)) | (static_cast<uint32_t>(1u + cl) << 24));
            x.insert(x.end(), pb_t.begin() + static_cast<std::ptrdiff_t>(j),
                     pb_t.begin() + static_cast<std::ptrdiff_t>(j + cl));
            x.insert(x.end(), ep, ep + 12);
            x.push_back(62u | (1u << 24));
            x.push_back(21u | (1u << 24));
            i += pb_ln;
            continue;
        }
        const bool two = pb_op == 77u || pb_op == 78u || pb_op == 81u || pb_op == 38u ||
                         pb_op == 132u || pb_op == 133u || pb_op == 142u;
        size_t dp[2] = { 0, 0 };
        size_t dln[2] = { 0, 0 };
        uint32_t dty[2] = { 0xFFu, 0xFFu };
        uint32_t reg[2] = { 0u, 0u };
        uint32_t msk[2] = { 0u, 0u };
        size_t rx[2] = { 0, 0 };   // The register index's position.
        bool any = false;
        int kk = 0;
        while (j < i + pb_ln) {
            const size_t ol = psv_oplen(pb_t.data(), j, i + pb_ln);
            if (ol == 0u) return PSV_R_TOKENS;
            const uint32_t q = pb_t[j];
            const uint32_t qt = (q >> 12) & 0xFFu;
            if (kk < 2) {
                dp[kk] = j;
                dln[kk] = ol;
                dty[kk] = qt;
            }
            if (qt == 2u) {
                if (kk > 1 || (kk == 1 && !two)) return PSV_R_OUTREAD;
                const size_t x0 = psv_reg0(pb_t.data(), j, j + ol);
                if (x0 == 0u) return PSV_R_RELOUT;
                if ((q & 3u) != 2u || ((q >> 2) & 3u) != 0u) return PSV_R_TOKENS;
                reg[kk] = pb_t[x0];
                msk[kk] = (q >> 4) & 0xFu;
                rx[kk] = x0;
                any = true;
            }
            j += ol;
            ++kk;
        }
        if (j != i + pb_ln) return PSV_R_TOKENS;
        if (!any) {
            x.insert(x.end(), pb_t.begin() + static_cast<std::ptrdiff_t>(i),
                     pb_t.begin() + static_cast<std::ptrdiff_t>(i + pb_ln));
            i += pb_ln;
            continue;
        }
        const int nd = two ? 2 : 1;
        bool keep[2] = { false, false };
        bool live = false;
        for (int d = 0; d < nd; ++d) {
            if (dty[d] == 2u) {
                keep[d] = reg[d] == 0u && (msk[d] & 8u) != 0u;
                if (keep[d]) live = true;
            } else if (dty[d] != 13u) {
                live = true;
            }
        }
        if (live) {
            const size_t pb_at = x.size();
            x.push_back(0u);
            for (size_t y = i + 1; y < i + pb_ln;) {
                int d = -1;
                if (y == dp[0] && dty[0] == 2u) d = 0;
                if (two && y == dp[1] && dty[1] == 2u) d = 1;
                if (d < 0) {
                    x.push_back(pb_t[y]);
                    ++y;
                } else if (keep[d]) {
                    // o0.w -> ra.w: the operand's type and mask, then its extended tokens, then the register.
                    x.push_back((pb_t[y] & ~(0xFFu << 12) & ~(0xFu << 4)) | (8u << 4));
                    for (size_t z = y + 1; z < rx[d]; ++z) {
                        x.push_back(pb_t[z]);
                    }
                    x.push_back(ra);
                    if (rx[d] + 1u != y + dln[d]) return PSV_R_TOKENS;
                    y += dln[d];
                } else {
                    x.push_back(13u << 12);   // null.
                    y += dln[d];
                }
            }
            const size_t nl = x.size() - pb_at;
            x[pb_at] = (tk & ~(0x7Fu << 24)) | (static_cast<uint32_t>(nl) << 24);
        }
        i += pb_ln;
    }
    if (i != pb_len || last != 62u) return PSV_R_PROGRAM;
    // The output signature: one float SV_Target0 (register 0) with .w.
    if (oi == cn) return PSB_R_SIG;
    {
        const size_t oo = psv_get32(b + 32 + 4 * oi) + 8u;
        const size_t on = psv_get32(b + oo - 4u);
        if (on < 32u || psv_get32(b + oo) != 1u || psv_get32(b + oo + 4) != 8u) {
            return PSB_R_SIG;
        }
        const size_t nm = psv_get32(b + oo + 8);
        if (psv_get32(b + oo + 12) != 0u || psv_get32(b + oo + 16) != 0u ||
            psv_get32(b + oo + 20) != 3u || psv_get32(b + oo + 24) != 0u ||
            nm + 10u > on) return PSB_R_SIG;
        if ((b[oo + 28] & 8u) == 0u) return PSV_R_NOALPHA;
        static const char sv[10] = { 'S', 'V', '_', 'T', 'A', 'R', 'G', 'E', 'T', 0 };
        for (size_t c = 0; c < 10u; ++c) {
            const char ch = static_cast<char>(b[oo + nm + c]);
            const char up = ch >= 'a' && ch <= 'z' ? static_cast<char>(ch - 32) : ch;
            if (up != sv[c]) return PSB_R_SIG;
        }
    }
    cbs = k;
    ts = ts0;
    tzs = tz0;   // PSB_ZB.
    // Our declarations, each where fxc puts its kind (constant buffers, resources, inputs, outputs, temps).
    const uint32_t dcb[4] = { 89u | (4u << 24), psb_op(8u, 1u, 0xE4u, 2u), k, 4u };   // PSB_LORES: 4.
    const uint32_t dt[4] = { 88u | (4u << 11) | (4u << 24), (7u << 12) | (1u << 20), ts0, 0x5555u };
    // PSB_ZB: dcl_resource_texture2d (float,float,float,float) tZ - Zb.
    const uint32_t dz[4] = { 88u | (3u << 11) | (4u << 24), (7u << 12) | (1u << 20), tz0, 0x5555u };
    // The position's x, y and z (the prologue reads all three: the pixel's address and its depth).
    const uint32_t dp0[4] = { 100u | (4u << 11) | (4u << 24), psb_op(1u, 0u, 7u, 1u), pr, 1u };
    const uint32_t pb_do[3] = { 101u | (3u << 24), psb_op(2u, 0u, 0x1u, 1u), 1u };   // PSB_PHASE: o1.x.
    const uint32_t dtm[2] = { 104u | (2u << 24), nt + PSB_TEMPS };
    std::vector<uint32_t> p;   // The new program.
    p.reserve(pb_len + 160u);
    p.push_back(pb_t[0]);
    p.push_back(0u);
    bool w[5] = { false, false, pos != 0u, false, tmp != 0u };   // cb, t, position, outputs, temps.
    auto rank = [](uint32_t rk_op) -> int {
        if (rk_op == 106u) return 0;
        if (rk_op == 53u) return 1;
        if (rk_op == 89u) return 2;
        if (rk_op == 90u) return 3;
        if (rk_op == 88u || rk_op == 161u || rk_op == 162u) return 4;
        if (rk_op >= 95u && rk_op <= 100u) return 5;
        if (rk_op >= 101u && rk_op <= 103u) return 6;
        if (rk_op == 104u) return 7;
        return 8;
    };
    auto put = [&](int r) {   // Ours whose kind ranks before r, not yet written.
        if (!w[0] && r > 2) { p.insert(p.end(), dcb, dcb + 4); w[0] = true; }
        if (!w[1] && r > 4) {   // PSB_ZB: Zb's first (the lower slot), then the depth's.
            p.insert(p.end(), dz, dz + 4);
            p.insert(p.end(), dt, dt + 4);
            w[1] = true;
        }
        if (!w[2] && r >= 5) { p.insert(p.end(), dp0, dp0 + 4); w[2] = true; }
        if (!w[3] && r > 6) { p.insert(p.end(), pb_do, pb_do + 3); w[3] = true; }
        if (!w[4] && r > 7) { p.insert(p.end(), dtm, dtm + 2); w[4] = true; }
    };
    for (const PsbDcl& e : dl) {
        const int rk = rank(e.op);
        put(rk);
        const size_t pb_at = p.size();
        p.insert(p.end(), pb_t.begin() + static_cast<std::ptrdiff_t>(e.at),
                 pb_t.begin() + static_cast<std::ptrdiff_t>(e.at + e.ln));
        if (e.at + 1u == pos) p[pb_at + 1] |= 7u << 4;                    // + .xyz
        if (e.at + 1u == o0) p[pb_at + 1] |= 0xFu << 4;                   // o0.xyzw
        if (e.at + 1u == tmp) p[pb_at + 1] = nt + PSB_TEMPS;
        if (e.op >= 101u && e.op <= 103u && !w[3]) {   // Ours right after o0.
            p.insert(p.end(), pb_do, pb_do + 3);
            w[3] = true;
        }
    }
    put(9);
    // The prologue. Registers: ra.w the alpha; rb = (z_1, z_2, z_3, z); rc = (r, k, scratch, the loop's bound), then
    // the verdicts (pF, pT, p1, p2); rd the read's base (xy the block's corner - the address is ra.xyww -, z k0) and
    // scratch, after the early-out the engine's SV_Position substitute. cb[0] = (m_1, m_2, m_3, Dq), cb[1] = (a_1, a_2,
    // a_3, D raw), cb[2] = (test-off mask, Tf mask, bands mask, reads: s s), cb[3] = (log2 s, s - 1, sample mask, Zb
    // flag) - psb_post uploads the same layout (TWIN).
    // r = the largest sample 0 of the block's s x s pixels (samples above 0 unread: an edge inside the block counts
    // the farther side for the whole block). The loop stops once r reaches every bound it is needed for: each tested
    // push, and Dq where z > D (pF is false at z <= D whatever r is); the test-off kind nearer than D reads nothing.
    // The verdicts stay live through the engine's code. pF = z > D (the Tf pass's GREATER) and r >= Dq (the float
    // just above D - PSB_TF_EPS where Tf is drawn, 0 where not; PSB_TF_EPS's note). Early-out: a fragment no
    // verdict passes with z_1 >= 1 (its NW MIN against N <= 1 is a no-op; the cloud deck's [1, 1] viewport over
    // nearer geometry) writes nothing and is discarded before the engine's code; every other fragment runs on (NW is
    // read behind the live depth too, so a hidden fragment's NW counts). PSB_ZB: under cb3.w only the terms
    // t <= Zb (the block's largest S, tZ at the low-res pixel) stay in the bound and the reads start at Zb's pixel
    // k0: the main depth only comes nearer within a cycle (the live maximum M <= Zb; psb_zb_trip clears the flag
    // wherever that could fail), so a term past Zb is false whatever the loop reads, and a kept term ends the loop
    // (r >= t) only where M >= t, or the loop runs out with r = M - each verdict is the full maximum's, as without.
    // No temp beyond ra.xy and rd.zw.
    {
        const uint32_t kt = 0u, ki = 1u, kl = 4u, kr = 7u, kc = 8u;
        const uint32_t cbx = psb_op(kc, 1u, 0x24u, 2u);   // cb[k].xyzx
        auto c1 = [&](uint32_t e, uint32_t c) {   // cb[k][e].c (one component)
            const uint32_t c1_t[3] = { psb_op(kc, 2u, c, 2u), k, e };
            p.insert(p.end(), c1_t, c1_t + 3);
        };
        auto r = [&](uint32_t kind, uint32_t sel, uint32_t reg) {
            p.push_back(psb_op(kt, kind, sel, 1u));
            p.push_back(reg);
        };
        auto ins = [&](uint32_t i_op, uint32_t i_len) { p.push_back(i_op | (i_len << 24)); };
        auto cw = [&](uint32_t e) {   // cb[k][e].wwww
            const uint32_t cw_t[3] = { psb_op(kc, 1u, 0xFFu, 2u), k, e };
            p.insert(p.end(), cw_t, cw_t + 3);
        };
        const uint32_t sx = 0x00u, sy = 0x55u, sz = 0xAAu, sw = 0xFFu;
        ins(54u, 5u);                                                    // mov ra.w, l(0)
        r(0u, 0x8u, ra);
        p.push_back(1u | (kl << 12));
        p.push_back(0u);
        ins(50u, 11u);                                                   // mad rb.xyz, v.zzzz, cb0.xyzx, cb1.xyzx
        r(0u, 0x7u, rb);
        p.push_back(psb_op(ki, 1u, sz, 1u));
        p.push_back(pr);
        p.push_back(cbx);
        p.push_back(k);
        p.push_back(0u);
        p.push_back(cbx);
        p.push_back(k);
        p.push_back(1u);
        ins(54u, 5u);                                                    // mov rb.w, v.zzzz
        r(0u, 0x8u, rb);
        p.push_back(psb_op(ki, 1u, sz, 1u));
        p.push_back(pr);
        ins(52u, 7u);                                                    // max rc.w, rb.xxxx, rb.yyyy
        r(0u, 0x8u, rc);
        r(1u, sx, rb);
        r(1u, sy, rb);
        ins(52u, 7u);                                                    // max rc.w, rc.wwww, rb.zzzz
        r(0u, 0x8u, rc);
        r(1u, sw, rc);
        r(1u, sz, rb);
        ins(1u, 8u);                                                     // and rc.w, rc.wwww, cb2.zzzz
        r(0u, 0x8u, rc);
        r(1u, sw, rc);
        p.push_back(psb_op(kc, 1u, sz, 2u));
        p.push_back(k);
        p.push_back(2u);
        // Dq joins the bound only where the fragment's own far test passes (pF needs D < z; elsewhere the loop keeps
        // its band breaks, and the test-off kind reads no sample).
        ins(49u, 8u);                                                    // lt rc.z, cb1.w, rb.w (D < z)
        r(0u, 0x4u, rc);
        c1(1u, 3u);
        r(2u, 3u, rb);
        ins(1u, 8u);                                                     // and rc.z, rc.z, cb0.w (Dq or +0)
        r(0u, 0x4u, rc);
        r(2u, 2u, rc);
        c1(0u, 3u);
        ins(52u, 7u);                                                    // max rc.w, rc.wwww, rc.zzzz
        r(0u, 0x8u, rc);
        r(1u, sw, rc);
        r(1u, sz, rc);
        // PSB_ZB: under cb3.w (Zb stands), the bound keeps only the terms Zb - the block's largest S, a bound on
        // the live maximum - can reach (each of z_1..3 masked by the bands, and the Dq term in rc.z), the rest decided
        // false without a read; rd.z = k0, Zb's argmax pixel, the loop's first read (0 under no flag). ra.xy and
        // rd.zw are free here (ra.w stays the address's 0).
        ins(54u, 5u);                                                    // mov rd.z, l(0)
        r(0u, 0x4u, rd);
        p.push_back(1u | (kl << 12));
        p.push_back(0u);
        ins(31u | (1u << 18), 4u);                                       // if_nz cb3.w
        c1(3u, 3u);
        ins(28u, 5u);                                                    //   ftou ra.xy, v.xyxx
        r(0u, 0x3u, ra);
        p.push_back(psb_op(ki, 1u, 0x04u, 1u));
        p.push_back(pr);
        // ld_indexable(texture2d)(float,float,float,float) ra.xy, ra.xyww, tZ.xyxx (Zb's texel: max S, k0)
        p.push_back(45u | (9u << 24) | (1u << 31));
        p.push_back(0x800000C2u);
        p.push_back(0x00155543u);
        r(0u, 0x3u, ra);
        r(1u, 0xF4u, ra);
        p.push_back(psb_op(kr, 1u, 0x04u, 1u));
        p.push_back(tz0);
        ins(29u, 7u);                                                    //   ge rd.zw, ra.xxxx, rb.xxxy
        r(0u, 0xCu, rd);
        r(1u, sx, ra);
        r(1u, 0x40u, rb);
        ins(1u, 7u);                                                     //   and rd.zw, rd.zzzw, rb.xxxy
        r(0u, 0xCu, rd);
        r(1u, 0xEAu, rd);
        r(1u, 0x40u, rb);
        ins(52u, 7u);                                                    //   max rd.z, rd.z, rd.w
        r(0u, 0x4u, rd);
        r(2u, 2u, rd);
        r(2u, 3u, rd);
        ins(29u, 7u);                                                    //   ge rd.w, ra.x, rb.z
        r(0u, 0x8u, rd);
        r(2u, 0u, ra);
        r(2u, 2u, rb);
        ins(1u, 7u);                                                     //   and rd.w, rd.w, rb.z
        r(0u, 0x8u, rd);
        r(2u, 3u, rd);
        r(2u, 2u, rb);
        ins(52u, 7u);                                                    //   max rd.z, rd.z, rd.w
        r(0u, 0x4u, rd);
        r(2u, 2u, rd);
        r(2u, 3u, rd);
        ins(1u, 8u);                                                     //   and rd.z, rd.z, cb2.z (bands)
        r(0u, 0x4u, rd);
        r(2u, 2u, rd);
        c1(2u, 2u);
        ins(29u, 7u);                                                    //   ge rd.w, ra.x, rc.z (Dq term)
        r(0u, 0x8u, rd);
        r(2u, 0u, ra);
        r(2u, 2u, rc);
        ins(1u, 7u);                                                     //   and rd.w, rd.w, rc.z
        r(0u, 0x8u, rd);
        r(2u, 3u, rd);
        r(2u, 2u, rc);
        ins(52u, 7u);                                                    //   max rc.w, rd.z, rd.w (the bound)
        r(0u, 0x8u, rc);
        r(2u, 2u, rd);
        r(2u, 3u, rd);
        ins(28u, 5u);                                                    //   ftou rd.z, ra.y (k0)
        r(0u, 0x4u, rd);
        r(2u, 1u, ra);
        ins(21u, 1u);                                                    // endif
        ins(28u, 5u);                                                    // ftou rd.xy, v.xyxx
        r(0u, 0x3u, rd);
        p.push_back(psb_op(ki, 1u, 0x04u, 1u));
        p.push_back(pr);
        // PSB_LORES: the block's corner at the frame's resolution (cb3.x = log2 s; s = 1: the pixel itself); the
        // address is ra.xyww (ra.w is 0 until the engine's code).
        ins(41u, 8u);                                                    // ishl rd.xy, rd.xyxx, cb3.xxxx
        r(0u, 0x3u, rd);
        r(1u, 0x04u, rd);
        p.push_back(psb_op(kc, 1u, sx, 2u));
        p.push_back(k);
        p.push_back(3u);
        ins(54u, 8u);                                                    // mov rc.xy, l(0, 0, 0, 0)
        r(0u, 0x3u, rc);
        p.push_back(2u | (kl << 12));
        for (int q = 0; q < 4; ++q) p.push_back(0u);
        ins(48u, 1u);                                                    // loop
        ins(29u, 7u);                                                    // ge rc.z, rc.xxxx, rc.wwww
        r(0u, 0x4u, rc);
        r(1u, sx, rc);
        r(1u, sw, rc);
        p.push_back(3u | (1u << 18) | (3u << 24));                       // breakc_nz rc.z
        r(2u, 2u, rc);
        ins(80u, 8u);                                                    // uge rc.z, rc.yyyy, cb2.w
        r(0u, 0x4u, rc);
        r(1u, sy, rc);
        c1(2u, 3u);
        p.push_back(3u | (1u << 18) | (3u << 24));                       // breakc_nz rc.z
        r(2u, 2u, rc);
        // PSB_LORES: the k-th read's pixel and sample - cb3 = (log2 s, s - 1, the sample mask, Zb's flag): pixel
        // corner + (k & (s - 1), (k >> log2 s) & (s - 1)), sample k & mask - s = 1: the pixel, sample k; s > 1: the
        // block's s x s pixels, sample 0 each (cb2.w = s s reads at most). PSB_ZB: k + k0 in place of k (ra.z) - the
        // column / row masks take it mod s s, so k = 0 .. s s - 1 still reads every pixel, Zb's argmax first.
        ins(30u, 7u);                                                    // iadd ra.z, rc.y, rd.z
        r(0u, 0x4u, ra);
        r(2u, 1u, rc);
        r(2u, 2u, rd);
        ins(1u, 8u);                                                     // and ra.x, ra.z, cb3.y
        r(0u, 0x1u, ra);
        r(2u, 2u, ra);
        c1(3u, 1u);
        ins(85u, 8u);                                                    // ushr ra.y, ra.z, cb3.x
        r(0u, 0x2u, ra);
        r(2u, 2u, ra);
        c1(3u, 0u);
        ins(1u, 8u);                                                     // and ra.y, ra.y, cb3.y
        r(0u, 0x2u, ra);
        r(2u, 1u, ra);
        c1(3u, 1u);
        ins(30u, 7u);                                                    // iadd ra.xy, ra.xyxx, rd.xyxx
        r(0u, 0x3u, ra);
        r(1u, 0x04u, ra);
        r(1u, 0x04u, rd);
        ins(1u, 8u);                                                     // and ra.z, ra.z, cb3.z
        r(0u, 0x4u, ra);
        r(2u, 2u, ra);
        c1(3u, 2u);
        // ldms_indexable(texture2dms)(float,float,float,float) rc.z, ra.xyww, t.xxxx, ra.z (PSB_LORES)
        p.push_back(46u | (11u << 24) | (1u << 31));
        p.push_back(0x80000102u);
        p.push_back(0x00155543u);
        r(0u, 0x4u, rc);
        r(1u, 0xF4u, ra);
        p.push_back(psb_op(kr, 1u, sx, 1u));
        p.push_back(ts0);
        r(2u, 2u, ra);
        ins(52u, 7u);                                                    // max rc.x, rc.xxxx, rc.zzzz
        r(0u, 0x1u, rc);
        r(1u, sx, rc);
        r(1u, sz, rc);
        ins(30u, 7u);                                                    // iadd rc.y, rc.yyyy, l(1)
        r(0u, 0x2u, rc);
        r(1u, sy, rc);
        p.push_back(1u | (kl << 12));
        p.push_back(1u);
        ins(22u, 1u);                                                    // endloop
        ins(29u, 8u);                                                    // ge rd.z, rc.xxxx, cb0.wwww (r >= Dq)
        r(0u, 0x4u, rd);
        r(1u, sx, rc);
        cw(0u);
        ins(49u, 8u);                                                    // lt rd.w, cb1.w, rb.w (D < z)
        r(0u, 0x8u, rd);
        c1(1u, 3u);
        r(2u, 3u, rb);
        ins(29u, 7u);                                                    // ge rc.yzw, rc.xxxx, rb.xxyz
        r(0u, 0xEu, rc);
        r(1u, sx, rc);
        r(1u, 0x90u, rb);
        ins(60u, 8u);                                                    // or rc.yzw, rc.yyzw, cb2.xxxx
        r(0u, 0xEu, rc);
        r(1u, 0xE5u, rc);
        p.push_back(psb_op(kc, 1u, sx, 2u));
        p.push_back(k);
        p.push_back(2u);
        ins(1u, 7u);                                                     // and rd.z, rd.z, rd.w
        r(0u, 0x4u, rd);
        r(2u, 2u, rd);
        r(2u, 3u, rd);
        ins(1u, 8u);                                                     // and rc.x, rd.z, cb2.y (pF)
        r(0u, 0x1u, rc);
        r(2u, 2u, rd);
        c1(2u, 1u);
        ins(60u, 7u);                                                    // or rd.w, rc.y, rc.z
        r(0u, 0x8u, rd);
        r(2u, 1u, rc);
        r(2u, 2u, rc);
        ins(60u, 7u);                                                    // or rd.w, rd.w, rc.w
        r(0u, 0x8u, rd);
        r(2u, 3u, rd);
        r(2u, 3u, rc);
        ins(60u, 7u);                                                    // or rd.w, rd.w, rc.x
        r(0u, 0x8u, rd);
        r(2u, 3u, rd);
        r(2u, 0u, rc);
        ins(29u, 7u);                                                    // ge rd.z, rb.x, l(1.0) (z_1 >= 1)
        r(0u, 0x4u, rd);
        r(2u, 0u, rb);
        p.push_back(1u | (kl << 12));
        p.push_back(0x3F800000u);
        ins(59u, 5u);                                                    // not rd.w, rd.w
        r(0u, 0x8u, rd);
        r(2u, 3u, rd);
        ins(1u, 7u);                                                     // and rd.z, rd.z, rd.w
        r(0u, 0x4u, rd);
        r(2u, 2u, rd);
        r(2u, 3u, rd);
        p.push_back(13u | (1u << 18) | (3u << 24));                      // discard_nz rd.z
        r(2u, 2u, rd);
        // PSB_LORES: where the engine's code reads SV_Position (its body reads rd in its place - above), rd =
        // (x, y, -, w) of a real pixel of the frame: s = 1 the fragment's own (v.xy); s > 1 the centre of the block's
        // pixel (ox, oy), ox = ((X ^ (Y >> 1)) & 1 | (X & 2)) & (s - 1), oy = ((X ^ Y ^ (Y >> 1)) & 1 | ((Y >> 1) & 2))
        // & (s - 1) (X, Y the block), so a screen-space read (a soft particle's depth, a lighting buffer) lands on a
        // texel centre and any 2 x 2 blocks hold one pixel of each quarter of the engine's 8 x 8 ordered dither, as
        // 2 x 2 pixels do at s = 1. A uniform branch: s = 1 runs the one mov (else).
        if (pos != 0u) {
            // Scratch: rb.yzw (free past the verdicts - the epilogue reads rb.x), ra.xyz (past the loop).
            auto i1 = [&](uint32_t v) {   // l(v), one component
                p.push_back(1u | (kl << 12));
                p.push_back(v);
            };
            ins(31u, 4u);                                                // if_z cb3.z (s > 1; rd.xy the corner)
            c1(3u, 2u);
            ins(85u, 8u);                                                // ushr ra.xy, rd.xyxx, cb3.xxxx
            r(0u, 0x3u, ra);
            r(1u, 0x04u, rd);
            p.push_back(psb_op(kc, 1u, sx, 2u));
            p.push_back(k);
            p.push_back(3u);
            ins(85u, 7u);                                                // ushr ra.z, ra.y, l(1)
            r(0u, 0x4u, ra);
            r(2u, 1u, ra);
            i1(1u);
            ins(87u, 7u);                                                // xor rb.y, ra.x, ra.z
            r(0u, 0x2u, rb);
            r(2u, 0u, ra);
            r(2u, 2u, ra);
            ins(1u, 7u);                                                 // and rb.y, rb.y, l(1)
            r(0u, 0x2u, rb);
            r(2u, 1u, rb);
            i1(1u);
            ins(1u, 7u);                                                 // and rb.z, ra.x, l(2)
            r(0u, 0x4u, rb);
            r(2u, 0u, ra);
            i1(2u);
            ins(60u, 7u);                                                // or rb.y, rb.y, rb.z (ox)
            r(0u, 0x2u, rb);
            r(2u, 1u, rb);
            r(2u, 2u, rb);
            ins(87u, 7u);                                                // xor rb.z, ra.x, ra.y
            r(0u, 0x4u, rb);
            r(2u, 0u, ra);
            r(2u, 1u, ra);
            ins(87u, 7u);                                                // xor rb.z, rb.z, ra.z
            r(0u, 0x4u, rb);
            r(2u, 2u, rb);
            r(2u, 2u, ra);
            ins(1u, 7u);                                                 // and rb.z, rb.z, l(1)
            r(0u, 0x4u, rb);
            r(2u, 2u, rb);
            i1(1u);
            ins(1u, 7u);                                                 // and rb.w, ra.z, l(2)
            r(0u, 0x8u, rb);
            r(2u, 2u, ra);
            i1(2u);
            ins(60u, 7u);                                                // or rb.z, rb.z, rb.w (oy)
            r(0u, 0x4u, rb);
            r(2u, 2u, rb);
            r(2u, 3u, rb);
            ins(1u, 8u);                                                 // and rb.yz, rb.xyzy, cb3.yyyy
            r(0u, 0x6u, rb);
            r(1u, 0x64u, rb);
            p.push_back(psb_op(kc, 1u, sy, 2u));
            p.push_back(k);
            p.push_back(3u);
            ins(30u, 7u);                                                // iadd ra.xy, rd.xyxx, rb.yzyy
            r(0u, 0x3u, ra);
            r(1u, 0x04u, rd);
            r(1u, 0x59u, rb);
            ins(86u, 5u);                                                // utof ra.xy, ra.xyxx
            r(0u, 0x3u, ra);
            r(1u, 0x04u, ra);
            ins(0u, 10u);                                                // add rd.xy, ra.xyxx, l(0.5, 0.5, 0, 0)
            r(0u, 0x3u, rd);
            r(1u, 0x04u, ra);
            p.push_back(2u | (kl << 12));
            p.push_back(0x3F000000u);
            p.push_back(0x3F000000u);
            p.push_back(0u);
            p.push_back(0u);
            ins(18u, 1u);                                                // else (s = 1)
            ins(54u, 5u);                                                //   mov rd.xy, v.xyxx
            r(0u, 0x3u, rd);
            p.push_back(psb_op(ki, 1u, 0x04u, 1u));
            p.push_back(pr);
            ins(21u, 1u);                                                // endif
            if (((pb_t[pos] >> 4) & 8u) != 0u) {
                ins(54u, 5u);                                            // mov rd.w, v.wwww
                r(0u, 0x8u, rd);
                p.push_back(psb_op(ki, 1u, sw, 1u));
                p.push_back(pr);
            }
        }
    }
    p.insert(p.end(), x.begin(), x.end());
    p[1] = static_cast<uint32_t>(p.size());
    // The signatures: SV_Position's used mask + .xyz; the output signature SV_Target0 - 1 (fxc's own encoding of a
    // float4 and a float target: 68 bytes, the name at 56, o1's masks 1 / 0xE).
    std::vector<uint8_t> isg(b + io, b + io + in);
    isg[pe + 21] = static_cast<uint8_t>(isg[pe + 21] | 7u);   // .xyz read.
    std::vector<uint8_t> osg(68u, 0u);
    psv_put32(osg.data(), 2u);
    psv_put32(osg.data() + 4, 8u);
    for (uint32_t e = 0; e < 2u; ++e) {
        uint8_t* const q = osg.data() + 8 + 24 * e;
        psv_put32(q, 56u);
        psv_put32(q + 4, e);
        psv_put32(q + 12, 3u);
        psv_put32(q + 16, e);
        q[20] = e == 1u ? 0x01u : 0x0Fu;
        q[21] = e == 1u ? 0x0Eu : 0x00u;
    }
    memcpy(osg.data() + 56, "SV_Target\0\xAB\xAB", 12);
    // The container again: the chunks in their order, the program and the signatures replaced.
    size_t total = 32u + 4u * cn;
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t co = psv_get32(b + 32 + 4 * c);
        total += 8u + (c == xi ? 4u * p.size() : c == ii ? isg.size()
                       : c == oi ? osg.size() : psv_get32(b + co + 4));
    }
    std::vector<uint8_t> nb(total, 0u);
    memcpy(nb.data(), b, 24);
    psv_put32(nb.data() + 24, static_cast<uint32_t>(total));
    psv_put32(nb.data() + 28, cn);
    size_t wp = 32u + 4u * cn;
    for (uint32_t c = 0; c < cn; ++c) {
        const size_t co = psv_get32(b + 32 + 4 * c);
        psv_put32(nb.data() + 32 + 4 * c, static_cast<uint32_t>(wp));
        memcpy(nb.data() + wp, b + co, 4);
        if (c == xi) {
            psv_put32(nb.data() + wp + 4, static_cast<uint32_t>(4u * p.size()));
            for (size_t q = 0; q < p.size(); ++q) {
                psv_put32(nb.data() + wp + 8 + 4 * q, p[q]);
            }
            wp += 8u + 4u * p.size();
        } else if (c == ii || c == oi) {
            const std::vector<uint8_t>& s = c == ii ? isg : osg;
            psv_put32(nb.data() + wp + 4, static_cast<uint32_t>(s.size()));
            memcpy(nb.data() + wp + 8, s.data(), s.size());
            wp += 8u + s.size();
        } else {
            const size_t cs = psv_get32(b + co + 4);
            memcpy(nb.data() + wp + 4, b + co + 4, 4u + cs);
            wp += 8u + cs;
        }
    }
    uint32_t sum[4];
    psv_checksum(nb.data(), nb.size(), sum);
    for (int q = 0; q < 4; ++q) psv_put32(nb.data() + 4 + 4 * q, sum[q]);
    out.swap(nb);
    return PSV_OK;
}
// PSV - the capture. psv_hook_ensure detours ID3D11Device::CreatePixelShader (vtable slot 15; ID3D11Device1 - 5's
// entry too where it is other code): the original runs, then a successful creation's bytecode is copied into the
// store keyed by the returned object. Skipped: our own creations (psv_ours; the tail's builtins register by
// psv_own), class-linkage creations, a non-DXBC blob or one over PSV_BLOB_MAX, a store at PSV_STORE_MAX
// (reported once). Each kept blob's object carries the blob's serial as private data (PSV_STAMP): a bound shader's
// serial names its bytes, so an object at a reused address that came another way never takes a dead one's blob. The
// store is process-scoped and the engine's blobs are never reset (its shaders outlive missions and device resets); a
// blob goes when its shader is destroyed (PSV_TOMB) or a creation lands at its address (our builtin taken by the
// detour and then by psv_own is that case: the overwrite releases the old sentinel); a builtin's also when its
// slot is made again and at Resources::release / the session's globals reset (psv_device_lost). Any thread:
// g_psv_mx guards the store and no D3D call is made under it (SetPrivateData after). Installed at the earliest device
// reach: the extension's load (main.cpp's pre_start calls psv_hook_ensure), else a mission edge
// (rendering_integration_reset), else the first Draw3D frame (flush_frame).
// A shader created before the install has no bytecode here: its draws keep today's passes.
static constexpr size_t PSV_BLOB_MAX = static_cast<size_t>(1u) << 20;    // 1 MB (the engine's particle PS: < 64 KB).
static constexpr size_t PSV_STORE_MAX = static_cast<size_t>(64u) << 20;  // 64 MB in all.
static const GUID PSV_STAMP = { 0x6E8A0F3Bu, 0x5C2D, 0x4B1E, { 0x9A, 0x47, 0x3D, 0x5F, 0x1C, 0x2B, 0x7E, 0x90 } };
struct PsvBlob {
    uint64_t ser = 0;   // Its serial (the stamp its shader carries).
    std::vector<uint8_t> b;
    int own = -1;   // One of our builtin mesh shaders (psv_own's slot, 0 - 5), else -1 (the engine's).
};
struct PsvBooks {   // Under g_psv_mx (the store's own books, kept since the hook went in).
    uint64_t bytes = 0;    // Bytes held (PSV_STORE_MAX's test).
};
static std::mutex g_psv_mx;
static std::unordered_map<const void*, PsvBlob> g_psv_store;   // By the created ID3D11PixelShader (an identity).
static PsvBooks g_psv_books;
static uint64_t g_psv_ser = 0;   // The last serial given (under g_psv_mx).
static std::atomic<bool> g_psv_cap_on{ false };   // The detour copies (cleared at process detach: a pass-through).
static thread_local int g_psv_self = 0;   // Our own variants' creation on this thread: not captured.
// PSV_TOMB - each held blob's shader carries a sentinel as private data (SetPrivateDataInterface); the runtime
// releases it when the shader is destroyed and its final Release drops the blob if the entry at that address still
// holds its serial. Any thread, possibly inside one of our hooks: g_psv_mx is a leaf lock (no other lock, D3D call or
// COM release under it by any holder), so the sentinel never waits on a lock its own thread holds. Gated on
// g_psv_cap_on (cleared at process detach, by g_psv_gate when this unit's statics end, and at a refused install): a
// sentinel released after that touches nothing, itself included. A refused allocation or attach leaves the blob held
// without one (dropped only by a creation at its address). HARD REQUIREMENT: this DLL is never unloaded at runtime -
// every held engine shader carries a vtable in this module and the store holds no reference to withdraw them by.
static const GUID PSV_TOMB = { 0x1A5A5FCBu, 0x8F7B, 0x4E1C, { 0x9C, 0x0F, 0x56, 0x84, 0xFE, 0xF6, 0xF7, 0xC2 } };
static struct PsvGate {   // Declared after the store, so it ends before it: the sentinels' gate closes first.
    ~PsvGate() { g_psv_cap_on.store(false, std::memory_order_relaxed); }
} g_psv_gate;
inline void psv_tomb_drop(const void* key, uint64_t ser) noexcept {
    try {
        std::lock_guard<std::mutex> g(g_psv_mx);
        const auto it = g_psv_store.find(key);
        if (it == g_psv_store.end() || it->second.ser != ser) return;
        g_psv_books.bytes -= it->second.b.size();
        g_psv_store.erase(it);
    } catch (...) {
        // The lock refused: the blob stays (dropped by a creation at its address).
    }
}
struct PsvTomb final : public IUnknown {
    PsvTomb(const void* k, uint64_t s) : key(k), ser(s) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        if (IsEqualIID(iid, __uuidof(IUnknown))) {
            *out = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return rc.fetch_add(1u, std::memory_order_relaxed) + 1u; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = rc.fetch_sub(1u, std::memory_order_acq_rel) - 1u;
        if (n == 0u && g_psv_cap_on.load(std::memory_order_relaxed)) {
            if (armed) psv_tomb_drop(key, ser);
            delete this;
        }
        return n;
    }
    std::atomic<ULONG> rc{ 1u };   // The creator's reference (the capture releases it once attached).
    const void* key;               // The shader (an identity) and its blob's serial.
    uint64_t ser;
    bool armed = true;             // False: never attached (released at once, nothing dropped).
};
// own: psv_own's slot (our builtin), else -1 (a creation the detour took for the engine's).
inline void psv_capture(const void* bc, SIZE_T n, ID3D11ClassLinkage* cl,
                        ID3D11PixelShader* ps, int own = -1) {
    if (!ps || g_psv_self > 0 || !g_psv_cap_on.load(std::memory_order_relaxed)) return;
    try {
        const bool fit = !cl && bc && n >= 32u && n <= PSV_BLOB_MAX &&
                         memcmp(bc, "DXBC", 4) == 0;
        std::vector<uint8_t> v;   // Copied before the lock.
        if (fit) {
            v.assign(static_cast<const uint8_t*>(bc), static_cast<const uint8_t*>(bc) + n);
        }
        uint64_t ser = 0;
        bool full = false;   // PSV_TOMB: refused by the cap (reported once, after the lock).
        {
            std::lock_guard<std::mutex> g(g_psv_mx);
            const auto it = g_psv_store.find(ps);
            if (it != g_psv_store.end()) {   // A dead shader's blob at this address.
                g_psv_books.bytes -= it->second.b.size();
                g_psv_store.erase(it);
            }
            if (own >= 0) {   // The slot's previous builtin (made again: a sample-count change) goes.
                for (auto o = g_psv_store.begin(); o != g_psv_store.end();) {
                    if (o->second.own != own) {
                        ++o;
                        continue;
                    }
                    g_psv_books.bytes -= o->second.b.size();
                    o = g_psv_store.erase(o);
                }
            }
            if (cl) {
                // PSV: class linkage - not held.
            } else if (!fit || g_psv_books.bytes + n > PSV_STORE_MAX) {
                full = fit;
            } else {
                PsvBlob& e = g_psv_store[ps];
                ser = ++g_psv_ser;
                e.ser = ser;
                e.own = own;
                e.b.swap(v);
                g_psv_books.bytes += n;
            }
        }
        if (ser != 0) {
            ps->SetPrivateData(PSV_STAMP, sizeof(ser), &ser);
            // PSV_TOMB: the sentinel (the runtime AddRefs it; ours is released here). The shader cannot die before
            // this: its creator holds it until the capture returns.
            PsvTomb* const t = new (std::nothrow) PsvTomb(ps, ser);
            if (t) {
                if (FAILED(ps->SetPrivateDataInterface(PSV_TOMB, t))) t->armed = false;
                t->Release();
            }
        }
        if (full) {   // PSV_TOMB: the creating thread, outside g_psv_mx (any thread; deduped).
            // EA_ERR_CLASS: a cap by design, not a fault - a warning (logged, never thrown).
            log_once_safe("KH RenderIntegration: world see-through: the particle shader store is full (64 MB of "
                          "live pixel shaders); particle shaders created while it stays full keep the earlier "
                          "see-through passes");
        }
    } catch (...) {
        // An allocation refused: the shader is not held (its draws keep today's passes).
    }
}
// The stamp of a bound shader (0: none - created before the hook, or not held).
inline uint64_t psv_stamp(ID3D11PixelShader* ps) {
    uint64_t v = 0;
    UINT n = sizeof(v);
    if (FAILED(ps->GetPrivateData(PSV_STAMP, &n, &v)) || n != sizeof(v)) return 0;
    return v;
}
using PsvCreate = HRESULT (STDMETHODCALLTYPE*)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*,
                                               ID3D11PixelShader**);
static constexpr int PSV_HOOK_N = 6;   // ID3D11Device - ID3D11Device5: at most six distinct entries.
static PsvCreate g_psv_orig[PSV_HOOK_N] = {};
// Our own creations are not captured (this renderer remakes its shaders every mission; their dead blobs would fill
// the store): a creation whose return address lies in this module's image (g_psv_base, set at the install) is ours.
// The tail's builtins register at their creation instead (psv_own: six slots, each holding its newest blob; all
// dropped at Resources::release, where they die - psv_device_lost). Only the outermost of nested hooked entries
// decides (g_psv_in: an entry whose original reaches another hooked one). A tool that hooks this entry after us
// makes our creations look like the engine's: those blobs are held and go at their shader's release (PSV_TOMB).
static const void* g_psv_base = nullptr;
static thread_local int g_psv_in = 0;
inline bool psv_ours(const void* ret) {
    MEMORY_BASIC_INFORMATION m = {};
    return g_psv_base && VirtualQuery(ret, &m, sizeof(m)) == sizeof(m) &&
           m.AllocationBase == g_psv_base;
}
template <int PsvI>
static HRESULT STDMETHODCALLTYPE psv_create(ID3D11Device* self, const void* bc, SIZE_T n,
                                            ID3D11ClassLinkage* cl, ID3D11PixelShader** ps) {
    const void* const ret = _ReturnAddress();
    const bool outer = g_psv_in == 0;
    ++g_psv_in;
    const HRESULT hr = g_psv_orig[PsvI](self, bc, n, cl, ps);
    --g_psv_in;
    if (outer && SUCCEEDED(hr) && ps && !psv_ours(ret)) {
        psv_capture(bc, n, cl, *ps);
    }
    return hr;
}
// A builtin mesh pixel shader the tail binds (ensure_resources / ensure_composite_shader: 0 PSMain, 1 its textured
// twin, 2 PSComposite, 3 its arb twin, 4 / 5 their textured twins), just made from bc: held as the engine's are.
// None with the system built out (WSEE_ON false: no store, no hook).
inline void psv_own(int slot, const void* bc, SIZE_T n, ID3D11PixelShader* ps) {
    if constexpr (WSEE_ON) psv_capture(bc, n, nullptr, ps, slot);
}
// The hook's state: 0 not yet / 1 installed / -1 refused (MinHook; reported once) - psv_pick reads it.
static std::atomic<int> g_psv_hook_state{ 0 };
// Game thread. Once a process (process-scoped, as the context hooks: no session reset, and never unhooked - process
// detach leaves the detour a pass-through into this module, g_psv_cap_on false, the context hooks' contract); a
// device not reachable yet leaves it for the next point. Never installed with the system built out (WSEE_ON
// false: no detour, no blob store; g_psv_hook_state stays 0).
inline void psv_hook_ensure() {
    if constexpr (WSEE_ON) {   // The body whole (no code left unreachable with it out: C4702).
        if (g_psv_hook_state.load(std::memory_order_relaxed) != 0 || !render_on()) return;
        if (!RVExtBridge::is_initialized()) return;   // flush_frame's rule before the device is asked for.
        ID3D11Device* const dev = RVExtBridge::get_d3d_device();
        if (!dev || !ensure_minhook()) return;
        // ID3D11Device1 - 5 (d3d11_1.h - d3d11_4.h's IIDs): one implementation normally serves them all with one table,
        // so their slot 15 is ID3D11Device's - an interface whose entry is other code is hooked too.
        static const GUID iid[5] = {
            { 0xA04BFB29u, 0x08EF, 0x43D6, { 0xA4, 0x9C, 0xA9, 0xBD, 0xBD, 0xCB, 0xE6, 0x86 } },
            { 0x9D06DFFAu, 0xD1E5, 0x4D07, { 0x83, 0xA8, 0x1B, 0xB1, 0x23, 0xF2, 0xF8, 0x41 } },
            { 0xA05C8C37u, 0xD2C6, 0x4732, { 0xB3, 0xA0, 0x9C, 0xE0, 0xB0, 0xDC, 0x9A, 0xE6 } },
            { 0x8992AB71u, 0x02E6, 0x4B8D, { 0xBA, 0x48, 0xB0, 0x56, 0xDC, 0xDA, 0x42, 0xC4 } },
            { 0x8FFDE202u, 0xA0E7, 0x45DF, { 0x9E, 0x01, 0xE8, 0x37, 0x80, 0x1B, 0x5E, 0xA0 } } };
        void* at[PSV_HOOK_N] = {};
        int n = 0;
        auto add = [&](void* obj) {
            void* const fn = (*reinterpret_cast<void***>(obj))[15];   // CreatePixelShader.
            for (int i = 0; i < n; ++i) {
                if (at[i] == fn) return;
            }
            if (n < PSV_HOOK_N) at[n++] = fn;
        };
        add(dev);
        for (const GUID& g : iid) {
            void* q = nullptr;
            if (SUCCEEDED(dev->QueryInterface(g, &q)) && q) {
                add(q);
                static_cast<IUnknown*>(q)->Release();
            }
        }
        void* const det[PSV_HOOK_N] = {
            reinterpret_cast<void*>(&psv_create<0>), reinterpret_cast<void*>(&psv_create<1>),
            reinterpret_cast<void*>(&psv_create<2>), reinterpret_cast<void*>(&psv_create<3>),
            reinterpret_cast<void*>(&psv_create<4>), reinterpret_cast<void*>(&psv_create<5>) };
        {
            MEMORY_BASIC_INFORMATION m = {};
            if (VirtualQuery(reinterpret_cast<const void*>(&psv_ours), &m, sizeof(m)) == sizeof(m)) {
                g_psv_base = m.AllocationBase;   // This module's image (psv_ours).
            }
        }
        g_psv_cap_on.store(true, std::memory_order_relaxed);
        int ok = 0;
        for (; ok < n; ++ok) {
            if (MH_CreateHook(at[ok], det[ok],
                              reinterpret_cast<void**>(&g_psv_orig[ok])) != MH_OK) break;
            if (MH_EnableHook(at[ok]) != MH_OK) {
                MH_RemoveHook(at[ok]);
                break;
            }
        }
        if (ok != n) {
            // PSV: the entries already enabled are disabled, never removed - a thread already inside
            // psv_create may still call its trampoline (g_psv_orig), which a removal would free; they stay created
            // for the process, g_psv_cap_on false (the one created but not enabled above is removed: no thread can
            // have entered it).
            for (int i = 0; i < ok; ++i) MH_DisableHook(at[i]);
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

// PSV - the variants by shader (render thread; the flush's tail may be the game thread's under a park, which
// stalls it). wsee_post and wsee_tail find or admit the bound shader (psv_pick); the main depth's clear makes
// what is queued (psv_cycle_head: device Create* alone, outside every draw, at most PSV_MAKE_N entries a clear,
// each up to three CreatePixelShader calls); Resources::release (a device reset, and every session's teardown) and the
// session's globals reset drop them all (psv_device_lost). key: the shader object - an identity, never dereferenced;
// its stamp, read from the bound object, names it. Past PSV_ENT_N shaders the entry least recently picked goes
// (psv_admit).
static constexpr uint8_t PSV_ST_NONE = 0u, PSV_ST_QUEUED = 1u, PSV_ST_MADE = 2u, PSV_ST_REFUSED = 3u,
                         PSV_ST_NOBLOB = 4u;
static constexpr uint32_t PSV_R_DEVICE = 11u;   // The device refused a variant (CreatePixelShader failed).
struct PsvEnt {
    const void* key = nullptr;
    uint64_t ser = 0;
    uint8_t st = PSV_ST_NONE;    // alpha + nw.
    bool own = false;               // Our own (the tail's): never a one pass.
    bool nw_none = false;           // NW binds no pixel shader.
    ID3D11PixelShader* alpha = nullptr;
    ID3D11PixelShader* nw = nullptr;
    ID3D11PixelShader* one = nullptr;   // PSB: the one pass's variant (null: refused - today's passes).
    uint32_t one_cb = 0, one_t = 0;     // Its constant-buffer and depth-view slots.
    uint32_t one_tz = 128u;             // PSB_ZB: and Zb's slot.
    uint64_t pick = 0;                  // PSV: its last pick's serial (g_psv_pick_n; the eviction's order).
};
static constexpr int PSV_ENT_N = 64;
// Entries made a clear: up to three creates each (alpha, nw, PSB's one) - the one variant is a full driver compile
// of the whole engine shader on the render thread, a hitch per shader at its first bind in each session and after
// each device reset (psv_device_lost drops the variants).
static constexpr int PSV_MAKE_N = 2;
static PsvEnt g_psv_ent[PSV_ENT_N];
// PSV: the picks' serial (render thread; 0 again with every entry - psv_device_lost). Per pick, not per cycle:
// a cycle stamp would tie a hot entry not yet picked this cycle with the last cycle's dead ones.
static uint64_t g_psv_pick_n = 0;
// g_psb.have (render thread): whether any engine entry holds a one-pass variant.
inline void psv_publish() {
    bool o = false;
    for (const PsvEnt& e : g_psv_ent) {
        if (!e.key || e.own) continue;   // PSV: the engine's alone (our tail's never hold a one pass).
        if (e.st == PSV_ST_MADE && e.one) o = true;   // PSB.
    }
    g_psb.have = o;
}
inline void psv_ent_drop(PsvEnt& e) {
    KH_SAFE_RELEASE(e.alpha);
    KH_SAFE_RELEASE(e.nw);
    KH_SAFE_RELEASE(e.one);   // PSB.
    e = PsvEnt();
}
// A shader first bound (render thread): queued where the store holds its bytes (one lock a new shader), else
// no-blob. A slot that held the same address (a dead shader) is reused first, then a free one, then the one least
// recently picked (PSV: a dead or cold entry before a hot one; the scan runs only at a new shader's admission).
// Past PSV_ENT_N distinct shaders in one frame the entries still cycle (the cache's size).
inline PsvEnt& psv_admit(const void* key, uint64_t ser, bool own) {
    int k = -1;
    for (int i = 0; i < PSV_ENT_N && k < 0; ++i) {
        if (g_psv_ent[i].key == key) k = i;
    }
    for (int i = 0; i < PSV_ENT_N && k < 0; ++i) {
        if (!g_psv_ent[i].key) k = i;
    }
    if (k < 0) {
        k = 0;
        for (int i = 1; i < PSV_ENT_N; ++i) {
            if (g_psv_ent[i].pick < g_psv_ent[k].pick) k = i;
        }
    }
    PsvEnt& e = g_psv_ent[k];
    psv_ent_drop(e);   // No variant is bound here: every pass's shader is set back before a post ends.
    e.key = key;
    e.ser = ser;
    e.own = own;
    e.st = PSV_ST_NOBLOB;
    if (ser != 0u) {
        std::lock_guard<std::mutex> g(g_psv_mx);
        const auto it = g_psv_store.find(key);
        if (it != g_psv_store.end() && it->second.ser == ser) {
            e.st = PSV_ST_QUEUED;
        }
    }
    psv_publish();
    return e;
}
// One re-issue's choice: on - the passes bind alpha (T, Tp, Tp2, Tf) and nw (NW; null: no pixel shader), and orig
// holds the bound shader (PSGetShader's reference) for the set-back. Off - nothing taken, nothing changes.
struct PsvPick {
    bool on = false;
    ID3D11PixelShader* alpha = nullptr;
    ID3D11PixelShader* nw = nullptr;
    ID3D11PixelShader* orig = nullptr;
    ID3D11PixelShader* one = nullptr;   // PSB: the one pass's variant where made (with alpha / nw's), and its
    uint32_t one_cb = 0, one_t = 0;     // slots.
    uint32_t one_tz = 128u;             // PSB_ZB: Zb's slot (128: none).
};
// tail: our own tail's part (wsee_tail). No variant (hook not in, no shader, class instances bound, no stamp,
// not made yet, refused): off.
inline void psv_pick(ID3D11DeviceContext* ctx, PsvPick& pk, bool tail) {
    if (g_psv_hook_state.load(std::memory_order_relaxed) != 1) return;   // No bytecode reaches us: not even a read.
    ID3D11PixelShader* ps = nullptr;
    ID3D11ClassInstance* ci[8] = {};
    UINT cn = 8;
    ctx->PSGetShader(&ps, ci, &cn);
    bool cls = cn > 0;
    for (ID3D11ClassInstance*& c : ci) {
        if (c) cls = true;
        KH_SAFE_RELEASE(c);
    }
    if (ps && !cls) {
        const uint64_t ser = psv_stamp(ps);
        PsvEnt* e = nullptr;
        for (PsvEnt& x : g_psv_ent) {
            if (x.key == ps && x.ser == ser) {
                e = &x;
                break;
            }
        }
        if (!e) e = &psv_admit(ps, ser, tail);
        e->pick = ++g_psv_pick_n;   // PSV: psv_admit's eviction order.
        if (e->st == PSV_ST_MADE) {
            pk.on = true;
            pk.alpha = e->alpha;
            pk.nw = e->nw_none ? nullptr : e->nw;
            pk.one = e->one;   // PSB.
            pk.one_cb = e->one_cb;
            pk.one_t = e->one_t;
            pk.one_tz = e->one_tz;   // PSB_ZB.
        }
    }
    if (pk.on) {
        pk.orig = ps;
        return;
    }
    KH_SAFE_RELEASE(ps);
}
// One variant from its patched bytes (psv_make's alpha / nw, psb_make_one's one; the clear, device Create*
// alone). g_psv_self keeps it out of the store.
inline uint32_t psv_create_one(ID3D11Device* dev, const std::vector<uint8_t>& v, ID3D11PixelShader** out) {
    struct PsvSelfScope {   // Held over the create alone, whatever leaves it.
        PsvSelfScope() { ++g_psv_self; }
        ~PsvSelfScope() { --g_psv_self; }
    } scope;
    const HRESULT hr = dev->CreatePixelShader(v.data(), v.size(), nullptr, out);
    if (SUCCEEDED(hr) && *out) return PSV_OK;
    KH_SAFE_RELEASE(*out);
    return PSV_R_DEVICE;
}
// PSB: the one pass's variant of an entry whose alpha / nw variants stand (psv_make). Refused or not created:
// none - the shader's re-issues keep the two variants' passes; the code is named once in the RPT
// (PSV_REFUSE_REPORT).
inline void psb_make_one(ID3D11Device* dev, PsvEnt& e, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> v;
    uint32_t cb = 0u, tt = 0u, tz = 128u;
    uint32_t ro = psb_patch(b.data(), b.size(), v, cb, tt, tz);
    if (ro == PSV_OK) ro = psv_create_one(dev, v, &e.one);
    if (ro == PSV_OK) {
        e.one_cb = cb;
        e.one_t = tt;
        e.one_tz = tz;   // PSB_ZB.
        return;
    }
    // PSV_REFUSE_REPORT: the refusal is named once per code (the code is PSV_R_* / PSB_R_*'s value, above);
    // the string is built at the make alone, never per frame. EA_ERR_CLASS: the patcher's refusal of an engine
    // shader is expected (a warning, logged); the device's (PSV_R_DEVICE) is an error.
    const std::string msg = "KH RenderIntegration: world see-through: an engine pixel shader has no one-pass "
                            "variant (refusal " + std::to_string(ro) +
                            "); its re-draws take the alpha and coverage passes";
    if (ro == PSV_R_DEVICE) report_error_once_safe(msg);
    else log_once_safe(msg);
}
inline void psv_make(ID3D11Device* dev, PsvEnt& e, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> v;
    uint32_t r = psv_patch(b.data(), b.size(), PSV_MODE_ALPHA, v);
    if (r == PSV_OK) r = psv_create_one(dev, v, &e.alpha);
    if (r == PSV_OK) {
        r = psv_patch(b.data(), b.size(), PSV_MODE_NW, v);
        if (r == PSV_NOPS) {
            e.nw_none = true;
            r = PSV_OK;
        } else if (r == PSV_OK) {
            r = psv_create_one(dev, v, &e.nw);
        }
    }
    if (r == PSV_OK) {
        // PSB: the one pass's variant beside them (not our tail's builtins, which write a second target and a
        // depth or coverage: refused by the patcher anyway).
        if (!e.own) psb_make_one(dev, e, b);
        e.st = PSV_ST_MADE;
        return;
    }
    KH_SAFE_RELEASE(e.alpha);
    KH_SAFE_RELEASE(e.nw);
    e.nw_none = false;
    e.st = PSV_ST_REFUSED;
    // PSV_REFUSE_REPORT: likewise, once per distinct message (an engine shader's: the code; ours: the bytes and
    // the code). EA_ERR_CLASS: an engine shader the patcher refuses is a warning (logged); a device refusal
    // (PSV_R_DEVICE) or a refused shader of ours (our builtins must reduce) is an error.
    const std::string msg = std::string("KH RenderIntegration: world see-through: ") +
                            (e.own ? "a tail pixel shader of ours, " + std::to_string(b.size()) +
                                                   " bytes, is not reduced"
                                             : std::string("an engine pixel shader is not reduced")) +
                            " (refusal " + std::to_string(r) + "); its re-issues keep the whole shader";
    if (e.own || r == PSV_R_DEVICE) report_error_once_safe(msg);
    else log_once_safe(msg);
}

// The main depth's clear (render thread, after wsee_cycle_note): the queued variants (PSV_TOMB: no device
// prune - a dead device's shaders die first and their blobs go at their release).
inline void psv_cycle_head(ID3D11DeviceContext* ctx) {
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return;
    int made = 0;
    for (PsvEnt& e : g_psv_ent) {
        if (made >= PSV_MAKE_N) break;
        if (e.st != PSV_ST_QUEUED) continue;
        std::vector<uint8_t> b;
        try {
            std::lock_guard<std::mutex> g(g_psv_mx);
            const auto it = g_psv_store.find(e.key);
            if (it != g_psv_store.end() && it->second.ser == e.ser) b = it->second.b;
        } catch (...) {
            b.clear();
        }
        ++made;
        if (b.empty()) {   // Gone from the store since (a dead shader's address reused).
            psv_ent_drop(e);
            continue;
        }
        try {   // An allocation refused leaves the entry refused, nothing half-made, the clear's work intact.
            psv_make(dev, e, b);
        } catch (...) {
            KH_SAFE_RELEASE(e.alpha);
            KH_SAFE_RELEASE(e.nw);
            KH_SAFE_RELEASE(e.one);   // PSB.
            e.nw_none = false;
            e.st = PSV_ST_REFUSED;
        }
    }
    dev->Release();
    if (made > 0) psv_publish();
}
// Resources::release (a device reset, and every session's teardown - session_teardown's g_res.release()) and the
// session's globals reset (rp_globals_reset; idempotent): every variant, our builtins' blobs (the engine's stay:
// the shaders they describe are not ours to forget).
inline void psv_device_lost() {
    for (PsvEnt& e : g_psv_ent) psv_ent_drop(e);
    {   // Our builtins' blobs: their shaders die in the same Resources::release (made again with the next ones).
        std::lock_guard<std::mutex> g(g_psv_mx);
        for (auto it = g_psv_store.begin(); it != g_psv_store.end();) {
            if (it->second.own < 0) {
                ++it;
                continue;
            }
            g_psv_books.bytes -= it->second.b.size();
            it = g_psv_store.erase(it);
        }
    }
    g_psv_pick_n = 0;   // PSV: every entry dropped above.
    psv_publish();
}
}