#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the shadow
// locators: the colour probes, the sky binding, the ring best and the rescue window. Never include it on its own.
namespace RenderIntegration {

struct CbColorProbe {
    ID3D11Resource* buf = nullptr;   // Locked upload location (weak identity).
    uint32_t off = 0;   // Float index of the decay anchor lane.
    uint32_t floats = 0;   // CB size (floats) at lock time.
    uint32_t nb_base = 0;   // First float mirrored into nb[].
    int      meta = 0;   // Off - nb_base (40 when the full head fits).
    bool     valid = false;
    uint64_t hits = 0;   // Anchor confirmations (health: climbs steadily).
    float    last_confirm = 0.0f;   // Wall clock of the last confirmation (expiry).
    float    mirror_confirm = 0.0f;   // KH_DEC_MIRROR_CLOCK: last_confirm as of the last nb refresh by
                                      // kh_probe_mirror_refresh (the light probe's decay-lane freshness).
    float    nb[96] = {};   // Live block mirror (render thread writes; the draw paths read).
};
static CbColorProbe g_light_probe;

static CbColorProbe g_sky_probe;

// Two call sites: every 8th opaque engine draw (reorder_pre_draw) and the injection (before our own state touches
// slot 0). The plausible-size bound CBs are copied to staging and read back at a later call (DO_NOT_WAIT): PS slots
// while the sky mirror is stale, VS slots while the view is unlocked and never at the injection. Every copy read back
// feeds the same anchor and mirror as the upload path; VS copies also feed the view scan (while still unlocked).
struct SkyBindProbe {
    ID3D11Buffer* staging[16] = {};   // 0-7 = PS slots, 8-15 = VS slots.
    uint32_t      bytes[16] = {};
    ID3D11Buffer* src[16] = {};   // Source buffer (identity only, no ref).
    uint32_t      base_fl[16] = {};   // Window base offset in floats.
    uint16_t      pending = 0;   // Bitmask: staging[i] holds an unread copy.
};
static SkyBindProbe g_skybind;
// Runs only while unlocked; the slot is age-bounded so a stale matrix cannot
// sit there winning after the camera moved on.
static constexpr uint64_t KH_RING_BEST_MAX_CYCLES = 120ull;   // ~2 s at 60 fps.
static bool     g_ring_best_valid = false;
// For the whole cold-cache compile window: the shadow acquisition chain stands down while set, and the first
// gate-passed frame after it clears is treated as the session's frame one.
static std::atomic<bool> g_acq_hold{false};
static float    g_ring_best_er = 0.0f;   // Its score at admission.
static const void* g_ring_best_res = nullptr;
static uint32_t g_ring_best_off = 0;
static uint32_t g_ring_best_orient = 1;
static uint64_t g_ring_best_cycle = 0;
static float    g_ring_best_m[16] = {};
static float    g_pub_first = -1.0f;   // Cold publish-window epoch (session-reset).
// Identity keys, never dereferenced: no AddRef, no Release, compared only against g_ls.vc_ring[].res and nulled at
// session reset. If you need the contents behind one, take a reference at the point of use; do not promote the key.
static ID3D11Resource* g_prewarm_cand_res = nullptr;
static uint32_t g_prewarm_cand_off = 0;
static ID3D11Resource* g_coldlock_cand_res = nullptr;   // Injection cold-adoption
                                                        // double-confirmation memory.
static uint32_t g_coldlock_cand_off = 0;

static std::atomic<uint32_t> g_cc_flush_opaques{0xFFFFFFFFu};   // Sentinel = no flush yet.

// The injection's depth encode (projection m22 / m32 and its near) and its stamp, re-published by every proceeding
// attempt (inject_composited_meshes, below its skip returns): the flush and kh_svs_take_pv read the last painter's.
static float    g_inj_enc_m22 = 0.0f;
static float    g_inj_enc_m32 = 0.0f;
static float    g_inj_enc_near = -1.0f;
static uint64_t g_inj_enc_ms = 0;
static constexpr float KH_INJ_SLOT_NEAR_MIN = KH_CAM_NEAR_MIN;
// The flush serial the carry latch was last stored for: the flush stores g_carry_pending_serial once per serial.
static uint64_t g_ms_flush_serial = 0;

// Serial-matched carry latch: the flush stores its serial when it carries a missed frame; the rescue window is exactly
// that frame's post-flush tail, self-closing when the next flush ticks the serial.
static std::atomic<uint64_t> g_carry_pending_serial{0};
// After a rescue the injection re-arms (at most KH_RESCUE_MAX_PER_FRAME - 1 times a frame) while the serial-matched
// window is open, so a later repainting partition's own trigger repaints again; the window self-closes at the next
// flush and healthy frames never open it.
static constexpr uint32_t KH_RESCUE_MAX_PER_FRAME = 4;
static uint32_t g_rescue_frame_count = 0;   // Rescues this frame (boundary reset).

struct KhRt0DescEntry {
    void*    res = nullptr;
    uint32_t fmt = 0;   // DXGI_FORMAT bits; 0 = non-Texture2D verdict.
    UINT     w = 0;
};
static KhRt0DescEntry g_rt0_desc[32] = {};

static uint32_t g_rt0_desc_n = 0;
static uint32_t g_rt0_desc_next = 0;
}