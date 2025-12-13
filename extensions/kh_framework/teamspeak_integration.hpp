#pragma once

using namespace intercept;
using namespace intercept::types;

// Shared memory structure for IPC between extension and TS3 plugin
#pragma pack(push, 1)
struct TSVoiceEffectConfig {
    uint32_t version;                    // Protocol version
    uint32_t sequence_number;            // For change detection
    uint8_t effects_enabled;             // Master enable flag
    uint8_t reserved[3];                 // Alignment padding

    // Ordered effect chain (for user-controlled application order)
    static constexpr uint8_t MAX_EFFECT_CHAIN = 32;
    uint8_t effect_chain_count;          // Number of effects in chain
    uint8_t effect_chain_types[MAX_EFFECT_CHAIN];   // Effect type IDs
    float effect_chain_values[MAX_EFFECT_CHAIN];    // Effect parameter values

    // Validation
    uint32_t checksum;                   // Simple validation
};

struct TSPluginStatus {
    uint32_t version;                    // Protocol version
    uint8_t plugin_active;               // Plugin is loaded and ready
    uint8_t capturing;                   // Currently capturing voice
    uint8_t transmitting;                // Currently transmitting
    uint8_t connected;                   // Connected to a server
    uint32_t sample_rate;                // Current sample rate
    uint32_t last_heartbeat;             // Tick count for liveness check
};
#pragma pack(pop)

enum class TSEffectType : uint8_t {
    NONE = 0,
    LOWPASS,
    HIGHPASS,
    DISTORTION,
    BITCRUSH,
    NOISE,
    RING_MOD,
    REVERB,
    ECHO_DELAY,
    ECHO_DECAY,
    VOLUME,
    RADIO_STATIC,
    RADIO_SQUELCH,
    FREQUENCY_WOBBLE,
    PITCH_SHIFT,
    TREMOLO_RATE,
    TREMOLO_DEPTH,
    COMPRESSOR_THRESHOLD,
    COMPRESSOR_RATIO,
    TELEPHONE,
    UNDERWATER,
    AGC,
    CHORUS,
    CHORUS_RATE,
    FLANGER,
    FLANGER_RATE,
    BASS_BOOST
};

static TSEffectType get_effect_type_from_name(const std::string& name) {
    if (name == "lowpass") return TSEffectType::LOWPASS;
    if (name == "highpass") return TSEffectType::HIGHPASS;
    if (name == "distortion") return TSEffectType::DISTORTION;
    if (name == "bitcrush") return TSEffectType::BITCRUSH;
    if (name == "noise") return TSEffectType::NOISE;
    if (name == "ring_mod") return TSEffectType::RING_MOD;
    if (name == "reverb") return TSEffectType::REVERB;
    if (name == "echo_delay") return TSEffectType::ECHO_DELAY;
    if (name == "echo_decay") return TSEffectType::ECHO_DECAY;
    if (name == "volume") return TSEffectType::VOLUME;
    if (name == "radio_static") return TSEffectType::RADIO_STATIC;
    if (name == "radio_squelch") return TSEffectType::RADIO_SQUELCH;
    if (name == "frequency_wobble") return TSEffectType::FREQUENCY_WOBBLE;
    if (name == "pitch_shift" || name == "pitch") return TSEffectType::PITCH_SHIFT;
    if (name == "tremolo_rate") return TSEffectType::TREMOLO_RATE;
    if (name == "tremolo_depth") return TSEffectType::TREMOLO_DEPTH;
    if (name == "compressor_threshold" || name == "compress_thresh") return TSEffectType::COMPRESSOR_THRESHOLD;
    if (name == "compressor_ratio" || name == "compress_ratio") return TSEffectType::COMPRESSOR_RATIO;
    if (name == "telephone") return TSEffectType::TELEPHONE;
    if (name == "underwater") return TSEffectType::UNDERWATER;
    if (name == "agc") return TSEffectType::AGC;
    if (name == "chorus") return TSEffectType::CHORUS;
    if (name == "chorus_rate") return TSEffectType::CHORUS_RATE;
    if (name == "flanger") return TSEffectType::FLANGER;
    if (name == "flanger_rate") return TSEffectType::FLANGER_RATE;
    if (name == "bass_boost" || name == "bass") return TSEffectType::BASS_BOOST;
    return TSEffectType::NONE;
}

// Note: TREMOLO_DEPTH, ECHO_DECAY, CHORUS_RATE, FLANGER_RATE, COMPRESSOR_RATIO
// are modifier parameters that affect their parent effects (TREMOLO_RATE, ECHO_DELAY, 
// CHORUS, FLANGER, COMPRESSOR_THRESHOLD respectively). They don't produce effects alone.

constexpr uint32_t TS_IPC_VERSION = 1;
constexpr const char* TS_SHARED_MEMORY_NAME = "KH_TeamSpeak_VoiceEffects";
constexpr const char* TS_STATUS_MEMORY_NAME = "KH_TeamSpeak_Status";
constexpr const char* TS_MUTEX_NAME = "KH_TeamSpeak_Mutex";
constexpr size_t TS_SHARED_MEMORY_SIZE = sizeof(TSVoiceEffectConfig);
constexpr size_t TS_STATUS_MEMORY_SIZE = sizeof(TSPluginStatus);

class TeamspeakFramework {
private:
    TeamspeakFramework() = default;
    ~TeamspeakFramework() { cleanup(); }
    TeamspeakFramework(const TeamspeakFramework&) = delete;
    TeamspeakFramework& operator=(const TeamspeakFramework&) = delete;
    HANDLE shared_memory_handle = nullptr;
    HANDLE status_memory_handle = nullptr;
    HANDLE mutex_handle = nullptr;
    TSVoiceEffectConfig* effect_config = nullptr;
    TSPluginStatus* plugin_status = nullptr;
    std::atomic<bool> is_initialized_flag{false};
    std::atomic<uint32_t> sequence_counter{0};
    mutable std::mutex ipc_mutex;
    std::thread heartbeat_thread;
    std::atomic<bool> heartbeat_running{false};
    
    // Current effects state
    std::vector<std::pair<std::string, float>> current_effects;
    std::mutex effects_mutex;
    
    static uint32_t calculate_checksum(const TSVoiceEffectConfig* config) {
        uint32_t sum = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(config);
        size_t size = offsetof(TSVoiceEffectConfig, checksum);
        
        for (size_t i = 0; i < size; i++) {
            sum = (sum << 1) | (sum >> 31);
            sum ^= data[i];
        }
        
        return sum;
    }
    
    bool create_shared_memory() {
        // Create mutex for synchronization
        mutex_handle = CreateMutexA(nullptr, FALSE, TS_MUTEX_NAME);
        
        if (mutex_handle == nullptr) {
            report_error("KH - TeamSpeak: Failed to create mutex");
            return false;
        }
        
        // Create shared memory for effect configuration
        shared_memory_handle = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(TS_SHARED_MEMORY_SIZE),
            TS_SHARED_MEMORY_NAME
        );
        
        if (shared_memory_handle == nullptr) {
            report_error("KH - TeamSpeak: Failed to create shared memory for effects");
            CloseHandle(mutex_handle);
            mutex_handle = nullptr;
            return false;
        }
        
        effect_config = static_cast<TSVoiceEffectConfig*>(
            MapViewOfFile(shared_memory_handle, FILE_MAP_ALL_ACCESS, 0, 0, TS_SHARED_MEMORY_SIZE)
        );
        
        if (effect_config == nullptr) {
            report_error("KH - TeamSpeak: Failed to map shared memory for effects");
            CloseHandle(shared_memory_handle);
            shared_memory_handle = nullptr;
            CloseHandle(mutex_handle);
            mutex_handle = nullptr;
            return false;
        }
        
        // Create shared memory for status
        status_memory_handle = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(TS_STATUS_MEMORY_SIZE),
            TS_STATUS_MEMORY_NAME
        );
        
        if (status_memory_handle == nullptr) {
            report_error("KH - TeamSpeak: Failed to create shared memory for status");
            UnmapViewOfFile(effect_config);
            effect_config = nullptr;
            CloseHandle(shared_memory_handle);
            shared_memory_handle = nullptr;
            CloseHandle(mutex_handle);
            mutex_handle = nullptr;
            return false;
        }
        
        plugin_status = static_cast<TSPluginStatus*>(
            MapViewOfFile(status_memory_handle, FILE_MAP_ALL_ACCESS, 0, 0, TS_STATUS_MEMORY_SIZE)
        );
        
        if (plugin_status == nullptr) {
            report_error("KH - TeamSpeak: Failed to map shared memory for status");
            CloseHandle(status_memory_handle);
            status_memory_handle = nullptr;
            UnmapViewOfFile(effect_config);
            effect_config = nullptr;
            CloseHandle(shared_memory_handle);
            shared_memory_handle = nullptr;
            CloseHandle(mutex_handle);
            mutex_handle = nullptr;
            return false;
        }

        reset_effects_internal();
        return true;
    }
    
    void reset_effects_internal() {
        if (effect_config == nullptr || mutex_handle == nullptr) return;
        DWORD wait_result = WaitForSingleObject(mutex_handle, 1000);
        
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            report_error("KH - TeamSpeak: Mutex wait timeout in reset_effects_internal");
            return;
        }

        memset(effect_config, 0, sizeof(TSVoiceEffectConfig));
        effect_config->version = TS_IPC_VERSION;
        effect_config->sequence_number = sequence_counter.fetch_add(1, std::memory_order_relaxed) + 1;
        effect_config->effects_enabled = 0;
        effect_config->effect_chain_count = 0;
        effect_config->checksum = calculate_checksum(effect_config);
        ReleaseMutex(mutex_handle);
    }
    
    void heartbeat_loop() {
        while (heartbeat_running.load(std::memory_order_acquire)) {
            // Check plugin status periodically
            if (plugin_status != nullptr && mutex_handle != nullptr) {
                DWORD wait_result = WaitForSingleObject(mutex_handle, 50);
                
                if (wait_result == WAIT_OBJECT_0 || wait_result == WAIT_ABANDONED) {
                    uint32_t current_tick = static_cast<uint32_t>(GetTickCount());
                    uint32_t last_heartbeat = plugin_status->last_heartbeat;
                    bool plugin_active_flag = plugin_status->plugin_active;
                    ReleaseMutex(mutex_handle);
                    bool plugin_alive = (current_tick - last_heartbeat) < 5000;
                    
                    if (!plugin_alive && plugin_active_flag) {
                        MainThreadScheduler::instance().schedule([]() {
                            report_error("KH - TeamSpeak: Plugin connection lost");
                        });
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    static std::string expand_env_path(const std::string& path) {
        char expanded[MAX_PATH];
        DWORD result = ExpandEnvironmentStringsA(path.c_str(), expanded, MAX_PATH);
        
        if (result > 0 && result < MAX_PATH) {
            return std::string(expanded);
        }
        
        return path;
    }
    
    static std::string get_extension_directory() {
        HMODULE hModule = nullptr;
        char dllPath[MAX_PATH];
        
        if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&get_extension_directory),
            &hModule
        )) {
            return "";
        }
        
        if (hModule == nullptr) {
            return "";
        }
        
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) == 0) {
            return "";
        }
        
        try {
            std::filesystem::path path(dllPath);
            auto parent = path.parent_path();
            if (parent.empty()) return "";
            auto grandparent = parent.parent_path();
            if (grandparent.empty()) return parent.string();
            return grandparent.string();
        } catch (...) {
            return "";
        }
    }
    
    static std::vector<std::string> find_ts3_plugin_directories() {
        std::vector<std::string> valid_paths;
        std::vector<std::string> paths_to_try;
        paths_to_try.push_back("C:\\Program Files\\TeamSpeak 3 Client\\plugins");
        paths_to_try.push_back("C:\\Program Files (x86)\\TeamSpeak 3 Client\\plugins");
        paths_to_try.push_back("%APPDATA%\\TS3Client\\plugins");
        
        // Check standard installation paths
        for (const auto& base_path : paths_to_try) {
            std::string expanded = expand_env_path(base_path);
            
            if (std::filesystem::exists(expanded) && std::filesystem::is_directory(expanded)) {
                valid_paths.push_back(expanded);
            }
        }
        
        // Check registry for custom installation path
        HKEY hKey;
        
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\TeamSpeak 3 Client", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char install_path[MAX_PATH];
            DWORD path_size = MAX_PATH;
            DWORD type;
            
            if (RegQueryValueExA(hKey, "InstallLocation", nullptr, &type, reinterpret_cast<LPBYTE>(install_path), &path_size) == ERROR_SUCCESS) {
                std::string plugins_path = std::string(install_path) + "\\plugins";
                
                if (std::filesystem::exists(plugins_path) && std::filesystem::is_directory(plugins_path)) {
                    valid_paths.push_back(plugins_path);
                }
            }
            
            RegCloseKey(hKey);
        }
        
        // Also check HKEY_LOCAL_MACHINE
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\TeamSpeak 3 Client", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char install_path[MAX_PATH];
            DWORD path_size = MAX_PATH;
            DWORD type;
            
            if (RegQueryValueExA(hKey, "InstallLocation", nullptr, &type, reinterpret_cast<LPBYTE>(install_path), &path_size) == ERROR_SUCCESS) {
                std::string plugins_path = std::string(install_path) + "\\plugins";
                
                if (std::filesystem::exists(plugins_path) && std::filesystem::is_directory(plugins_path)) {
                    valid_paths.push_back(plugins_path);
                }
            }
            
            RegCloseKey(hKey);
        }
        
        // Check WOW6432Node for 32-bit TS3 on 64-bit Windows
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\WOW6432Node\\TeamSpeak 3 Client", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char install_path[MAX_PATH];
            DWORD path_size = MAX_PATH;
            DWORD type;
            
            if (RegQueryValueExA(hKey, "InstallLocation", nullptr, &type, reinterpret_cast<LPBYTE>(install_path), &path_size) == ERROR_SUCCESS) {
                std::string plugins_path = std::string(install_path) + "\\plugins";
                
                if (std::filesystem::exists(plugins_path) && std::filesystem::is_directory(plugins_path)) {
                    valid_paths.push_back(plugins_path);
                }
            }
            
            RegCloseKey(hKey);
        }
        
        return valid_paths;
    }

public:
    static TeamspeakFramework& instance() {
        static TeamspeakFramework inst;
        return inst;
    }
    
    bool initialize() {
        if (is_initialized_flag.load(std::memory_order_acquire)) {
            return true;
        }
        
        std::lock_guard<std::mutex> lock(ipc_mutex);
        
        if (is_initialized_flag.load(std::memory_order_acquire)) {
            return true;
        }

        if (!create_shared_memory()) {
            cleanup();
            return false;
        }
        
        // Start heartbeat monitoring thread
        heartbeat_running.store(true, std::memory_order_release);
        heartbeat_thread = std::thread(&TeamspeakFramework::heartbeat_loop, this);
        is_initialized_flag.store(true, std::memory_order_release);
        reapply_stored_effects();
        return true;
    }
    
    void cleanup() {
        clear_voice_effects();
        
        // Signal stop BEFORE joining - must be atomic with release semantics
        heartbeat_running.store(false, std::memory_order_release);
        
        if (heartbeat_thread.joinable()) {
            heartbeat_thread.join();
        }
        
        std::lock_guard<std::mutex> lock(ipc_mutex);
        
        if (effect_config != nullptr) {
            UnmapViewOfFile(effect_config);
            effect_config = nullptr;
        }
        
        if (plugin_status != nullptr) {
            UnmapViewOfFile(plugin_status);
            plugin_status = nullptr;
        }
        
        if (shared_memory_handle != nullptr) {
            CloseHandle(shared_memory_handle);
            shared_memory_handle = nullptr;
        }
        
        if (status_memory_handle != nullptr) {
            CloseHandle(status_memory_handle);
            status_memory_handle = nullptr;
        }
        
        if (mutex_handle != nullptr) {
            CloseHandle(mutex_handle);
            mutex_handle = nullptr;
        }
        
        is_initialized_flag.store(false, std::memory_order_release);
    }
    
    bool is_initialized() const {
        return is_initialized_flag.load(std::memory_order_acquire);
    }
    
    bool is_plugin_active() const {
        if (!is_initialized_flag.load(std::memory_order_acquire)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(ipc_mutex);
        
        if (plugin_status == nullptr || mutex_handle == nullptr) {
            return false;
        }
        
        DWORD wait_result = WaitForSingleObject(mutex_handle, 50);
        
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return false;
        }
        
        uint32_t current_tick = static_cast<uint32_t>(GetTickCount());
        uint32_t last_heartbeat = plugin_status->last_heartbeat;
        bool active = plugin_status->plugin_active && (current_tick - last_heartbeat) < 5000;
        ReleaseMutex(mutex_handle);
        return active;
    }
        
    bool is_connected() const {
        if (!is_initialized_flag.load(std::memory_order_acquire)) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(ipc_mutex);
        
        if (plugin_status == nullptr || mutex_handle == nullptr) {
            return false;
        }
        
        DWORD wait_result = WaitForSingleObject(mutex_handle, 50);

        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return false;
        }
        
        uint32_t current_tick = static_cast<uint32_t>(GetTickCount());
        uint32_t last_heartbeat = plugin_status->last_heartbeat;
        bool active = plugin_status->plugin_active && (current_tick - last_heartbeat) < 5000;
        bool connected = active && (plugin_status->connected != 0);
        ReleaseMutex(mutex_handle);
        return connected;
    }
    
    bool is_transmitting() const {
        if (!is_initialized_flag.load(std::memory_order_acquire)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(ipc_mutex);
        
        if (plugin_status == nullptr || mutex_handle == nullptr) {
            return false;
        }
        
        DWORD wait_result = WaitForSingleObject(mutex_handle, 50);
        
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return false;
        }
        
        uint32_t current_tick = static_cast<uint32_t>(GetTickCount());
        uint32_t last_heartbeat = plugin_status->last_heartbeat;
        bool active = plugin_status->plugin_active && (current_tick - last_heartbeat) < 5000;
        bool transmitting = active && (plugin_status->transmitting != 0);
        ReleaseMutex(mutex_handle);
        return transmitting;
    }

    bool apply_voice_effects(const std::vector<std::pair<std::string, float>>& effects) {
        // Always store effects locally first
        {
            std::lock_guard<std::mutex> fx_lock(effects_mutex);
            current_effects = effects;
        }
        
        // If not initialized, just store - will apply on next connection
        if (!is_initialized_flag.load(std::memory_order_acquire)) {
            return true;
        }

        return write_effects_to_shared_memory(effects);
    }
    
    bool write_effects_to_shared_memory(const std::vector<std::pair<std::string, float>>& effects) {
        std::lock_guard<std::mutex> ipc_lock(ipc_mutex);
        
        if (effect_config == nullptr || mutex_handle == nullptr) {
            return false;
        }
        
        DWORD wait_result = WaitForSingleObject(mutex_handle, 1000);
        
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return false;
        }

        // Clear effect chain before writing new effects
        memset(effect_config->effect_chain_types, 0, sizeof(effect_config->effect_chain_types));
        memset(effect_config->effect_chain_values, 0, sizeof(effect_config->effect_chain_values));

        // Apply each effect
        uint8_t chain_idx = 0;

        for (const auto& [name, value] : effects) {
            if (chain_idx >= TSVoiceEffectConfig::MAX_EFFECT_CHAIN) break;
            TSEffectType effect_type = get_effect_type_from_name(name);
            if (effect_type == TSEffectType::NONE) continue;
            effect_config->effect_chain_types[chain_idx] = static_cast<uint8_t>(effect_type);
            
            // Clamp values appropriately based on effect type
            float clamped_value = value;
            switch (effect_type) {
                case TSEffectType::LOWPASS:
                case TSEffectType::HIGHPASS:
                case TSEffectType::DISTORTION:
                case TSEffectType::BITCRUSH:
                case TSEffectType::NOISE:
                case TSEffectType::REVERB:
                case TSEffectType::ECHO_DELAY:
                case TSEffectType::RADIO_STATIC:
                case TSEffectType::RADIO_SQUELCH:
                case TSEffectType::FREQUENCY_WOBBLE:
                case TSEffectType::TREMOLO_DEPTH:
                case TSEffectType::COMPRESSOR_THRESHOLD:
                case TSEffectType::TELEPHONE:
                case TSEffectType::UNDERWATER:
                case TSEffectType::AGC:
                case TSEffectType::CHORUS:
                case TSEffectType::FLANGER:
                case TSEffectType::BASS_BOOST:
                    clamped_value = std::clamp(value, 0.0f, 1.0f);
                    break;
                case TSEffectType::RING_MOD:
                    clamped_value = std::clamp(value, 0.0f, 1000.0f);
                    break;
                case TSEffectType::ECHO_DECAY:
                    clamped_value = std::clamp(value, 0.0f, 0.95f);
                    break;
                case TSEffectType::VOLUME:
                    clamped_value = std::clamp(value, 0.0f, 10.0f);
                    break;
                case TSEffectType::PITCH_SHIFT:
                    clamped_value = std::clamp(value, -12.0f, 12.0f);
                    break;
                case TSEffectType::TREMOLO_RATE:
                    clamped_value = std::clamp(value, 0.0f, 20.0f);
                    break;
                case TSEffectType::COMPRESSOR_RATIO:
                    clamped_value = std::clamp(value, 1.0f, 20.0f);
                    break;
                case TSEffectType::CHORUS_RATE:
                case TSEffectType::FLANGER_RATE:
                    clamped_value = std::clamp(value, 0.1f, 5.0f);
                    break;
                default:
                    break;
            }
            
            effect_config->effect_chain_values[chain_idx] = clamped_value;
            chain_idx++;
        }

        effect_config->effects_enabled = (chain_idx > 0) ? 1 : 0;
        effect_config->effect_chain_count = chain_idx;
        effect_config->sequence_number = sequence_counter.fetch_add(1, std::memory_order_relaxed) + 1;
        effect_config->checksum = calculate_checksum(effect_config);
        ReleaseMutex(mutex_handle);
        return true;
    }
    
    void reapply_stored_effects() {
        std::vector<std::pair<std::string, float>> effects_copy;

        {
            std::lock_guard<std::mutex> fx_lock(effects_mutex);
            if (current_effects.empty()) return;
            effects_copy = current_effects;
        }

        write_effects_to_shared_memory(effects_copy);
    }
    
    bool clear_voice_effects() {
        // Always clear stored effects
        {
            std::lock_guard<std::mutex> fx_lock(effects_mutex);
            current_effects.clear();
        }
        
        // If not initialized, nothing else to do
        if (!is_initialized_flag.load(std::memory_order_acquire)) {
            return true;
        }
        
        std::lock_guard<std::mutex> ipc_lock(ipc_mutex);
        
        if (effect_config == nullptr || mutex_handle == nullptr) {
            return false;
        }
        
        DWORD wait_result = WaitForSingleObject(mutex_handle, 1000);
        
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return false;
        }
        
        memset(effect_config, 0, sizeof(TSVoiceEffectConfig));
        effect_config->version = TS_IPC_VERSION;
        effect_config->sequence_number = sequence_counter.fetch_add(1, std::memory_order_relaxed) + 1;
        effect_config->effects_enabled = 0;
        effect_config->effect_chain_count = 0;
        effect_config->checksum = calculate_checksum(effect_config);
        ReleaseMutex(mutex_handle);
        return true;
    }
    
    // Get the path to the plugin
    static std::string get_plugin_dll_source_path() {
        std::string ext_dir = get_extension_directory();
        if (ext_dir.empty()) return "";
        return ext_dir + "\\kh_framework_teamspeak_win64.dll";
    }
    
    // Install plugin using
    static bool install_plugin() {
        std::string source_dll_path = get_plugin_dll_source_path();
        
        if (source_dll_path.empty()) {
            sqf::diag_log("KH - TeamSpeak: Could not determine extension directory");
            return false;
        }
        
        if (!std::filesystem::exists(source_dll_path)) {
            sqf::diag_log("KH - TeamSpeak: Plugin DLL not found at: " + source_dll_path);
            return false;
        }
        
        // Find TeamSpeak plugin directories
        auto ts3_dirs = find_ts3_plugin_directories();
        
        if (ts3_dirs.empty()) {
            sqf::diag_log("KH - TeamSpeak: No TeamSpeak plugin directories found");
            return false;
        }
        
        bool installed = false;
        
        for (const auto& plugins_dir : ts3_dirs) {
            std::string dest_path = plugins_dir + "\\kh_framework_teamspeak_win64.dll";
            
            try {
                // Create plugins directory if it doesn't exist
                std::filesystem::create_directories(plugins_dir);
                
                // Copy the DLL (overwrite if exists)
                std::filesystem::copy_file(
                    source_dll_path,
                    dest_path,
                    std::filesystem::copy_options::overwrite_existing
                );
                
                sqf::diag_log("KH - TeamSpeak: Plugin installed to: " + dest_path);
                installed = true;
                break;
            } catch (const std::filesystem::filesystem_error& e) {
                sqf::diag_log("KH - TeamSpeak: Failed to copy plugin to " + dest_path + ": " + std::string(e.what()));
                // Continue trying other directories
            }
        }
        
        if (!installed) {
            sqf::diag_log("KH - TeamSpeak: Failed to install plugin to any TeamSpeak directory");
        }
        
        return installed;
    }
    
    // Check if plugin is installed
    static bool is_plugin_installed() {
        auto ts3_dirs = find_ts3_plugin_directories();
        
        for (const auto& plugins_dir : ts3_dirs) {
            // Check for the DLL with the naming convention TS3 uses
            std::string plugin_path = plugins_dir + "\\kh_framework_teamspeak_win64.dll";

            if (std::filesystem::exists(plugin_path)) {
                return true;
            }
        }
        
        return false;
    }

    // Parse effects from game_value args (matches TTS format)
    static std::vector<std::pair<std::string, float>> parse_effects_from_args(const game_value& args, size_t start_index) {
        std::vector<std::pair<std::string, float>> effects;
        if (args.type_enum() != game_data_type::ARRAY) return effects;
        auto& arr = args.to_array();
        
        for (size_t i = start_index; i < arr.size(); i++) {
            if (arr[i].type_enum() == game_data_type::ARRAY) {
                auto& effect_arr = arr[i].to_array();
                
                if (effect_arr.size() >= 2 && 
                    effect_arr[0].type_enum() == game_data_type::STRING &&
                    effect_arr[1].type_enum() == game_data_type::SCALAR) {
                    std::string name = effect_arr[0];
                    float value = effect_arr[1];
                    effects.emplace_back(name, value);
                }
            }
        }
        
        return effects;
    }
};

// Auto-install check function - call from pre_start
static void teamspeak_check_and_install_plugin() {
    try {
        if (!TeamspeakFramework::is_plugin_installed()) {
            TeamspeakFramework::install_plugin();
        }
    } catch (const std::exception& e) {
        // Silent failure for auto-check
    } catch (...) {
        // Silent failure
    }
}