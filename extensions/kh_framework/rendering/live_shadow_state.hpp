#pragma once

// The shared head (rendering_integration.hpp): the engine's live shadow cascades (LiveShadowState g_ls). Never include
// it on its own.
namespace RenderIntegration {

// Per frame, per cascade cycle: latch that matrix from the upload (locator-cached offset, rediscovery on miss),
// snapshot the bound viewport at the cycle's next draw as the cascade's atlas tile, and hand the table to the
// composited mesh shader, which performs the engine's own shadow compare for our pixels.
static constexpr uint32_t KH_LIVE_MAX_CASCADES = 8;

struct LiveShadowEntry {
    float    m[12];
    float    tile[4];
    float    cam[3];   // The camera origin this entry's transform is relative to (entries persist across frames, so
                       // each carries its own).
    uint64_t stamp;   // Finalize order; the newest entry is the atlas.
    float    time = 0.0f;   // Seal wall-clock (staleness expiry; see frame_reset).
};
struct LiveShadowState {
    std::atomic<bool> wanted{ false };   // Game thread: a visible lit mesh or a shadow-active one (the census).
    // Atlas tracking (render thread).
    void*    atlas_identity = nullptr;
    ID3D11Texture2D*          atlas_tex = nullptr;
    ID3D11ShaderResourceView* atlas_srv = nullptr;
    uint32_t atlas_size = 0;
    uint32_t atlas_fmt = 0;
    float    atlas_last_seen = 0.0f;   // Last t15 consumption of the held atlas (stamped in
                                       // mask_note_draw).
    float    first_capture_time = -1.0f;   // First band capture (receive warm-up gate).
    bool     receive_warmed = false;
    bool     srv_failed = false;
    bool     phase_on_atlas = false;
    bool     pending_valid = false;
    bool     cycle_latched = false;   // One distinct entry per cascade cycle.
    uint32_t pending_bytes = 0;   // source-CB size of the pending matrix.
    bool     pending_vp_valid = false;
    float    pending_vp[4] = {};   // topLeftX, TopLeftY, Width, Height.
    float    pending_m[12] = {};
    // An engine observation (no code decodes it): the small sampling block's second 4x3 is a pure camera-yaw rotation
    // R with translation t2 = (0.30, -1.59, 0.01) - an eye-height-shaped origin offset in yaw-rotated space.
    // KH_REG_SLICE: data holds floats [KH_REG_SLICE_LO, KH_REG_SLICE_LO + KH_REG_SLICE_N) of the upload, and only
    // when floats >= KH_REG_READ_MIN - the whole of what resolve_pair_capture / band_capture read; the rest is stale.
    struct CbRecord { void* buf = nullptr; uint32_t floats = 0; uint64_t seq = 0; float data[512]; };
    CbRecord cb_reg[16];
    uint32_t cb_reg_next = 0;
    uint64_t cb_reg_seq = 0;   // Recency: which record was written last.
    // Persistent cascade table: the engine re-renders (and re-uploads the transform for) a cascade only when it
    // needs updating, so a per-frame table is empty on most frames. Entries are keyed by scale (each cascade has a
    // distinct extent) and replaced in place. The engine's cascade CB exceeds the registry's size gate, so the
    // pass-scoped stash of the last window-shaped matrix is what holds it.
    LiveShadowEntry entries[KH_LIVE_MAX_CASCADES] = {};
    uint32_t count = 0;
    bool     wipe_deferred = false;   // The table was marked suspect; the first commit prunes it.
    uint64_t stamp_counter = 0;
    int      newest = -1;   // Entry index of the last finalize.
    struct BandSlot {
        bool  valid = false;
        bool  pending_view = false;
        // The provisional vcol came from the bridge fallback (no engine view published yet): a cross-convention
        // pairing the receive term must not consume. Cleared when the true same-frame view completes the seal.
        bool  vcol_bridge = false;
        float pending_since = 0.0f;   // When the seal went pending.
        float sm[12] = {};   // PSC_ShadowmapMatrix rows (float4x3).
        float vcol[12] = {};   // View matrix columns 0..2 (world->view, row-vector conv).
        float border[4] = {};   // x = near, y = far.
        float last_time = 0.0f;
        ID3D11Texture2D*          tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        // Deferred capture: the sm/border a sweep produced, held out of the live triple until the view that pairs
        // with it arrives. The slot keeps serving its previous complete seal meanwhile.
        bool     stage_valid = false;
        float    stage_sm[12] = {};
        float    stage_border[4] = {};
        float    stage_since = 0.0f;
        float    stage_view[12] = {};
        bool     stage_view_ok = false;
        // vcol layout (columns), like stage_view above.
        bool     stage_pair_ok = false;
        // The first exact native publish accepted after this band's own stage, no source filter and no boundary
        // guard.
        float    stage_first[12] = {};
        bool     stage_first_ok = false;
        int      stage_pool = -1;   // Claimed back buffer (-1 = none).
    };
    BandSlot band[8];   // All 8 engine cascades: full-range receive.
    bool     stage_any = false;   // Cheap per-draw gate for the commit.
    bool resolve_seen_since_cast = false;
    bool view_published_this_frame = false;   // Dual gate for the cast arm.
    float last_publish_rot_err = 1.0f;   // rot agreement of the last accepted publish.
    // A healthy lock re-proves exactness within the window continuously; a genuinely off-axis era never does and
    // starves - at most one window late.
    uint64_t pub_exact_ms = 0;   // 0 = no exact publication yet.
    uint64_t pub_fresh_ms = 0;
    float band_last_reject[4] = {};   // Last border quad rejected by sanity.
    uint32_t resolve_window = 0;   // Qualifying draws remaining after an atlas-pass exit.
    float frame_view[16] = {};
    bool  frame_view_valid = false;
    float frame_view_time = -1.0f;
    // Latched at the accept whenever the incoming publish crosses sources.
    float alt_view[16] = {};
    bool  alt_view_valid = false;
    float alt_view_time = -1.0f;
    struct ViewCand { ID3D11Resource* res; uint32_t off; float m[16]; };
    ViewCand vc_ring[16];
    uint32_t vc_n = 0;
    ID3D11Resource* view_src_res = nullptr;   // Learned location (weak key).
    uint32_t view_src_off = 0;
    int  view_src_orient = 0;   // 1 direct, 2 transposed.
    bool view_src_relative = false;   // Engine view is camera-relative (t ~ 0).
    bool view_src_valid = false;
    uint32_t pub_rej_streak = 0;   // Consecutive bridge-truth publish rejections.
    bool pub_ok_since_lock = false;   // This lock has passed at least one.
    uint32_t view_src_miss = 0;
    uint64_t view_locks = 0;
    // Exactness-preference wait: time of the first bar-passing but non-exact candidate deferral (0 = not waiting).
    float view_wait_since = 0.0f;
    float prev_view[16] = {};   // Previous accepted view (rotation extrapolation).
    bool  prev_view_valid = false;
    bool     cache_valid = false;
    uint32_t cache_bytes = 0;
    uint32_t cache_offset = 0;
    float    cam[3] = {};   // The frame's render camera (engine axes) recorded at injection - the
                            // shadow space origin.
};
static LiveShadowState g_ls;

// Flush serial (the scene flush: the render thread's, or the game thread's under a park): once-per-flush key for
// the miss latch.
static std::atomic<uint64_t> g_cc_flush_serial{0};   // Ticks once per scene flush (a chain-only run does not).
}