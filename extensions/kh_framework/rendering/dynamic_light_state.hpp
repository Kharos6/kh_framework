#pragma once

// The shared head (rendering_integration.hpp): the dynamic lights' shared state (DynLightsState, the window / pool
// records, the DLS map flags and casters). Never include it on its own.
namespace RenderIntegration {

// KH_DL_SHADOW slice budget: 8 lights x 6 cube faces is the ceiling; the array is grown to demand (dls_slices).
static constexpr int KH_DLS_SLICES_MAX = 8 * 6;

// From the lit material shaders' disassembly: the engine's non-sun lighting is per-pixel, driven by PS cb10[4]
// (control: [0].x point count and [1].x spot count as raw INT bit patterns, [2].x global distance scale, [3].xyz
// global diffuse multiplier) and cb11[192] (32 lights x 6 float4, points first, spots appended; record layout in the
// HLSL cbuffer comment).

static constexpr uint32_t KH_DL_MAX_LIGHTS = 32;   // cb11 capacity (192 / 6).
// That bound is the ENGINE's: one of its per-draw lists carries at most 32. The merged pool is uncapped, and so is
// what one of our meshes receives from it (KH_DL_RING, kh_dl_select).
static constexpr uint32_t KH_DL_LIGHT_BYTES = 6 * 16;
static constexpr uint32_t KH_DL_WIN_MAX = 16;   // Distinct per-draw windows captured per frame
static constexpr uint32_t KH_DL_VS_SLOTS = 13;   // VS cb slots captured (0..12).
// Anchor table capacity (DynLightsState::anchor_pos; full: dynlights_anchor_upsert evicts the oldest).
static constexpr uint32_t KH_DL_ANCHOR_N = 64;
// A harvest gap past this is a stall (alt-tab, load, time skip), not a frame.
static constexpr uint64_t KH_DL_POOL_STALE_MS = 500;
// KH_DL_WIN_SLOTS: wider than KH_DL_WIN_MAX because this table remembers windows across harvests as the engine
// cycles buffer addresses.
static constexpr uint32_t KH_DL_WIN_SLOTS = KH_DL_WIN_MAX * 4u;
// KH_DL_PRES_CUR: the distinct origins one harvest's placed windows with a live capture camera remember (more are not
// kept).
static constexpr uint32_t KH_DL_CUR_N = 4;

// The engine's constant buffers are shadowed on the CPU at the Unmap / UpdateSubresource hooks; the light sampler
// reads each draw's cb10 / cb11 / VS windows from those shadows. The harvest at the injection merges what was drawn
// since the last injection (the span), keyed on the shadow's write generation, so every list the engine uploaded is
// seen (cb11 is DISCARD-mapped for nearly every draw). That completeness is what lets presence replace the TTL.
static constexpr uint32_t KH_DL_WIN_MAX_CPU = 64;   // Windows per span (authoring + routine).
// A record the harvest could not place (no origin by derivation or vote) is remembered by its raw list-frame
// position, quantized to a metre, for KH_DL_UNPL_KEEP harvests; an upload whose unknown records are all remembered
// earns no window past the routine quota until that lapses, a buffer with no origin included (KH_DL_UNPL_ORIGIN) -
// unless it also carries two known lights, which clear the vote's bar: its noted records then author as unknown
// ones do. Presence and placeable lights are untouched.
static constexpr uint32_t KH_DL_UNPL_N = 256;     // Table entries (open addressing, 8-slot probe).

struct DlWindowMeta {
    void*    buf = nullptr;   // Weak identity (dedupe key with first).
    uint32_t first = 0;
    uint32_t bytes11 = 0;   // Lights bytes actually copied.
    uint16_t vs_bytes[KH_DL_VS_SLOTS] = {};   // VS constants copied per slot (0 = absent).
    // The cb11 shadow's write generation (0 = a GPU-arena window), the content hash of ctl + lights (per-harvest
    // dedupe) and the harvest this window belongs to.
    uint32_t gen = 0;
    uint64_t chash = 0;
    uint64_t seq = 0;
    uint64_t wseq = 0;   // KH_DL_REC_ORDER: the cb11 content's capture order (its shadow's wseq; 0 = GPU arena).
    // KH_DL_WIN_VIEW: the frame view (3 float4 columns) and true-world camera this window was drawn under
    // (kh_dl_meta_view). A trigger-to-trigger span holds the previous frame's post-trigger draws and this frame's,
    // which need not share a camera, so one latch per span cannot serve both.
    float    view[12] = {};
    float    cam[3] = {};
    uint8_t  view_ok = 0;
    uint8_t  view_live = 0;   // KH_DL_PRES_CUR: the view is the clear's own fetch (kh_dl_view_live).
};

struct DlPoolLight {
    float    rec[24] = {};   // The 6-float4 record verbatim (producer side) +0 rewritten to
                             // absolute world.
    uint8_t  spot = 0;   // Index >= its list's pointN.
    uint64_t stamp = 0;   // Last re-sight (persistence TTL).
    uint16_t gap_max_ms = 0;   // Worst observed re-sight gap (cadence evidence).
    uint16_t sightings = 0;
    // Session-unique identity, minted at first sight, carried through compaction. 0 = unminted. Lets a shadow slot
    // name its light when two lights tie on position (a flashlight and a muzzle flash on one weapon).
    uint32_t id = 0;
    // The newest harvest whose span carried this light; presence at harvest N is seq == N. The depth-clear cycle is
    // not usable as the key: the engine clears the main depth many times a frame at night.
    uint64_t seq = 0;
    uint64_t first_seq = 0;   // KH_DL_DIAG (CN-4): the harvest (g_dl.harvest_seq) whose merge created it.
    // KH_DL_REC_ORDER: the capture order (KhCbShadow::wseq) of the content rec holds; 0 = unordered (a GPU-arena
    // window's). A write never replaces a newer content with an older one.
    uint64_t rec_wseq = 0;
};

struct DynLightsState {
    // Published mirror (render thread writes at harvest; getRenderStats reads it on the game thread - dlValid and
    // the light list).
    bool     valid = false;
    uint32_t point_n = 0, spot_n = 0;
    float    lights[KH_DL_MAX_LIGHTS * 24] = {};
    bool     view_valid = false;
    float    cam[3] = {};   // Capture camera (engine axes).
    // Staging machinery (render thread only).
    ID3D11Buffer* staging[2] = {};
    uint8_t  inflight[2] = {};
    uint32_t slot_bytes11[2] = {};   // Lights bytes actually copied.
    float    pending_cam[2][3] = {};
    uint8_t  pending_view_ok[2] = {};
    ID3D11Buffer* arena[2] = {};
    uint8_t  arena_inflight[2] = {};
    int      arena_write = 0;
    DlWindowMeta win_meta[2][KH_DL_WIN_MAX_CPU] = {};   // KH_DL_CPU: the CPU cap bounds both.
    uint32_t win_count[2] = {};
    std::vector<uint8_t> cpu_arena[2];   // KH_DL_CPU: the same window layout, CPU-resident.
    uint8_t  side_miss[2] = {};          // KH_DL_CPU: a draw this side could not capture whole.
    uint32_t side_uploads[2] = {};       // KH_DL_PRESENCE: uploads seen this side.
    uint64_t harvest_seq = 0;            // KH_DL_SEQ: harvests so far; uploads are tagged with the next.
    int32_t  unpl_q[KH_DL_UNPL_N][3] = {};   // KH_DL_UNPL: quantized list-frame positions...
    uint64_t unpl_seq[KH_DL_UNPL_N] = {};    // ...and the harvest that noted them (0 = empty).
    // Merged absolute-world pool (render thread writes at harvest; the draw paths read).
    std::vector<DlPoolLight> pool;   // Uncapped; grows at harvest, TTL-swept.
    uint32_t pool_n = 0;
    // Anchor table (derived-provenance light positions; the vote's ground truth, decoupled from shading
    // persistence).
    float    anchor_pos[KH_DL_ANCHOR_N][3] = {};
    uint64_t anchor_stamp[KH_DL_ANCHOR_N] = {};
    uint32_t anchor_n = 0;
    // The last harvest's merge (steady_now_ms): KH_DL_POOL_STALE_MS ages the pool from it.
    uint64_t pool_stamp = 0;
    float    main_gdiff[3] = { 1.0f, 1.0f, 1.0f };
    float    main_scale = 1.0f;
    uint8_t  main_ref_valid = 0;
    // KH_DL_REF_MEDIAN: the brightest candidate's (main_gdiff rgb, main_scale) of the last three harvests that had
    // one (a placed window, or KH_DL_REF_SEEN an upload re-sighted through its buffer's origin), oldest first, and
    // how many are held (0 - 3; the session reset empties it).
    float    ref_hist[3][4] = {};
    uint32_t ref_hist_n = 0;
    // Per-window resolved origin.
    float    win_ox[KH_DL_WIN_SLOTS] = {};
    float    win_oz[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_oset[KH_DL_WIN_SLOTS] = {};
    uint64_t win_oseq[KH_DL_WIN_SLOTS] = {};   // KH_DL_PRES_FRESH: the harvest that set it (0 = none).
    // KH_DL_PRES_CUR: the origins (x, z) harvest cur_oseq placed windows with a live capture camera (view_live) at
    // (0 = none yet), cur_n of them, each with the capture camera (x, z) of the last such window placed there; and
    // whether it also placed one without a live camera, or at a (KH_DL_CUR_N + 1)th origin (cur_amb).
    float    cur_o[KH_DL_CUR_N][4] = {};
    uint32_t cur_n = 0;
    uint64_t cur_oseq = 0;
    uint8_t  cur_amb = 0;   // Harvest cur_oseq placed a window that could contradict unseen: it vouches for none.
    // KH_DL_REF_SEEN: span ref_seen_seq's brightest ctl (gdiff rgb, scale) among the uploads presence re-sighted a
    // record of through their buffer's origin, and its brightness (the sum of the three; -1 = none).
    float    ref_seen[4] = {};
    float    ref_seen_lum = -1.0f;
    uint64_t ref_seen_seq = 0;
    // The pool vote runs as a confirmation, not only a fallback: the engine culls its light list to the camera, so a
    // correct origin places at least one of a window's lights near the camera and a wrong one displaces them all.
    // Keyed per window: a shared tuple would let the last window to finish overwrite another's.
    uint8_t  win_hint_set[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_kind[KH_DL_WIN_SLOTS] = {};
    int32_t  win_hint_slot[KH_DL_WIN_SLOTS] = {};
    uint32_t win_hint_off[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_form[KH_DL_WIN_SLOTS] = {};
    int32_t  win_hint_wslot[KH_DL_WIN_SLOTS] = {};
    uint32_t win_hint_woff[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_wform[KH_DL_WIN_SLOTS] = {};
    uint32_t win_fail_lo[KH_DL_WIN_SLOTS] = {};
    uint32_t win_fail_run[KH_DL_WIN_SLOTS] = {};
    uint64_t win_use[KH_DL_WIN_SLOTS] = {};   // KH_DL_SLOT_LRU: the lookup that last named the entry.
    uint64_t win_use_n = 0;                   // Lookups so far (kh_dl_win_slot).
};
static DynLightsState g_dl;
static uint32_t  g_dls_n = 0;
// KH_DLS_ALLOW: allowDynamicShadows (script-set; off by default and at every session start). Off: kh_dls_select
// holds no slot, so no light renders a map, the world pass stands down and every mesh reads its dynamic-light
// shadows as lit. The lights still light the meshes (fill_dynlights_cb).
static std::atomic<bool> g_dls_allowed{ false };
static uint64_t g_dls_map_valid = 0;                // Bitmask of slots with a usable map.

struct KhDlswCaster {
    float pos[3]; float size[3]; float rot[9]; bool rotated; int mesh; bool lod_lock;
    float lod_r;    // KH_SKEL_LOD_R, from the SunCaster: the mask's pick radius.
    float res[3];   // KH_POS_RES, from the SunCaster.
    uint32_t slot = 0xFFFFFFFFu;   // KH_CLOTH: which substitute vertex buffer this caster draws from.
    // KH_DLSW_MASK_ALPHA: the mask's own lanes, copied from the SunCaster.
    bool visible;                 // false = casterOnly invisible: no pixel is ours, not drawn.
    float alpha;                  // < 0.999 = a whole translucent object: writes no depth, not drawn.
    const KhMaterialSet* mats;    // Alpha-carrying set, or nullptr.
    bool alpha_caster;            // The sun's verdict (kh_sun_caster_of).
    bool in_front;                // KH_INFRONT: masked under the hands pair, after the world casters.
    bool no_foot;                 // KH_FOOT_BLEND: a see-through blend mode owns no pixel, not drawn.
};
// The DLS world mask's casters (kh_dlsw_mask_render). Uncapped; rebuilt by kh_dls_render (cleared first) and cleared
// when kh_dls_frame has nothing to render; freed only by the session reset (kh_session_globals_reset's kh_reinit).
static std::vector<KhDlswCaster> g_dlsw_casters;

// The light maps render on their own bracket, free of the sun pass's gates. Once per cycle, whichever mesh pass
// arrives first (the injection, or the flush fallback) - the kh_dls_select rule.
static uint64_t g_dls_frame_cycle = ~0ull;      // once-per-cycle guard (state, not a lane).
}