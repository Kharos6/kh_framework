#pragma once

#define NOMINMAX
#define STB_IMAGE_IMPLEMENTATION
// stb refuses a picture wider or taller than the renderer's cap (16384, the texture loaders' own
// test) from its header, before it allocates - its default allows 2^24 a side, so a 20-byte .tga claiming
// 23170 x 23170 had stb allocate and fill ~2 GB before the cap refused the result.
#define STBI_MAX_DIMENSIONS 16384
#define FD_SETSIZE 256

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <winhttp.h>
#include <windows.h>
#include <windowsx.h>
#include <Winternl.h>
#include <string>
#include <vector>
#include <array>
#include <regex>
#include <random>
#include <deque>
#include <list>
#include <functional>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <future>
#include <optional>
#include <unordered_set>
#include <stdexcept>
#include <unordered_map>
#include <shlobj.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <wincrypt.h>
#include <delayimp.h>
#include <mmeapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <propsys.h>
#include <gdiplus.h>
#include <dxgi.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <dwrite.h>
#include <ksmedia.h>
#include <d3dcompiler.h>
#include <d3d11_1.h>
#include <intrin.h>
#include <smmintrin.h>

#include "intercept/include/intercept.hpp"
#include "intercept/include/client/sqf/sqf.hpp"
#include "intercept/include/client/pointers.hpp"
#include "sol/sol.hpp"
#include "sherpa/include/c-api.h"
#include "llama/include/llama.h"
#include "llama/include/common.h"
#include "ultralight/include/Ultralight/Ultralight.h"
#include "minhook/include/MinHook.h"
#include "lz4/include/lz4.h"
#include "rendering/stb_image.h"
#include "rendering/ufbx.c"        // pulls in ufbx.h itself
#include "rendering/mikktspace.c"  // pulls in mikktspace.h itself

using namespace intercept;
using namespace intercept::types;

constexpr float PI = 3.14159265359f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float EPSILON = 0.0001f;
constexpr float YAW_WINDOW = 0.1f;    // 100ms measurement window
constexpr const int YAW_MAXSAMPLES = 32;  // ring capacity (enough for very high fps over 100ms)

// Lower / upper-cased copies (ASCII; each char passed unsigned, as the C functions require).
static std::string kh_lower_copy(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

static std::string kh_upper_copy(std::string value) {
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}

static game_value trigger_cba_event_sqf(game_value_parameter params);
static code g_compiled_sqf_generic_call;
static code g_compiled_sqf_generic_call_args;
static code g_compiled_kh_set_variable_generic;
static code g_compiled_kh_cba_local_event;
static code g_compiled_kh_cba_server_event;
static code g_compiled_kh_cba_owner_event;
static code g_compiled_kh_cba_target_event;
static code g_compiled_kh_cba_group_owner_dispatch;
static code g_compiled_kh_cba_array_target_dispatch;
static code g_compiled_kh_cba_code_target_dispatch;
static code g_compiled_kh_cba_callback_receiver;
static code g_compiled_kh_cba_callback_predicate;
static code g_compiled_sqf_add_game_event_handler;
static code g_compiled_sqf_remove_game_event_handler;
static code g_compiled_sqf_game_event_handler_lua_bridge;
static code g_compiled_sqf_execute_lua;
static code g_compiled_sqf_remove_handler;
static code g_compiled_sqf_create_hash_map_from_array;
static code g_compiled_sqf_trigger_lua_reset_event;
static code g_compiled_ai_initialized_event;
static code g_compiled_ai_response_progress_event;
static code g_compiled_ai_response_event;
static code g_compiled_tts_generated_event;
static code g_compiled_tts_finished_event;
static code g_compiled_stt_transcription_event;
static code g_compiled_html_js_event;
static code g_compiled_kh_empty_code;
static code g_compiled_kh_subfunction_basic;
static code g_compiled_kh_subfunction_process;
static code g_compiled_kh_monitor_set;
static code g_compiled_kh_monitor_delete;
static code g_compiled_kh_monitor_wrapper_scalar;
static code g_compiled_kh_monitor_wrapper_code;
static code g_compiled_kh_handler_scalar_iteration;
static code g_compiled_kh_handler_scalar;
static code g_compiled_kh_handler_timeout;
static code g_compiled_kh_handler_code_iteration_hard_fail;
static code g_compiled_kh_handler_code_iteration_soft_fail;
static code g_compiled_kh_handler_code_iteration;
static code g_compiled_kh_handler_code_hard_fail;
static code g_compiled_kh_handler_code;
static code g_compiled_kh_handler_string;
static code g_compiled_kh_callback_handler;
static code g_compiled_kh_persistent_marker;
static code g_compiled_kh_immediate_call;
static code g_compiled_kh_ui_render_init;
static game_value g_return_value;
static game_value g_call_arguments;
static game_value g_kh_cached_entity_initializations;
static game_value g_kh_cached_entity_initializations_deletions;
static bool g_is_menu = true;
static bool g_is_eden = false;
static bool g_is_server = false;
static bool g_is_dedicated_server = false;
static bool g_is_headless = false;
static bool g_is_player = false;
// Closed (DB, the user: a non-issue): g_game_time / g_mission_time are float sums of diag_deltaTime, so past
// ~18 h at 240 fps one frame's step rounds (the clock runs fast, then stops at ~36 h).
static float g_game_time = 0.0f;
static int g_game_frame = 0;
static float g_mission_time = 0.0f;
static int g_mission_frame = 0;
static std::vector<std::vector<float>> g_terrain_matrix;
static float g_terrain_grid_width = 0.0f;
static float g_world_size = 0.0f;
static float g_last_ts_connect_attempt = -1.0f;

enum class GPUBackend {
    CPU = 0,
    CUDA = 1,
    VULKAN = 2
};

static std::atomic<GPUBackend> g_active_backend{GPUBackend::CPU};
static bool g_has_cuda = false;
static bool g_has_vulkan = false;

inline bool g_gpu_available() {
    return g_active_backend != GPUBackend::CPU;
}

inline std::string get_backend_name() {
    switch (g_active_backend) {
        case GPUBackend::CUDA: return "CUDA";
        case GPUBackend::VULKAN: return "Vulkan";
        default: return "CPU";
    }
}

struct unit_states {
    float heading[YAW_MAXSAMPLES];
    float time[YAW_MAXSAMPLES];
    int head = 0;
    int count = 0;
    bool seen_this_frame = false;
};

static std::unordered_map<void*, unit_states> g_unit_states;
static std::atomic<bool> g_minhook_initialized{false};
static std::mutex g_minhook_mutex;

static bool ensure_minhook() {
    if (g_minhook_initialized.load(std::memory_order_acquire)) return true;
    std::lock_guard<std::mutex> lock(g_minhook_mutex);
    if (g_minhook_initialized.load(std::memory_order_acquire)) return true;
    if (MH_Initialize() != MH_OK) return false;
    g_minhook_initialized.store(true, std::memory_order_release);
    return true;
}

// IDXGISwapChain::Present is one function per DXGI
// implementation and MinHook takes one hook per target, so a second module's
// MH_CreateHook on it fails (MH_ERROR_ALREADY_CREATED) and one module's
// MH_RemoveHook would take the other's hook with it. Every module subscribes
// here instead. kh_present_hook detours each distinct target a module resolved
// (at most KH_PRESENT_TARGETS; repeat calls are no-ops) and never removes it
// before MH_Uninitialize; the detour calls the subscribers in slot order (none
// on a DXGI_PRESENT_TEST call), then the original once. Slots: the renderer
// first (its UI passes composite onto the engine's frame), then the HTML
// overlay (UIFramework, drawn on top). A
// subscriber must not throw. Clearing a slot stops later calls; one already
// made may still be running, so a module that clears its slot keeps its own
// in-flight guard. The renderer never clears its slot: its subscriber stays
// until MH_Uninitialize, which DllMain runs after the renderer's teardown.
// The original runs under UIFramework's long-standing rule: a structured
// exception inside it (D3D teardown) reads as S_OK.
// The renderer hooks the game's immediate context, and
// another module's present-time draws on it are not the engine's: the
// dispatcher calls the begin / end pair the renderer registered around every
// subscriber but the renderer's own, and the renderer excludes those draws as
// it excludes its own present-time draws.
enum KhPresentSlot : int { KH_PRESENT_SLOT_RENDER = 0, KH_PRESENT_SLOT_UI = 1, KH_PRESENT_SLOTS = 2 };
typedef void (*KhSharedPresentCb)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT (STDMETHODCALLTYPE* KhSharedPresentFn)(IDXGISwapChain*, UINT, UINT);
static std::atomic<KhSharedPresentCb> g_kh_present_cb[KH_PRESENT_SLOTS] = {};
static std::atomic<int (*)()> g_kh_overlay_begin{nullptr};
static std::atomic<void (*)(int)> g_kh_overlay_end{nullptr};
static constexpr int KH_PRESENT_TARGETS = 2;
static void* g_kh_present_target[KH_PRESENT_TARGETS] = {};   // Under g_kh_present_mu.
static KhSharedPresentFn g_kh_present_orig[KH_PRESENT_TARGETS] = {};   // Set before the target is enabled.
static std::mutex g_kh_present_mu;

static void kh_present_dispatch(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
    for (int slot = 0; slot < KH_PRESENT_SLOTS; ++slot) {
        const KhSharedPresentCb callback = g_kh_present_cb[slot].load(std::memory_order_acquire);
        if (!callback) continue;
        int (*const overlay_begin)() = slot != KH_PRESENT_SLOT_RENDER
                                           ? g_kh_overlay_begin.load(std::memory_order_acquire) : nullptr;
        void (*const overlay_end)(int) = overlay_begin ? g_kh_overlay_end.load(std::memory_order_acquire) : nullptr;
        const bool bracketed = overlay_begin && overlay_end;
        const int token = bracketed ? overlay_begin() : 0;
        callback(swap_chain, sync_interval, flags);
        if (bracketed) overlay_end(token);
    }
}
template <int Target>
static HRESULT STDMETHODCALLTYPE kh_present_detour(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags) {
    // DXGI_PRESENT_TEST presents nothing and every subscriber draws, so none
    // runs on a test.
    if (!(flags & DXGI_PRESENT_TEST)) kh_present_dispatch(swap_chain, sync_interval, flags);
    const KhSharedPresentFn original = g_kh_present_orig[Target];
    if (!original) return E_FAIL;
    __try {
        return original(swap_chain, sync_interval, flags);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return S_OK;
    }
}
// MH_OK when address is detoured (now or before); -1 when MinHook is
// unavailable, -2 when every target slot is taken, else the failing MH_STATUS
// (a created hook that would not enable is removed again).
static int kh_present_hook(void* address) {
    if (!address) return -2;
    if (!ensure_minhook()) return -1;
    std::lock_guard<std::mutex> lock(g_kh_present_mu);
    for (int target = 0; target < KH_PRESENT_TARGETS; ++target) {
        if (g_kh_present_target[target] == address) return MH_OK;
    }
    for (int target = 0; target < KH_PRESENT_TARGETS; ++target) {
        if (g_kh_present_target[target]) continue;
        void* const detour = target == 0 ? reinterpret_cast<void*>(&kh_present_detour<0>)
                                           : reinterpret_cast<void*>(&kh_present_detour<1>);
        MH_STATUS status = MH_CreateHook(address, detour, reinterpret_cast<void**>(&g_kh_present_orig[target]));
        if (status != MH_OK) return static_cast<int>(status);
        status = MH_EnableHook(address);
        if (status != MH_OK) {
            MH_RemoveHook(address);
            g_kh_present_orig[target] = nullptr;
            return static_cast<int>(status);
        }
        g_kh_present_target[target] = address;
        return MH_OK;
    }
    return -2;
}
static void kh_present_subscribe(KhPresentSlot slot, KhSharedPresentCb callback) {
    g_kh_present_cb[slot].store(callback, std::memory_order_release);
}
static void kh_present_overlay_register(int (*begin)(), void (*end)(int)) {
    g_kh_overlay_end.store(end, std::memory_order_release);
    g_kh_overlay_begin.store(begin, std::memory_order_release);
}

// Detect explicitly dedicated server
static bool get_machine_is_server() {
    static bool initialized = false;
    static bool is_server = false;
    
    if (!initialized) {
        char module_name[MAX_PATH];

        if (GetModuleFileNameA(NULL, module_name, MAX_PATH) != 0) {
            std::string exe_name = kh_lower_copy(std::filesystem::path(module_name).filename().string());
            is_server = exe_name == "arma3server_x64.exe" || exe_name == "arma3server.exe";
        }

        initialized = true;
    }

    return is_server;
}

static float parse_number(const std::string& str) {
    try {
        return std::stof(str);
    } catch (...) {
        return 0.0f;
    }
}

static size_t hash_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return std::hash<std::string>{}(contents);
}

static void report_error(const std::string& error_message) {
    sqf::diag_log(error_message);
    sqf::throw_exception(error_message);
}

static game_value raw_call_sqf_native(const code& code_obj) noexcept {
    g_return_value = game_value();
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    game_value result = g_return_value;
    g_return_value = game_value();
    return result;
}

static game_value raw_call_sqf_args_native(const code& code_obj, const game_value& args) noexcept {
    g_return_value = game_value();
    g_call_arguments = args;
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    game_value result = g_return_value;
    g_return_value = game_value();
    return result;
}

static game_value raw_call_sqf_native_no_return(const code& code_obj) noexcept {
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    return game_value();
}

static game_value raw_call_sqf_args_native_no_return(const code& code_obj, const game_value& args) noexcept {
    auto game_state = (intercept::client::host::functions.get_engine_allocator())->gameState;
    static r_string args_name = "_khargs"sv;
    game_state->set_local_variable(args_name, args);
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    return game_value();
}

static float bezier_shape(float t, const std::vector<float>& interior) {
    const size_t n = interior.size() + 1;              // degree
    
    // binomial C(n,k) iteratively; n is tiny here
    float shaped = 0.0f;
    float c = 1.0f;                                    // C(n,0)

    for (size_t k = 0; k <= n; ++k) {
        const float p = (k == 0) ? 0.0f : (k == n ? 1.0f : interior[k - 1]);
        const float term = c * std::pow(1.0f - t, static_cast<float>(n - k)) * std::pow(t, static_cast<float>(k)) * p;
        shaped += term;
        // C(n,k+1) = C(n,k) * (n-k)/(k+1)
        c = c * static_cast<float>(n - k) / static_cast<float>(k + 1);
    }

    return shaped;
}

static float invert_bezier(float target, const std::vector<float>& interior) {
    float lo = 0.0f, hi = 1.0f;
    
    for (int i = 0; i < 40; ++i) {
        const float mid = 0.5f * (lo + hi);
        if (bezier_shape(mid, interior) < target) lo = mid; else hi = mid;
    }

    return 0.5f * (lo + hi);
}

static float invert_shape_numeric(float target, float (*shape)(float)) {
    // shapes here are monotonic on [0,1]; bisection is robust and cheap
    float lo = 0.0f, hi = 1.0f;

    for (int i = 0; i < 40; ++i) {
        const float mid = 0.5f * (lo + hi);
        if (shape(mid) < target) lo = mid; else hi = mid;
    }
    
    return 0.5f * (lo + hi);
}

// shape functions reused for numeric inversion
static float s_smootherstep(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

static float curve_shape(const std::string& curve, bool is_bezier, float t, const std::vector<float>& bezier_interior) {
    if (curve == "linear") return t;
    if (curve == "smoothstep") return t * t * (3.0f - 2.0f * t);
    if (curve == "smootherstep") return s_smootherstep(t);
    if (curve == "easein") return t * t;
    if (curve == "easeout") return t * (2.0f - t);
    if (curve == "sine") return 0.5f * (1.0f - std::cos(t * PI));
    if (curve == "exponentialin") return (t <= 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
    if (curve == "exponentialout")  return (t >= 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);

    if (curve == "circular") {
        if (t < 0.5f) return 0.5f * (1.0f - std::sqrt(1.0f - 4.0f * t * t));
        const float u = 2.0f * t - 2.0f;
        return 0.5f * (std::sqrt(1.0f - u * u) + 1.0f);
    }

    if (is_bezier) return bezier_shape(t, bezier_interior);
    return t;
}

static float curve_slope(const std::string& curve, bool is_bezier, float t, const std::vector<float>& bezier_interior) {
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    const float ln2 = 0.69314718056f;
    if (curve == "linear") return 1.0f;
    if (curve == "smoothstep") return 6.0f * t * (1.0f - t);
    if (curve == "smootherstep") return 30.0f * t * t * (1.0f - t) * (1.0f - t);
    if (curve == "easein") return 2.0f * t;
    if (curve == "easeout") return 2.0f - 2.0f * t;
    if (curve == "sine") return 0.5f * PI * std::sin(t * PI);
    if (curve == "exponentialin") return 10.0f * ln2 * std::pow(2.0f, 10.0f * (t - 1.0f));
    if (curve == "exponentialout") return 10.0f * ln2 * std::pow(2.0f, -10.0f * t);

    // circular and bezier: central finite difference (no clean/stable closed form near joins/arbitrary points)
    const float h = 0.001f;
    float p0 = t - h; if (p0 < 0.0f) p0 = 0.0f;
    float p1 = t + h; if (p1 > 1.0f) p1 = 1.0f;
    const float span = (p1 - p0); 
    const float denom = (span > 1e-6f) ? span : 1e-6f;
    const float f0 = curve_shape(curve, is_bezier, p0, bezier_interior);
    const float f1 = curve_shape(curve, is_bezier, p1, bezier_interior);
    return (f1 - f0) / denom;
}

static float curve_inverse_shape(const std::string& curve, bool is_bezier, float shaped, const std::vector<float>& bezier_interior) {
    float t;
    if (curve == "linear") t = shaped;
    else if (curve == "smoothstep") t = 0.5f - std::sin(std::asin(1.0f - 2.0f * shaped) / 3.0f);
    else if (curve == "smootherstep") t = invert_shape_numeric(shaped, s_smootherstep);
    else if (curve == "easein") t = (shaped <= 0.0f) ? 0.0f : std::sqrt(shaped);
    else if (curve == "easeout") t = 1.0f - std::sqrt(1.0f - shaped);
    else if (curve == "sine") t = std::acos(1.0f - 2.0f * shaped) / PI;
    else if (curve == "exponentialin") t = (shaped <= 0.0f) ? 0.0f : 1.0f + std::log2(shaped) / 10.0f;
    else if (curve == "exponentialout") t = (shaped >= 1.0f) ? 1.0f : -std::log2(1.0f - shaped) / 10.0f;

    else if (curve == "circular") {
        if (shaped < 0.5f) { const float v = 1.0f - 2.0f * shaped; t = 0.5f * std::sqrt(1.0f - v * v); }
        else { const float v = 2.0f * shaped - 1.0f; t = 1.0f - 0.5f * std::sqrt(1.0f - v * v); }
    }
    
    else if (is_bezier) t = invert_bezier(shaped, bezier_interior);
    else t = shaped;
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    return t;
}

static std::vector<float> read_bezier_interior(const game_value& slot) {
    std::vector<float> interior;

    if (slot.type_enum() == game_data_type::ARRAY) {
        auto& pts = slot.to_array();
        interior.reserve(pts.size());
        for (size_t i = 0; i < pts.size(); ++i) interior.push_back(static_cast<float>(pts[i]));
    }
    
    if (interior.empty()) { interior.push_back(0.0f); interior.push_back(1.0f); } // default -> classic cubic
    return interior;
}

// Replicates SQF params/param semantics: nil, missing, or type-mismatched values yield the default. Empty type list accepts anything.
static game_value kh_param(const auto_array<game_value>& arr, size_t index, game_value default_value, std::initializer_list<game_data_type> allowed_types = {}) {
    if (index >= arr.size()) return default_value;
    const game_value& value = arr[index];
    if (value.is_nil()) return default_value;

    if (allowed_types.size() > 0) {
        const game_data_type type = value.type_enum();

        for (game_data_type allowed : allowed_types) {
            if (type == allowed) return value;
        }

        return default_value;
    }

    return value;
}

// kh_param for one type, as the value: a SCALAR / BOOL / STRING element, else the default.
static float kh_param_float(const auto_array<game_value>& arr, size_t index, float default_value) {
    return static_cast<float>(kh_param(arr, index, game_value(default_value), { game_data_type::SCALAR }));
}

static bool kh_param_bool(const auto_array<game_value>& arr, size_t index, bool default_value) {
    return static_cast<bool>(kh_param(arr, index, game_value(default_value), { game_data_type::BOOL }));
}

static std::string kh_param_string(const auto_array<game_value>& arr, size_t index, const std::string& default_value) {
    return static_cast<std::string>(kh_param(arr, index, game_value(default_value), { game_data_type::STRING }));
}

// The effects from arr[start_index] on (ttsSpeak, ttsUpdateSpeaker, tsApplyVoiceEffects): each element a [name, value]
// pair, or one element holding the whole chain ([[name, value], ...] - the shape the SQF functions pass, as one
// argument); anything else is skipped.
static std::vector<std::pair<std::string, float>> kh_parse_effects(const auto_array<game_value>& arr,
                                                                   size_t start_index) {
    std::vector<std::pair<std::string, float>> effects;

    auto add_pair = [&effects](const game_value& pair) {
        if (pair.type_enum() != game_data_type::ARRAY) return;
        auto& effect_arr = pair.to_array();

        if (effect_arr.size() >= 2 && effect_arr[0].type_enum() == game_data_type::STRING &&
            effect_arr[1].type_enum() == game_data_type::SCALAR) {
            effects.emplace_back(static_cast<std::string>(effect_arr[0]), static_cast<float>(effect_arr[1]));
        }
    };

    for (size_t i = start_index; i < arr.size(); i++) {
        if (arr[i].type_enum() != game_data_type::ARRAY) continue;
        auto& element = arr[i].to_array();

        if (!element.empty() && element[0].type_enum() == game_data_type::ARRAY) {   // A chain in one element.
            for (const game_value& pair : element) add_pair(pair);
        } else {
            add_pair(arr[i]);
        }
    }

    return effects;
}

static game_value kh_make_array(std::initializer_list<game_value> values) {
    auto_array<game_value> arr;
    arr.reserve(values.size());

    for (const game_value& value : values) {
        arr.push_back(value);
    }

    return game_value(std::move(arr));
}

class RandomStringGenerator {
private:
    static std::mt19937& get_rng() {
        static std::mt19937 gen(std::random_device{}());
        return gen;
    }
    
public:
    static std::string generate(int length, bool use_numbers = true, 
                                bool use_letters = true, bool use_symbols = false) {
        if (length <= 0) return "";
        std::string charset;
        if (use_numbers) charset += "0123456789";
        if (use_letters) charset += "abcdefghijklmnopqrstuvwxyz";
        if (use_symbols) charset += "!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";
        
        if (charset.empty()) {
            charset = "0123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";
        }
        
        std::uniform_int_distribution<> dis(0, static_cast<int>(charset.size() - 1));
        std::string result;
        result.reserve(length);
        auto& gen = get_rng();

        for (int i = 0; i < length; i++) {
            result += charset[dis(gen)];
        }
        
        return result;
    }
};

class UIDGenerator {
private:
    static std::atomic<uint32_t> counter;
    static thread_local std::mt19937 rng;
    static thread_local bool rng_seeded;
    static std::once_flag init_flag;
    static uint32_t unique_machine_id;
    static constexpr char hex_chars[] = "0123456789abcdef";
    
    static void to_hex(char* buffer, uint32_t value) {
        for (int i = 7; i >= 0; --i) {
            buffer[i] = hex_chars[value & 0xF];
            value >>= 4;
        }
    }
    
    static void initialize_machine_id() {
        uint32_t pid = GetCurrentProcessId();
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        uint32_t name_hash = 0;

        if (GetComputerNameA(computerName, &size)) {
            for (DWORD i = 0; i < size; ++i) {
                name_hash = name_hash * 31 + computerName[i];
            }
        }
        
        unique_machine_id = (pid ^ name_hash ^ (name_hash >> 16));
    }
    
public:
    static std::string generate() {
        std::call_once(init_flag, initialize_machine_id);

        if (!rng_seeded) {
            rng.seed(std::random_device{}() ^ unique_machine_id);
            rng_seeded = true;
        }
        
        uint32_t timestamp = static_cast<uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );

        uint32_t part1 = (timestamp & 0xFFFF) | ((unique_machine_id & 0xFF) << 16) | ((rng() & 0xFF) << 24);
        uint32_t part2 = counter.fetch_add(1, std::memory_order_relaxed);
        char buffer[17];
        to_hex(buffer, part1);
        to_hex(buffer + 8, part2);
        buffer[16] = '\0';
        return std::string(buffer, 16);
    }
};

std::atomic<uint32_t> UIDGenerator::counter{0};
thread_local std::mt19937 UIDGenerator::rng;
thread_local bool UIDGenerator::rng_seeded = false;
std::once_flag UIDGenerator::init_flag;
uint32_t UIDGenerator::unique_machine_id = 0;

class MainThreadScheduler {
private:
    MainThreadScheduler() = default;
    ~MainThreadScheduler() = default;
    MainThreadScheduler(const MainThreadScheduler&) = delete;
    MainThreadScheduler& operator=(const MainThreadScheduler&) = delete;
    std::deque<std::function<void()>> pending_commands;
    std::mutex queue_mutex;

public:
    // Never destroyed, as the framework singletons are not: nothing it holds needs releasing at exit, and a heap
    // instance cannot be touched by a destructor that ran before some last caller.
    static MainThreadScheduler& instance() {
        static MainThreadScheduler* inst = new MainThreadScheduler();
        return *inst;
    }

    void schedule(std::function<void()> command) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_commands.push_back(std::move(command));
    }

    void process_frame() {
        std::vector<std::function<void()>> commands_to_execute;

        {
            std::lock_guard<std::mutex> lock(queue_mutex);

            if (pending_commands.empty()) {
                return;
            }

            commands_to_execute.reserve(pending_commands.size());

            for (auto& cmd : pending_commands) {
                commands_to_execute.push_back(std::move(cmd));
            }

            pending_commands.clear();
        }

        for (auto& cmd : commands_to_execute) {
            try {
                cmd();
            } catch (const std::exception& e) {
                sqf::diag_log("KH Framework: scheduled command failed: " + std::string(e.what()));
            } catch (...) {
                sqf::diag_log("KH Framework: scheduled command failed: unknown exception");
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_commands.clear();
    }

    // The queue moved out (game thread), to survive a clear(); restore() puts it back in front of whatever was
    // scheduled since.
    std::deque<std::function<void()>> take() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return std::move(pending_commands);
    }

    void restore(std::deque<std::function<void()>>&& commands) {
        if (commands.empty()) return;
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_commands.insert(pending_commands.begin(), std::make_move_iterator(commands.begin()),
                                std::make_move_iterator(commands.end()));
    }
};

static std::mutex g_err_once_mutex;
static std::vector<std::string> g_err_once;
// The 256-message cap was reached this session and said so (under g_err_once_mutex; cleared
// with g_err_once by the renderer's session reset).
static bool g_err_once_capped = false;

static void report_error_once_safe(const std::string& msg) {
    std::string reported_msg;   // Built only for a message that is reported (a repeat costs no copy).
    {
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        if (std::find(g_err_once.begin(), g_err_once.end(), msg) != g_err_once.end()) return;
        if (g_err_once.size() >= 256) {
            // The first message past the cap is replaced by one saying later ones go unreported.
            if (g_err_once_capped) return;
            g_err_once_capped = true;
            reported_msg = "KH Framework: 256 distinct errors this session; later ones are not reported (" + msg + ")";
        } else {
            g_err_once.push_back(msg);
            reported_msg = msg;
        }
    }

    MainThreadScheduler::instance().schedule([reported_msg]() { report_error(reported_msg); });
}

template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t max_size) : max_size_(max_size) {
        if (max_size_ == 0) max_size_ = 1;
    }
    
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    
    LRUCache(LRUCache&& other) noexcept {
        std::unique_lock<std::shared_mutex> lock(other.mutex_);
        max_size_ = other.max_size_;
        access_order_ = std::move(other.access_order_);
        cache_map_ = std::move(other.cache_map_);
    }
    
    LRUCache& operator=(LRUCache&& other) noexcept {
        if (this != &other) {
            std::unique_lock<std::shared_mutex> lock1(mutex_, std::defer_lock);
            std::unique_lock<std::shared_mutex> lock2(other.mutex_, std::defer_lock);
            std::lock(lock1, lock2);
            max_size_ = other.max_size_;
            access_order_ = std::move(other.access_order_);
            cache_map_ = std::move(other.cache_map_);
        }

        return *this;
    }
    
    std::optional<Value> get(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_map_.find(key);
        
        if (it == cache_map_.end()) {
            return std::nullopt;
        }
        
        // Move accessed item to front (most recently used)
        access_order_.splice(access_order_.begin(), access_order_, it->second.list_it);
        return it->second.value;
    }
    
    // Get without updating LRU order (for read-heavy scenarios)
    std::optional<Value> peek(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_map_.find(key);
        
        if (it == cache_map_.end()) {
            return std::nullopt;
        }
        
        return it->second.value;
    }
    
    void put(const Key& key, const Value& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            it->second.value = value;
            access_order_.splice(access_order_.begin(), access_order_, it->second.list_it);
            return;
        }
        
        // Evict least recently used if at capacity
        while (cache_map_.size() >= max_size_) {
            const Key& lru_key = access_order_.back();
            cache_map_.erase(lru_key);
            access_order_.pop_back();
        }
        
        // Insert new entry at front
        access_order_.push_front(key);
        cache_map_[key] = {value, access_order_.begin()};
    }
    
    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_map_.find(key) != cache_map_.end();
    }
    
    void remove(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            access_order_.erase(it->second.list_it);
            cache_map_.erase(it);
        }
    }
    
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_map_.clear();
        access_order_.clear();
    }
    
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_map_.size();
    }
    
    size_t max_size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return max_size_;
    }
    
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_map_.empty();
    }

private:
    struct CacheEntry {
        Value value;
        typename std::list<Key>::iterator list_it;
    };
    
    size_t max_size_;
    std::list<Key> access_order_;  // Front = most recent, Back = least recent
    std::unordered_map<Key, CacheEntry> cache_map_;
    mutable std::shared_mutex mutex_;
};

static std::string game_value_to_json(const game_value& val) {
    switch (val.type_enum()) {
        case game_data_type::SCALAR:
            return std::to_string(static_cast<float>(val));

        case game_data_type::BOOL:
            return static_cast<bool>(val) ? "true" : "false";

        case game_data_type::STRING: {
            std::string str = static_cast<std::string>(val);
            std::string escaped = "\"";

            for (char c : str) {
                switch (c) {
                    case '"': escaped += "\\\""; break;
                    case '\\': escaped += "\\\\"; break;
                    case '\n': escaped += "\\n"; break;
                    case '\r': escaped += "\\r"; break;
                    case '\t': escaped += "\\t"; break;
                    default: escaped += c;
                }
            }

            escaped += "\"";
            return escaped;
        }

        case game_data_type::ARRAY: {
            auto& arr = val.to_array();
            std::string result = "[";
            
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) result += ",";
                result += game_value_to_json(arr[i]);
            }
            
            result += "]";
            return result;
        }

        default:
            return "null";
    }
}

static game_value json_to_game_value(const std::string& json) {
    if (json.empty() || json == "undefined" || json == "null") {
        return game_value();
    }

    if (json == "true") return game_value(true);
    if (json == "false") return game_value(false);

    if (json.size() >= 2 && json.front() == '"' && json.back() == '"') {
        std::string result;
        result.reserve(json.size() - 2);
        const size_t end = json.size() - 1;

        // Four hex digits at i (i + 4 <= end), or -1.
        auto hex4 = [&](size_t i) -> int {
            if (i + 4 > end) return -1;
            int value = 0;

            for (size_t k = 0; k < 4; ++k) {
                const char h = json[i + k];
                int d;
                if (h >= '0' && h <= '9') d = h - '0';
                else if (h >= 'a' && h <= 'f') d = h - 'a' + 10;
                else if (h >= 'A' && h <= 'F') d = h - 'A' + 10;
                else return -1;
                value = (value << 4) | d;
            }

            return value;
        };
        auto push_utf8 = [&](uint32_t cp) {
            if (cp < 0x80) {
                result += static_cast<char>(cp);
            } else if (cp < 0x800) {
                result += static_cast<char>(0xC0 | (cp >> 6));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (cp >> 18));
                result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            }
        };

        for (size_t i = 1; i < end; ++i) {
            if (json[i] != '\\' || i + 1 >= end) {
                result += json[i];
                continue;
            }

            const char next = json[i + 1];

            switch (next) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '/': result += '/'; ++i; break;
                case 'b': result += '\b'; ++i; break;
                case 'f': result += '\f'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case 'u': {
                    const int cp = hex4(i + 2);
                    // Malformed: kept as written. \u0000 too: an embedded NUL ends the SQF string, the six
                    // characters do not.
                    if (cp <= 0) { result += json[i]; break; }
                    i += 5;

                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < end && json[i + 1] == '\\' && json[i + 2] == 'u') {
                        const int low = hex4(i + 3);

                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            push_utf8(0x10000 + ((static_cast<uint32_t>(cp) - 0xD800) << 10) +
                                      (static_cast<uint32_t>(low) - 0xDC00));
                            i += 6;
                            break;
                        }
                    }

                    push_utf8(static_cast<uint32_t>(cp));
                    break;
                }
                default: result += json[i]; break;
            }
        }

        return game_value(result);
    }

    if (json.front() == '[' && json.back() == ']') {
        auto_array<game_value> arr;
        std::string content = json.substr(1, json.size() - 2);
        
        if (content.empty()) {
            return game_value(std::move(arr));
        }
        
        size_t pos = 0;
        int depth = 0;
        size_t start = 0;
        bool in_string = false;
        
        while (pos < content.size()) {
            char c = content[pos];
            
            if (in_string) {
                if (c == '\\') { pos += 2; continue; }   // The escaped character is not a delimiter.
                if (c == '"') in_string = false;
            } else if (c == '"') {
                in_string = true;
            } else if (c == '[' || c == '{') {
                depth++;
            } else if (c == ']' || c == '}') {
                depth--;
            } else if (c == ',' && depth == 0) {
                arr.push_back(json_to_game_value(content.substr(start, pos - start)));
                start = pos + 1;
                while (start < content.size() && content[start] == ' ') start++;
                pos = start;
                continue;
            }

            pos++;
        }
        
        if (start < content.size()) {
            arr.push_back(json_to_game_value(content.substr(start)));
        }
        
        return game_value(std::move(arr));
    }

    try {
        size_t processed = 0;
        float num = std::stof(json, &processed);

        if (processed == json.size()) {
            return game_value(num);
        }
    } catch (...) {}
    
    // Fallback: return as string
    return game_value(json);
}

static void initialize_terrain_matrix() {
    bool building = false;   // The rows below are being replaced.

    try {
        static std::string cached_world;
        static float cached_grid_width = 0;
        std::string current_world = sqf::world_name();
        auto terrain_info = sqf::get_terrain_info();
        
        // Check if we need to recalculate
        if (current_world == cached_world && 
            terrain_info.terrain_grid_width == cached_grid_width &&
            !g_terrain_matrix.empty()) {
            return;
        }
        
        // Update cache keys
        cached_world = current_world;
        cached_grid_width = terrain_info.terrain_grid_width;
        
        // Store terrain info
        g_terrain_grid_width = terrain_info.terrain_grid_width;
        g_world_size = sqf::world_size();
        
        if (g_world_size <= 0 || g_terrain_grid_width <= 0) {
            return;
        }
        
        // Calculate grid dimensions
        int grid_points = static_cast<int>(g_world_size / g_terrain_grid_width) + 1;
        
        // Initialize matrix
        building = true;
        g_terrain_matrix.clear();
        g_terrain_matrix.reserve(grid_points);
        
        // Populate the matrix
        for (int y = 0; y < grid_points; y++) {
            std::vector<float> row;
            row.reserve(grid_points);
            float world_y = y * g_terrain_grid_width;
            
            for (int x = 0; x < grid_points; x++) {
                float world_x = x * g_terrain_grid_width;
                
                // Get terrain height at this position
                vector3 pos_atl(world_x, world_y, 0);
                vector3 pos_asl = sqf::atl_to_asl(pos_atl);
                float height = pos_asl.z;
                row.push_back(height);
            }
            
            g_terrain_matrix.push_back(std::move(row));
        }        
    } catch (const std::exception& e) {
        // A build that threw part-way (an allocation; the rows are ~67 MB on a large map)
        // left the cache keys naming this world with only its first rows - every later call returned early on
        // them. Emptied, the matrix is built again at the next call, as one that was never built.
        if (building) g_terrain_matrix.clear();
        report_error("getTerrainMatrix: failed to initialize the terrain matrix: " + std::string(e.what()));
    } catch (...) {
        if (building) g_terrain_matrix.clear();
        report_error("getTerrainMatrix: failed to initialize the terrain matrix: unknown error");
    }
}

class ShutdownWatchdog {
public:
    static ShutdownWatchdog& instance() {
        static ShutdownWatchdog inst;
        return inst;
    }

    void initialize() {
        if (InterlockedCompareExchange(&initialized_, 1, 0) == 1) return;
        
        thread_handle_ = CreateThread(
            nullptr, 
            0, 
            WatchdogThreadProc, 
            this, 
            0, 
            nullptr
        );
    }
    
    void shutdown(bool process_terminating = false) {
        InterlockedExchange(&armed_, 0);
        InterlockedExchange(&shutdown_thread_, 1);
        
        if (process_terminating) {
            return;
        }
        
        // Normal DLL unload - wait and cleanup
        if (thread_handle_ != nullptr) {
            if (WaitForSingleObject(thread_handle_, 150) == WAIT_TIMEOUT) {
                TerminateThread(thread_handle_, 0);
            }

            CloseHandle(thread_handle_);
            thread_handle_ = nullptr;
        }
    }

    void arm(DWORD timeout_ms = 3000) {
        if (InterlockedCompareExchange(&shutdown_thread_, 0, 0) == 1) return;
        timeout_ms_ = timeout_ms;
        arm_time_ = GetTickCount64();
        InterlockedExchange(&armed_, 1);
    }

    void disarm() {
        InterlockedExchange(&armed_, 0);
    }

    static void force_terminate(UINT exit_code = 0xDEAD0002) {
        TerminateProcess(GetCurrentProcess(), exit_code);
    }

private:
    static DWORD WINAPI WatchdogThreadProc(LPVOID lpParam) {
        ShutdownWatchdog* self = static_cast<ShutdownWatchdog*>(lpParam);
        
        while (InterlockedCompareExchange(&self->shutdown_thread_, 0, 0) == 0) {
            if (InterlockedCompareExchange(&self->armed_, 1, 1) == 1) {
                ULONGLONG elapsed = GetTickCount64() - self->arm_time_;
                
                if (elapsed >= self->timeout_ms_) {
                    TerminateProcess(GetCurrentProcess(), 0xDEAD0001);
                }
            }
            
            Sleep(50);
        }
        
        return 0;
    }

    ShutdownWatchdog() = default;
    ~ShutdownWatchdog() = default;
    ShutdownWatchdog(const ShutdownWatchdog&) = delete;
    ShutdownWatchdog& operator=(const ShutdownWatchdog&) = delete;
    HANDLE thread_handle_ = nullptr;
    volatile LONG initialized_ = 0;
    volatile LONG armed_ = 0;
    volatile LONG shutdown_thread_ = 0;
    volatile ULONGLONG arm_time_ = 0;
    volatile DWORD timeout_ms_ = 3000;
};