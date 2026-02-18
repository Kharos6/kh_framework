#pragma once

#define NOMINMAX
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
#include <regex>
#include <random>
#include <deque>
#include <functional>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdint>
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

using namespace intercept;
using namespace intercept::types;

static code g_compiled_sqf_trigger_cba_event;
static code g_compiled_sqf_add_game_event_handler;
static code g_compiled_sqf_remove_game_event_handler;
static code g_compiled_sqf_game_event_handler_lua_bridge;
static code g_compiled_sqf_execute_lua;
static code g_compiled_sqf_execute_sqf;
static code g_compiled_sqf_remove_handler;
static code g_compiled_sqf_create_hash_map_from_array;
static code g_compiled_sqf_create_hash_map;
static code g_compiled_sqf_trigger_lua_reset_event;
static code g_compiled_ai_initialized_event;
static code g_compiled_ai_response_progress_event;
static code g_compiled_ai_response_event;
static code g_compiled_tts_generated_event;
static code g_compiled_tts_finished_event;
static code g_compiled_stt_transcription_event;
static code g_compiled_html_js_event;
static game_value g_return_value;
static game_value g_call_arguments;
static bool g_is_menu = false;
static bool g_is_server = false;
static bool g_is_dedicated_server = false;
static bool g_is_headless = false;
static bool g_is_player = false;
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

bool g_gpu_available() {
    return g_active_backend != GPUBackend::CPU;
}

std::string get_backend_name() {
    switch (g_active_backend) {
        case GPUBackend::CUDA: return "CUDA";
        case GPUBackend::VULKAN: return "Vulkan";
        default: return "CPU";
    }
}

// Detect explicitly dedicated server
static bool get_machine_is_server() {
    static bool initialized = false;
    static bool is_server = false;
    
    if (!initialized) {
        char module_name[MAX_PATH];

        if (GetModuleFileNameA(NULL, module_name, MAX_PATH) != 0) {
            std::string exe_name = std::filesystem::path(module_name).filename().string();
            std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::tolower);
            is_server = exe_name == "arma3server_x64.exe" || exe_name == "arma3server.exe";
        }

        initialized = true;
    }

    return is_server;
}

static void report_error(const std::string& error_message) {
    sqf::diag_log(error_message);
    sqf::throw_exception(error_message);
}

static game_value raw_call_sqf_native(const code& code_obj) noexcept {
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    return g_return_value;
}

static game_value raw_call_sqf_args_native(const code& code_obj, const game_value& args) noexcept {
    g_call_arguments = args;
    intercept::client::host::functions.invoke_raw_unary(intercept::client::__sqf::unary__isnil__code_string__ret__bool, code_obj);
    return g_return_value;
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
    static MainThreadScheduler& instance() {
        static MainThreadScheduler inst;
        return inst;
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
            } catch (...) {
                // Command failed - continue with others
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_commands.clear();
    }
};

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

        case game_data_type::NOTHING:
            return "null";
            
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
        
        for (size_t i = 1; i < json.size() - 1; ++i) {
            if (json[i] == '\\' && i + 1 < json.size() - 1) {
                char next = json[i + 1];

                switch (next) {
                    case '"': result += '"'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    case 'n': result += '\n'; ++i; break;
                    case 'r': result += '\r'; ++i; break;
                    case 't': result += '\t'; ++i; break;
                    default: result += json[i]; break;
                }
            } else {
                result += json[i];
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
            
            if (c == '"' && (pos == 0 || content[pos - 1] != '\\')) {
                in_string = !in_string;
            } else if (!in_string) {
                if (c == '[' || c == '{') depth++;
                else if (c == ']' || c == '}') depth--;
                else if (c == ',' && depth == 0) {
                    arr.push_back(json_to_game_value(content.substr(start, pos - start)));
                    start = pos + 1;
                    while (start < content.size() && content[start] == ' ') start++;
                    pos = start;
                    continue;
                }
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
        report_error("Failed to initialize terrain matrix: " + std::string(e.what()));
    } catch (...) {
        report_error("Unknown error while initializing terrain matrix");
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
            WaitForSingleObject(thread_handle_, 1000);
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