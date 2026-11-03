#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the latched
// engine lighting: the fog staging, the sky sun, the world-lighting owner, the raw sun axis and the probes. Never
// include it on its own.
namespace RenderIntegration {

// The game thread stages the fog (stage_world_lighting, under g_fog_stage_mu) and flush_locked publishes it, so
// render-thread consumers never see it mid-write.

// Game-thread staging (the extension self-fetches fogParams). RV's fog density decays exponentially with height above
// fogBase.
static bool  g_fog_staged_valid = false;
static float g_fog_staged[3] = { 0.0f, 0.0f, 0.0f };
static bool  g_fog_valid = false;
static float g_fog[3] = { 0.0f, 0.0f, 0.0f };
// A raw publish stepping past SRAW_JUMP_DEG: the clear hook resets the union's map hash so the re-fit is
// unconditional.
static bool g_sun_jump_pending = false;
// The sky sun: the reference direction and its motion (the latched engine lighting).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct SkySunState {
    float ref[3] = { 0, 0, 0 };
    bool  ref_valid = false;
    uint64_t ref_ms = 0;
    float    mot_ref[3] = {};   // Sky dir at the window open.
    uint64_t mot_ref_ms = 0;
    float    mot_deg = -1.0f;   // Last ~1 s sky motion (-1 = no window yet).
};
static SkySunState g_skysun;
// The sun lane's two predicates. FRESH: the sky block was uploaded inside the window. OK: fresh AND above the horizon -
// the engine writes the sun lane all night, and a lane at or below SUN_AXIS_LANE_MIN_Y is not the light anything
// shadows with (a below-horizon sun flipped skyward is the anti-solar point, which at full moon would pass the moon
// witness).
static constexpr uint64_t SUN_AXIS_SKY_FRESH_MS = 1000;
static constexpr float    SUN_AXIS_LANE_MIN_Y = 0.10f;

// The published sun direction (skyward, unit, engine axes): written by
// sun_raw_frame at the main depth clear, mirrored into g_sun_dir_engine.
static bool  g_pub_valid = false;
static float g_pub_dir[3] = {};
// The band positions' far borders, rebuilt by every injection fill (BAND_TAB_FILL; -1 = no sane band there).
static float g_band_tab_far[8]   = { -1,-1,-1,-1,-1,-1,-1,-1 };

// LIGHT_BASE: the lighting publish. The engine uploads the same lighting block many times per frame, every variant
// the one scene lighting scaled DOWN per object (0.3 - 0.8 of the brightest). Each admitted upload owns the main-scene
// draws issued until the next (g_blko.draws, our draws excluded); uploads in one luminance band (probe_lum_band)
// form a cluster; at the main depth clear of a world cycle the BRIGHTEST cluster holding at least BLKO_BASE_SHARE of
// the lit members publishes (attenuation only goes down). A standing base is kept down to BLKO_BASE_KEEP. No clock,
// no hold. Non-world cycles keep the standing publish. Picking by luminance with holds, or by draw share, follows the
// camera - do not reintroduce.
static constexpr int   BLKO_MAX = 16;
static constexpr float BLKO_BASE_SHARE = 0.10f;   // Admission share of the frame's lit members (BLKO_MEMBER).
static constexpr float BLKO_BASE_KEEP  = 0.05f;   // A standing base is kept down to this member share.
// LIT_EVIDENCE: a close with real evidence publishes at once. A SPARSE close - fewer than LITE_CONF_DRAWS lit
// draws owned across its clusters - may not overturn a standing publish with a value outside its band unless that
// value repeats for LITE_CONF_CLOSES consecutive non-empty closes. Measured: the frame the terrain leaves the view
// owns two or three lit draws, all the sky pass's; its block (sun 0, a faint blue ambient - not all-zero, so admitted)
// won that vote and stood black through every following sky-only close, which publish nothing. A genuinely dark
// sparse scene still publishes, LITE_CONF_CLOSES closes late.
static constexpr uint32_t LITE_CONF_DRAWS  = 16;
static constexpr uint32_t LITE_CONF_CLOSES = 8;
// The world-lighting candidate vote (LITE_CONF_CLOSES): the candidate block and its run of closes.
// Owner: the render thread.
// Session reset: session_globals_reset resets every member.
struct LiteCandState {
    uint32_t n = 0;      // Consecutive sparse out-of-band closes agreeing with the candidate.
    float    sl = 0.0f;  // The candidate's sun / ambient sums.
    float    al = 0.0f;
};
static LiteCandState g_lite_cand;
struct BlkOwn { float amb[3]; float sun[3]; uint32_t draws; uint8_t flag;
                uint32_t members; uint32_t rep_draws; float al; float sl; };
// The world-lighting owner (the block owner): the lit draws counted per close, the open block and its ambient / sun,
// the generation of the published block.
// Owner: the render thread.
// Session reset: session_globals_reset resets every member.
struct BlockOwnerState {
    uint64_t draws = 0;          // Main-scene draws, monotonic.
    BlkOwn cur[BLKO_MAX] = {};   // The open frame's clusters.
    int      n = 0;
    uint64_t open_draws = 0;     // g_blko.draws when the last upload landed.
    bool     open = false;
    int      open_i = -1;        // Cluster the open span belongs to.
    float    open_amb[3] = {};   // The open span's own values (a cluster member).
    float    open_sun[3] = {};
    // DLSW_GEN_AMB: the brightest ambient and sun, per channel, over the last non-sparse world frame's lit clusters
    // that owned a draw (the engine's per-object variants are the scene lighting scaled DOWN, so the brightest is the
    // scene's own - see LIGHT_BASE) - the dynamic-light world pass's denominator floor (dls_world_pass_body). The
    // published block is ONE cluster - the brightest holding BLKO_BASE_SHARE of the lit members (LIGHT_BASE) - so
    // which cluster it is follows what is on screen: turn the camera, a bright cluster's members fall under the bar and
    // a darker one can win, the pass's Ltotal drops, and the whole world shadow darkens in one step. KhDlsWorldFactor's
    // contract is a generous denominator (every error toward under-darkening); the brightest world cluster keeps it
    // generous whichever cluster the publish picked. Our meshes' lighting still reads the publish.
    float    gen_amb[3] = {};
    float    gen_sun[3] = {};
    bool     gen_valid = false;
};
static BlockOwnerState g_blko;
static constexpr int      SRAW_CYC = 16;      // Samples one cycle holds (8 cascades measured).
static constexpr float    SRAW_JUMP_DEG = 1.0f;   // A published step past this is a jump (the union re-fits unconditionally).
static constexpr uint64_t SRAW_SKY_FALLBACK_MS = 1000;   // No cascade sample this long: the sky lane publishes.
static constexpr float    SRAW_LANE_WIT_DEG = 10.0f;     // = SUN_AXIS_MAX_DEG: the lane is the tracked light within this.
                                                            // (Freshness and the horizon floor: sun_axis_lane_fresh / _ok.)
static constexpr float SRAW_LANE_STILL_DEG = 0.05f;   // SRAW_LANE_HOLD: a lane step under this is 'did not move'.
// SRAW_SETTLE: the consensus is published only when it is STILL, and what is published is its median over the last
// SRAW_RING sampled world closes, not the raw close. Two things move the raw consensus with no sun behind them: its
// convergence after a jump (a second or more) and its departure under camera rotation (the engine re-fits its cascades
// to the view; tens of degrees for a few frames on a flick, a lean on a turn). A third moves it at rest: single-close
// spikes up to ~1 deg (measured 0.001 deg mean / < 1 deg max while the sky stood still). Do not test the raw close's
// step: it never settles, and its cap then publishes a raw spike. The median is still to ~0.001 deg at rest; its
// close-to-close step against the elevation-scaled bar (0.02 rad x sin^2, floored at SRAW_SETTLE_MIN_DEG, capped at
// SRAW_SETTLE_MAX_DEG) for SRAW_SETTLE_MS is 'settled'. A moving sun is witnessed by the lane
// (SRAW_LANE_STILL_DEG) and publishes at once. Unsettled and unwitnessed, the standing publish holds: the lane
// after a jump (SRAW_JUMP_HOLD), else the last publish; SRAW_HOLD_CAP_MS bounds those two holds (night, or a sun
// run so fast the lane test is not the witness) - not the standing-lane hold (Src 4 while the lane stands still) nor
// SRAW_FLOOR_HOLD (no clock). A shadow tip moves 1 / sin^2(elevation) per radian of axis - 365x at
// 3 deg - which is why none of this is visible at noon.
static constexpr int      SRAW_RING          = 32;     // ~0.5 s of world closes.
static constexpr uint64_t SRAW_HOLD_CAP_MS   = 6000;
static constexpr uint64_t SRAW_SETTLE_MS     = 400;
static constexpr float    SRAW_SETTLE_MIN_DEG = 0.005f;
static constexpr float    SRAW_SETTLE_MAX_DEG = 0.15f;
// The raw sun axis: its sample cycle, the sky steps and gaps, the median ring, the hold and the settle clocks.
// Owner: the render thread.
// Session reset: session_globals_reset resets every member.
struct SunRawState {
    float    cyc[SRAW_CYC][3] = {};   // The open cycle's raw axes (world, skyward, unit).
    int      cyc_n = 0;
    uint64_t last_sample_ms = 0;   // Last cascade sample (the sky fallback's clock).
    // 0 none, 1 cascade consensus, 2 sky lane (fallback), 3 sky lane (witnessed), 4 sky lane (held).
    uint8_t  last_src = 0;
    float    sky_prev[3] = {};    // The lane at the previous close (its own step per publish).
    bool     sky_prev_valid = false;
    float    sky_step_deg = -1.0f;   // How far the lane moved since the previous close (-1 = no fresh lane).
    float    pub_gap_deg = -1.0f;   // Consensus vs the standing publish at the close (pre-publish).
    float    sky_gap_deg = -1.0f;   // Consensus vs the sky lane (skyward), -1 = no fresh lane.
    float    ring[SRAW_RING][3] = {};   // Raw consensus per sampled world close.
    int      ring_n = 0;                   // Filled entries (<= SRAW_RING).
    int      ring_i = 0;                   // Next write slot.
    float    med_prev[3] = {};             // The median at the previous close (the step's reference).
    bool     med_prev_valid = false;
    uint64_t hold_until_ms = 0;   // SRAW_JUMP_HOLD: the lane stands until the median settles or this passes.
    uint64_t cons_moved_ms = 0;   // Last close whose median stepped past the bar.
    uint64_t unsettled_ms = 0;    // First unsettled close of the current run (0 = settled); the cap's clock.
};
static SunRawState g_sraw;

static constexpr float SUN_AXIS_MAX_DEG = 10.0f;
}