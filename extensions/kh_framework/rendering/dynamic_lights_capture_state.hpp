#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the
// dynamic-light harvest, part 1: the CB shadow, the want gate, the presence memo and the sampled draws. Never include
// it on its own.
namespace RenderIntegration {
static constexpr uint32_t DL_CTL_BYTES = 4 * 16;
static constexpr uint32_t DL_ARR_BYTES = DL_MAX_LIGHTS * DL_LIGHT_BYTES;
static constexpr uint32_t DL_STAGE_BYTES = DL_CTL_BYTES + DL_ARR_BYTES;
static constexpr uint32_t DL_VS_CONSTS = 80;   // Constants per slot (view fits anywhere in 80).
static constexpr uint32_t DL_VS_BYTES = DL_VS_CONSTS * 16;
static constexpr float    DL_DERIVE_Y_TOL = 1.5f;   // Heights are absolute in the list frame.
static constexpr float    DL_DERIVE_GRID_TOL = 1.0f;   // Strong routes: within this of a whole land-grid cell.
static constexpr float    DL_DERIVE_ROT_TOL = 0.03f;   // Candidate rotation vs the frame view.
// Windows with only loose candidates go unresolved (dark-safe), never flapping.
static constexpr float    DL_ORIGIN_RES_MAX = 0.03f;
static constexpr uint64_t DL_ANCHOR_TTL_MS = 60000;
static constexpr float    DL_ANCHOR_RES = 0.005f;
static constexpr uint32_t DL_WIN_VS_BYTES = DL_VS_SLOTS * DL_VS_BYTES;
static constexpr uint32_t DL_WIN_BYTES = DL_STAGE_BYTES + DL_WIN_VS_BYTES;
static constexpr uint32_t DL_ARENA_BYTES = DL_WIN_MAX * DL_WIN_BYTES;
static constexpr uint64_t DL_TTL_CAP_MS = 1500;   // Proven ceiling (sparse worst case).
static constexpr uint64_t DL_TTL_FLOOR_MS = 250;   // Safety floor (unproven cadence).
// The tail a light with proven cadence gets after the engine stops emitting it - how long a despawned light keeps
// lighting our meshes. DL_TTL_FLOOR_MS stays long: an unproven light (a muzzle flash seen once) keeps the long tail,
// which is what makes sporadic lights visible rather than a one-frame blip.
static constexpr uint64_t DL_TTL_DENSE_FLOOR_MS = 50;
static constexpr uint64_t DL_GAP_MULT = 2;   // TTL = mult x worst observed gap.
// DL_RANGE_PROBE: how many of a window's records the origin arbiter probes (the engine's list is not
// distance-sorted; eight is far past where a correct origin shows itself).
static constexpr uint32_t DL_RANGE_PROBE = 8;
// Two residuals this close are the same residual.
static constexpr float DL_RES_TIE = 1.0e-6f;
// How much nearer a candidate must put the window's closest light to the camera before proximity decides.
static constexpr float DL_NEAR_MARGIN_M = 50.0f;
static constexpr float    DL_ORIGIN_RANGE_M = 3000.0f;   // Arbiter bar, both routes.
static constexpr uint32_t DL_DENSE_SIGHTS = 6;   // Across at least this many sightings.
// A lifetime maximum never decays: one unlucky early gap would price an entry's
// tail for its whole life. The high-water mark is the transient tail.
static constexpr uint32_t DL_GAP_DECAY_NUM = 7;   // Mark *= 7/8 per sighting.
static constexpr uint32_t DL_GAP_DECAY_DEN = 8;
static constexpr float    DL_MATCH_M = 1.0f;   // Pool update match radius (same light
                                                  // re-sighted).
// The engine uploads a distinct list for nearly every draw, so a window per upload is not a design. Presence is decided
// without one: the origin the last harvest resolved for the upload's buffer puts the record in world and the re-sight
// cube names the light (dynlights_presence_upload). A window is made only on the routine quota, for a buffer with no
// origin yet (unless its unmatched records are all noted - DL_UNPL), or when the upload carries a record no pool
// light matches.
static constexpr uint32_t DL_WIN_ROUTINE = 8;
static constexpr uint32_t DL_UNPL_KEEP = 16;   // Harvests a note stands (~a quarter second).
static constexpr uint32_t CBS_N = 512;           // Shadow table entries (open addressing).
static constexpr uint32_t CBS_PROBE = 16;        // Probe run; the LRU of a full run is recycled.
struct CbShadow {
    void*    id = nullptr;   // Weak identity (the resource pointer), compare only.
    uint32_t bytes = 0;      // Bytes held: the buffer's width, capped at the scratch size.
    uint32_t gen = 0;        // Write generation: the global counter at the last capture (never 0 once written).
    // DL_REC_ORDER: the capture's place in capture order (g_dlcap.cbs_wseq at the write; 0 = never written). gen
    // wraps at 32 bits; this orders two captured contents of any identities, and is the LRU clock (CBS_LRU_WSEQ).
    uint64_t wseq = 0;
    // The (generation, first constant, span) the sampler last processed this
    // buffer at - the O(1) 'same upload again' test.
    uint32_t pres_gen = 0;
    uint32_t pres_first = 0;
    uint64_t pres_seq = 0;
    std::vector<uint8_t> data;
};
alignas(64) static CbShadow g_cbs[CBS_N];   // HOT_ALIGN (one entry, one line).
// DL_IDLE - the harvester's whole-buffer capture (cbs_write: every constant buffer the engine maps, copied whole
// out of write-combined memory, ~15,000 a frame), run every frame, cost a lit scene without dynamic lights 9 ms a frame
// for nothing (measured in the map hooks outside every scanner; every harvest incomplete, the pool empty). While the
// harvester has seen no light for DL_IDLE_FRAMES harvests, the capture runs every DL_IDLE_EVERY-th cycle only
// (the scratch's eager capture serves the scanners on the others); a light in the pool restores every frame at once, so
// a light switching on is picked up within the cadence.
static constexpr uint32_t DL_IDLE_FRAMES = 8u;
static constexpr uint64_t DL_IDLE_EVERY = 8u;
static constexpr uint32_t DL_WANT_N = 64u;   // DL_WANT_GATE: identities the table holds (measured at night: 25 a frame).
// DL_WANT_GATE - the identities persist across cycles with the cycle each was last asked for; the capture copies a
// buffer whole only when its identity was asked for within DL_WANT_KEEP cycles (measured at night: 25 identities a
// frame among ~22,000 maps). The identities come from the sampled draws' bindings, not from the copies, so the table
// fills on the first sampled draw and the capture follows it the next cycle; a new identity costs the harvester one
// incomplete frame. Render thread throughout.
// DL_WANT_STALE: a shadow holds the last CAPTURED upload of its identity, so it is current only while no upload of
// that identity has gone to the scratch since - where the gate sends every one it refuses: a window that lapsed, or an
// identity the table evicted (each later upload of it refused until the sampler asks again). Every refused upload
// empties its identity's shadow (cbs_stale: bytes 0 and no presence key, the slot and its probe run kept; one probe
// of the table, a miss for an identity never captured), so the sampler's next read of it is a miss - the one incomplete
// frame above - until a capture writes it again; without that emptying, the old content would be read as this span's (a
// light authored at a stale position, or a dead one re-stamped). A buffer not uploaded again keeps its shadow - its
// content as far as the funnel saw: a write the capture never sees (one while the capture is off, an
// UpdateSubresource1, a boxed UpdateSubresource, a copy into the buffer) leaves the shadow as it was, read as current.
// The idle cadence (DL_IDLE) is left as it is: its cycles capture nothing by design and its shadows hold an empty
// pool's content.
static constexpr uint64_t DL_WANT_KEEP = 4u;
// DL_WANT_INDEX - the table's entries found by identity through an open-addressed index (linear probing, at most
// half full: each live entry's slot + 1, 0 = empty) instead of a walk of the entries: the gate asks on every captured
// upload while the light capture runs (dl_cpu_capture_now; ~38,000 a frame in a heavy interior). The entries
// themselves (their order, the stalest replaced, ties to the lowest slot) do not depend on it; the index is rebuilt
// whole on that replacement (a full table and a new identity) and cleared with the entries (g_dlcap.keep_n = 0). Render
// thread throughout, as the table.
static constexpr uint32_t DL_WANT_IX = 2u * DL_WANT_N;
static_assert((DL_WANT_IX & (DL_WANT_IX - 1u)) == 0u && DL_WANT_N < 255u, "DL_WANT_INDEX: the index shape");
alignas(64) static uint8_t g_dl_keep_ix[DL_WANT_IX];   // HOT_ALIGN.

// DL_CHASH - the twin check's content hash (dynlights_sample_draw_cpu): XXH64 (seed 0) of ctl || records, read 8
// bytes at a time in four independent lanes. ctl is exactly two of XXH64's 32-byte stripes, so streaming it and then
// the records is XXH64 of the two laid end to end. Only ever compared for equality within one span. Not a
// byte-at-a-time hash: FNV-1a runs ~4 cycles a byte on one multiply chain (a 32-light list is 3,136 bytes, every new
// upload).
static constexpr uint64_t XXH_P1 = 0x9E3779B185EBCA87ull;
static constexpr uint64_t XXH_P2 = 0xC2B2AE3D27D4EB4Full;
static constexpr uint64_t XXH_P3 = 0x165667B19E3779F9ull;
static constexpr uint64_t XXH_P4 = 0x85EBCA77C2B2AE63ull;
static constexpr uint64_t XXH_P5 = 0x27D4EB2F165667C5ull;

// DL_PRES_MEMO - presence's match, remembered. The engine uploads a light list for nearly every draw, and the draws
// of one room carry the same list again and again; each upload would run the whole match (every record against every
// pool light), which nothing but the pool and the key below decides. An entry holds a run's outcome - the light each
// record picked, the rewrites it owed, the stale verdict, the counts - and a later upload of the same list replays it:
// the sightings (dl_pres_sight, the loop's own body) and the tail (the rewrites, with that upload's own capture
// order and bytes; the reference) run as a run would. Exact: an entry is stored only by a run that is a fixed point -
// its rewrites changed no record byte, so the pool it matched against is the pool after it - and it is replayed only
// while nothing the match reads has changed since: the key is the content itself (ctl and the counted records, compared
// byte for byte - the hash only picks the entry), the record count, the span (seq, which the unplaced notes are
// read against), the buffer's origin and whether it may rewrite (rw: the origin table, the cycle's view and the
// main reference), the land grid as read (dl_grid_base / dl_grid), and the epoch below. The rest of what the
// match reads - the pool (records, count, order) and the unplaced notes - is written only by presence's own rewrites,
// the harvest and the session / device resets, and each of those moves the epoch (a rewrite only when it changes a
// record's bytes: one that leaves them writes no more than spot and rec_wseq, which the match does not read); the
// origin table, the cycle's view, the main reference and the land grid reach the match only through the keyed values,
// whoever writes them. The sightings (stamp, gap mark, count, seq) are not read by the match, so a replay applies them
// per upload exactly as a run does: every count the TTL and the fast lane read is the one the full runs gave. Render
// thread (as g_dl).
static constexpr uint32_t DL_PMEMO_N = 32;   // Entries (power of two), direct-mapped on the content hash.
struct DlPresMemo {
    uint64_t epoch = 0;   // g_dlcap.pres_epoch at the store; 0 = empty.
    uint64_t seq = 0;     // The span (seq).
    uint32_t total = 0;   // Records matched.
    uint32_t ox = 0, oz = 0, cb = 0, q = 0;   // Bits: the origin, dl_grid_base, dl_grid.
    uint8_t  origin = 0, rw = 0, stale = 0, any_org = 0;
    uint32_t unknown = 0, unpl = 0, hits = 0;
    uint32_t pick_n = 0;                      // The picks in record order (records that matched nothing skipped).
    int32_t  pick[DL_MAX_LIGHTS] = {};
    uint32_t rw_n = 0;                        // The rewrites owed (pool light, record).
    int32_t  rw_pick[DL_MAX_LIGHTS] = {};
    uint32_t rw_rec[DL_MAX_LIGHTS] = {};
    uint8_t  bytes[DL_STAGE_BYTES] = {};   // ctl, then total records.
};

// DL_PRES_HIX - presence's height index. Without it a run (a DL_PRES_MEMO miss) stands every record against every
// pool light, and DL_PRES_BAND turns away first each light whose height is not within the band of the record's -
// fabsf(light - record) < DL_MATCH_M (1 m), or <= 0.25 m, inside it. In a town that is most of the pool for each
// record. The index holds the pool's heights in ascending order with their pool indices, so a record visits only the
// lights whose height lies in [r - 1, r + 1], and visits them in POOL order, running the loop's body on them unchanged
// - the band test first.
// Exact: a float difference under 1 is an exact difference under 1 (rounding is monotone and 1 is a float), so every
// light the band admits has r - 1 < h < r + 1 exactly; rounding to double is monotone too, so fl(r - 1) <= h <=
// fl(r + 1) - the interval's ends taken in double, inclusive (at large |r| both round onto r itself). A NaN height is
// left out of the index and fails every compare of the band; a light the band turns away reached only its
// `continue`. So the picks, the claims, the census, the stale verdict and every tie (a pick replaces only on a
// strictly lower cost: the first in pool order wins) come out as the whole-pool walk gives them.
// Pool order without a comparison sort: the candidates are marked in a pool-sized bit map and read back word by
// word, lowest bit first (dl_bit_low), which is ascending pool index; reading clears the marks.
// Fresh: checked whenever g_dlcap.pres_epoch has moved since the last check - every writer of the pool moves it
// (DL_PRES_MEMO's note: the harvest, the resets, a rewrite that changes a record's bytes) - or the pool's count
// differs. The heights' bits are compared with the index's: street lamps keep their heights, so a harvest or a colour
// rewrite leaves it standing. A presence rewrite that moves a light's height moves its entry in place
// (dl_phix_move), so the compare finds it in step. Any other difference makes it stale (invalid), and it is rebuilt
// only once the whole-pool walks run while it stood stale have visited DL_HIX_PAYBACK times the pairs a rebuild
// costs (DL_HIX_DEBT: without it a run of a few records after a height change pays a whole rebuild - up to 8 x the
// walk it replaces). A pool under DL_HIX_MIN_POOL lights always walks (the index cannot pay there). Which way a run
// goes never changes its outcome - both are the same walk over the same lights in pool order - only its cost.
// DL_HIX_OOM: the index is marked invalid before its rebuild's allocations (and around an in-place move, which
// allocates nothing) and valid only after it completes, so an allocation that throws leaves it invalid (rebuilt from
// scratch later), never half-built and in use.
// A record whose candidates exceed a sixth of the pool walks the whole pool - the same result, and there the walk is
// the cheaper (benchmarked when the index was written, not re-run since: a band holding a quarter of a large pool ran
// at 1.1 - 1.35 x the walk with the index, a seventh at 0.50 - 0.55 x). Neither way is always faster than the pre-band
// walk: on pools of a few hundred lights whose heights mostly lie within a few metres (the band holding a third to two
// thirds of the pool, where the index stands down), the band's own branch mispredicts and a run took up to ~1.6 x the
// pre-band walk (g++; ~1.3 x clang; MSVC unmeasured) - at the measured pools (25 - 48 lights) 0.7 - 0.9 x. The test is
// one compare after the lower bound: is the (n / 6 + 1)-th candidate inside the band? - so a dense band pays one binary
// search before the walk, and nothing is marked. Render thread (as g_dl); the session reset reinitialises it.
static constexpr uint32_t DL_HIX_MIN_POOL = 64u;   // Smaller pools always walk.
static constexpr uint64_t DL_HIX_PAYBACK = 4u;     // Stale walks repay this many rebuilds before one is made.
struct DlPresHix {
    bool     valid = false;                        // hb / hs / mark describe the pool's heights (DL_HIX_OOM).
    uint64_t epoch = 0;                            // g_dlcap.pres_epoch at the last check that found it in step.
    uint32_t n = 0;                                // g_dl.pool_n at the build.
    uint64_t debt = 0;                             // DL_HIX_DEBT: pairs walked while stale, since the last build.
    std::vector<uint32_t> hb;                      // Each pool light's height bits.
    std::vector<std::pair<float, uint32_t>> hs;    // The non-NaN heights, ascending, with their pool indices.
    std::vector<uint64_t> mark;                    // One bit per pool light, all clear between records.
    std::vector<uint32_t> cand;                    // A record's candidates (a run's scratch).
};
// The dynamic-light harvest, part 1: the CB shadow's clocks (cbs_gen, cbs_wseq; the table itself, g_cbs, and the want
// index, g_dl_keep_ix, stay cache-line aligned on their own), the cell grid, the want gate (keep_*), the harvest's
// cycle, the presence memo and the sampled draws.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: cell, idle.
// Session reset: session_globals_reset resets 10 of them.
struct DlCaptureState {
    // DL_ORIGIN_GRID - the engine's light-list origins sit on whole cells of the terrain's land grid
    // (getTerrainInfo's land_grid_width) at a multiple of it that the map decides: 4 x 25 m = 100 m on Malden, 4 x 32 m
    // = 128 m on cytech_underground_terrain (measured). Nothing is assumed per map: g_dlcap.cell is the terrain's land
    // grid width, staged on the game thread once per session (stage_world_lighting; 0 until read), and the strong
    // routes - the camera matrix (route 0), matrix x world pairs (route 1) and route 1's memoised hint - accept an
    // origin on it. Every origin they commit teaches the multiple (g_dlcap.q_cells: the greatest common divisor of the
    // committed origins' cell counts), and the weaker tests - bare triples (routes 2 / 3 and their hints), the anchor
    // vote, the re-sight fallback and the anchor ghost test - use that learned grid and stand down until one is
    // learned.
    std::atomic<float> cell{ 0.0f };   // Game thread writes, once per session; the render thread reads.
    uint32_t q_cells = 0;              // Render thread (or a park): the learned multiple, 0 = none yet.
    // Two records in one list are two lights: one uploaded list never carries the same light twice, so each record of a
    // list claims a slot no earlier record of that list has claimed. Not a property compare (an animated fire changes
    // colour every upload and must still re-sight its own slot).
    uint32_t pool_next_id = 1;   // DL_POOL_ID: next id to mint; never 0.
    uint32_t   cbs_gen = 0;    // The global write counter (the generation; the LRU takes wseq, below).
    uint64_t   cbs_wseq = 0;   // DL_REC_ORDER: the capture clock (one tick per cbs_write; never wraps).
    std::atomic<uint32_t> idle{ 0 };   // Harvests in a row with an empty pool (the harvest writes, the hooks read).
    const void* keep_id[DL_WANT_N];
    uint64_t    keep_cycle[DL_WANT_N];
    uint8_t     keep_kind[DL_WANT_N];   // PROF_FUNNEL: the roles the sampler named it in (armed only).
    uint32_t    keep_n = 0u;
    // DL_FALLBACK_HARVEST: the cycle (g_topo_cycles) whose span the pool last merged. The injection harvests an
    // accepted cycle (DL_TRIGGER_HARVEST: its trigger, when it has nothing to draw); the render-thread flush
    // harvests any cycle the injection did not, so the pool never goes stale while meshes still draw (fill_dynlights_cb
    // drops every light of a pool older than DL_POOL_STALE_MS). Render thread.
    uint64_t harvest_cycle = ~0ull;
    // DL_SPAN_BRIDGE: the last harvest was the flush's (at the scene resolve), not the injection's (at the trigger).
    // Presence assumes trigger-to-trigger spans; the span from a flush harvest to the next injection's misses the
    // frame's post-trigger draws, so that one injected harvest also keeps the lights the flush's span saw. Render
    // thread.
    bool harvest_flush_last = false;
    DlPresMemo pmemo[DL_PMEMO_N];
    // Bumped by every writer of what the match reads beyond the key (the pool and the unplaced notes): the harvest
    // (dynlights_harvest_arena), the resets (dl_pmemo_clear) and a presence rewrite that changes a record's bytes.
    // Never reset: it only moves on, and an entry matches only the value it was stored at.
    uint64_t pres_epoch = 1;
    // DL_STALL_GAP - a re-sight whose gap spans a stall of the harvest itself (the map screen, a loading screen, a
    // savegame write, alt-tab: harvests DL_POOL_STALE_MS or more apart) is no evidence of the light's cadence -
    // "a stall, not a frame" (DL_POOL_STALE_MS) - so its mark keeps its value, as an out-of-era stamp's does.
    // Taken as a cadence, the stall sets every re-sighted light's TTL to DL_TTL_CAP_MS for ~0.5 s, and a light
    // that dies then keeps lighting our meshes (and the fog) for up to 1.5 s. The stall the merge last saw is kept
    // ([pool stamp before it, the merge after it]); a sighting before this harvest's merge tests the gap since
    // the last merge itself. Render thread (the harvest's).
    uint64_t stall_a = 0;   // The pool stamp before the last stall.
    uint64_t stall_b = 0;   // The merge that ended it (0 = none seen).
    DlPresHix phix;
};
static DlCaptureState g_dlcap;
// A rebuild's cost in walked pairs (the unit of DL_HIX_DEBT): n (log2 n + 2) steps of DL_HIX_STEP_PAIRS -
// measured (g++ / clang -O2, pools of 64 - 1024): a rebuild took the time of 3.3 - 5.2 x n (log2 n + 2) pairs the
// band turns away (the cheapest pair, so the debt is never repaid early).
static constexpr uint64_t DL_HIX_STEP_PAIRS = 4u;
}