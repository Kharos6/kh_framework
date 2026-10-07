#include "framework.hpp"
#include "search_mod_folders.hpp"
#include "rv_extension_bridge.hpp"
#include "cryptography.hpp"
#include "kh_data.hpp"
#include "lua_integration.hpp"
#include "ai_integration.hpp"
#include "teamspeak_integration.hpp"
#include "text_to_speech_integration.hpp"
#include "speech_to_text_integration.hpp"
#include "ui_integration.hpp"
#include "network_integration.hpp"
#include "rendering_integration.hpp"
#include "sqf_integration.hpp"

using namespace intercept;
using namespace intercept::types;

static std::unordered_map<std::string, HMODULE> g_loaded_delay_modules;
static std::mutex g_delay_load_mutex;

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

// The Lua "game" / "mission" tables from the globals: the phase flags and whether a mission is active, with the
// machine's role and the frame / time counters.
static void kh_lua_sync_state(bool pre_init, bool post_init, bool mission_active) {
    if (!g_lua_state) return;
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = pre_init;
    game["postInit"] = post_init;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    game["server"] = g_is_server;
    game["dedicated"] = g_is_dedicated_server;
    game["headless"] = g_is_headless;
    game["player"] = g_is_player;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = mission_active;
}

// Stops every framework a mission started (both mission edges): the AI instances, TTS, STT, the HTML UI, the
// network, and TeamSpeak - its effects cleared at a mission's start, its IPC closed at a mission's end. Each
// failure is reported and the next framework still stops. TTS and STT are stopped unconditionally: their
// is_initialized() only says a model is loaded, while their worker threads outlive a failed reload, and
// cleanup() is idempotent.
static void kh_stop_frameworks(bool teamspeak_disconnect) {
    auto stop = [](const char* what, bool initialized, const std::function<void()>& action) {
        if (!initialized) return;

        try {
            action();
        } catch (const std::exception& e) {
            report_error(std::string("KH Framework: error stopping ") + what + ": " + e.what());
        } catch (...) {
            report_error(std::string("KH Framework: unknown error stopping ") + what);
        }
    };

    stop("AI instances", AIFramework::instance().is_initialized(), []() { AIFramework::instance().stop_all(); });
    stop("TTS", true, []() { TTSFramework::instance().cleanup(); });
    stop("STT", true, []() { STTFramework::instance().cleanup(); });
    stop("UI framework", UIFramework::instance().is_initialized(), []() { UIFramework::instance().shutdown(); });
    stop("Network framework", NetworkFramework::instance().is_initialized(),
         []() { NetworkFramework::instance().shutdown(); });
    stop(teamspeak_disconnect ? "TeamSpeak bridge" : "TeamSpeak effects",
         TeamspeakFramework::instance().is_initialized(), [teamspeak_disconnect]() {
             if (teamspeak_disconnect) TeamspeakFramework::instance().cleanup();
             else TeamspeakFramework::instance().clear_voice_effects();
         });
}

// kh_stop_frameworks, then the scheduler cleared of what the framework threads scheduled while they were being
// stopped (a generation that finished during the join, its event or report): that work belongs to the mission that
// ended, not to the next on_frame. Anything already queued before the stop (rendering_integration_reset runs just
// before this) is kept, as it always was.
static void kh_stop_frameworks_cleared(bool teamspeak_disconnect) {
    auto kept = MainThreadScheduler::instance().take();
    kh_stop_frameworks(teamspeak_disconnect);
    MainThreadScheduler::instance().clear();
    MainThreadScheduler::instance().restore(std::move(kept));
}

void intercept::pre_start() {
    g_is_player = sqf::has_interface();
    g_kh_cached_entity_initializations = game_value(auto_array<game_value>());
    g_kh_cached_entity_initializations_deletions = game_value(auto_array<game_value>());
    (void)AIFramework::instance();
    (void)TTSFramework::instance();
    (void)STTFramework::instance();
    (void)UIFramework::instance();
    (void)NetworkFramework::instance();
    (void)TeamspeakFramework::instance();
    sqf::call_extension("kh_rv_extension", "ready");

    if (RVExtBridge::initialize()) {
        sqf::diag_log("KH Framework: RV extension initialized");
    } else {
        sqf::diag_log("KH Framework: RV extension failed");
    }

    initialize_sqf_integration();
    initialize_lua_state();
    kh_lua_sync_state(false, false, false);
    KHDataManager::instance().initialize();
    sqf::diag_log(g_has_cuda ? "KH Framework: CUDA detected" : "KH Framework: CUDA not detected");
    sqf::diag_log(g_has_vulkan ? "KH Framework: Vulkan detected" : "KH Framework: Vulkan not detected");

    teamspeak_check_and_install_plugin();
    sqf::diag_log("KH Framework: pre-start");
}

void intercept::pre_init() {
    ModFolderSearcher::clear_cache();
    g_is_server = sqf::is_server();
    g_is_headless = (!(sqf::is_server()) && !(sqf::has_interface()));
    populate_sqf_command_map();
    auto displays = sqf::all_displays();
    g_is_menu = (displays.size() == 1 && displays[0] == sqf::find_display(0));
    g_is_eden = sqf::is_eden();
    g_is_dedicated_server = sqf::is_dedicated();

    if (!g_is_menu || g_is_dedicated_server) {
        g_last_ts_connect_attempt = -1.0f;
        g_mission_time = 0.0f;
        g_mission_frame = 0;
        kh_lua_sync_state(true, false, true);
        clean_lua_state();
        KHDataManager::instance().flush_all();
        MainThreadScheduler::instance().clear();
        RenderIntegration::rendering_integration_reset();
        kh_stop_frameworks_cleared(false);
        kh_temporal_clear();
        raw_call_sqf_native_no_return(sqf::get_variable(sqf::mission_namespace(), "kh_fnc_preinit"));
        g_kh_cached_entity_initializations = sqf::get_variable(sqf::mission_namespace(), "kh_var_entityinitializations");
        g_kh_cached_entity_initializations_deletions = sqf::get_variable(sqf::mission_namespace(), "kh_var_entityinitializationsdeletions");
        sqf::diag_log("KH Framework: pre-init");
    }
}

void intercept::post_init() {
    if (!g_is_menu || g_is_dedicated_server) {
        if (g_lua_state) {
            LuaStackGuard guard(*g_lua_state);
            sol::table game = (*g_lua_state)["game"];
            game["postInit"] = true;
        }
        
        raw_call_sqf_native_no_return(sqf::get_variable(sqf::mission_namespace(), "kh_fnc_postinit"));
        sqf::diag_log("KH Framework: post-init");
    }
}

void intercept::on_frame() {
    g_is_eden = sqf::is_eden();
    process_temporal_execution_stack();        
    MainThreadScheduler::instance().process_frame();

    if ((!g_is_eden && !g_is_menu) || g_is_dedicated_server) {
        if (!g_is_dedicated_server) {
            if (!g_is_eden && sqf::is_multiplayer()) {
                if (g_last_ts_connect_attempt < 0.0f || g_game_time - g_last_ts_connect_attempt >= 1.0f) {
                    g_last_ts_connect_attempt = g_game_time;
                    
                    try {
                        TeamspeakFramework::instance().initialize();
                    } catch (...) {
                        // Silent failure - will retry next second
                    }
                }
            }

            if (UIFramework::instance().is_initialized()) {
                UIFramework::instance().set_mouse_enabled(sqf::dialog());
            }
        }

        update_unit_states();
        float current_delta = sqf::diag_delta_time();
        g_game_time += current_delta;
        g_game_frame++;
        g_mission_time += current_delta;
        g_mission_frame++;

        if (g_lua_state) {
            LuaStackGuard guard(*g_lua_state);
            sol::table game = (*g_lua_state)["game"];
            sol::table mission = (*g_lua_state)["mission"];
            game["frame"] = g_game_frame;
            game["time"] = g_game_time;
            mission["frame"] = g_mission_frame;
            mission["time"] = g_mission_time;
            LuaFunctions::update_scheduler();
        }

        network_on_frame();
    }
}

void intercept::mission_ended() {
    g_is_server = false;
    g_is_headless = false;
    kh_temporal_clear();
    g_kh_cached_entity_initializations = game_value(auto_array<game_value>());
    g_kh_cached_entity_initializations_deletions = game_value(auto_array<game_value>());
    g_mission_time = 0.0f;
    g_mission_frame = 0;
    reset_lua_state();
    kh_lua_sync_state(false, false, false);
    KHDataManager::instance().flush_all();
    MainThreadScheduler::instance().clear();
    RenderIntegration::rendering_integration_reset();
    kh_stop_frameworks_cleared(true);
    sqf::diag_log("KH Framework: mission end");
    g_is_menu = true;
    ModFolderSearcher::clear_cache();
}

// The folders a runtime's DLLs are looked for in (try_load_* and delay_load_hook): the SDK environment
// variables' bin folders first, then the usual install folders.
static std::vector<std::string> kh_env_bin_paths(std::initializer_list<const char*> variables, const char* bin) {
    std::vector<std::string> paths;
    char value[MAX_PATH];

    for (const char* variable : variables) {
        DWORD result = GetEnvironmentVariableA(variable, value, MAX_PATH);
        if (result > 0 && result < MAX_PATH) paths.push_back(std::string(value) + bin);
    }

    return paths;
}

static std::vector<std::string> cuda_search_paths() {
    std::vector<std::string> paths = kh_env_bin_paths({ "CUDA_PATH", "CUDA_PATH_V12_9" }, "\\bin\\");

    for (const char* version : { "v12.9", "v12.8", "v12.7", "v12.6", "v12.5", "v12.4", "v12.3", "v12.2", "v12.1",
                                 "v12.0", "v12" }) {
        paths.push_back(std::string("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\") + version + "\\bin\\");
    }

    return paths;
}

static std::vector<std::string> vulkan_search_paths() {
    std::vector<std::string> paths = kh_env_bin_paths({ "VULKAN_SDK", "VK_SDK_PATH" }, "\\Bin\\");
    paths.push_back("C:\\Windows\\System32\\");

    for (const char* version : { "1.4.309.0", "1.4.304.1", "1.3.296.0", "1.3.290.0", "1.3.283.0", "1.3.280.0",
                                 "1.3.275.0", "1.3.268.0" }) {
        paths.push_back(std::string("C:\\VulkanSDK\\") + version + "\\Bin\\");
    }

    return paths;
}

// Whether file exists in one of the folders.
static bool kh_file_in_paths(const std::vector<std::string>& paths, const char* file) {
    for (const auto& path : paths) {
        if (GetFileAttributesA((path + file).c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    }

    return false;
}

static bool try_load_vulkan() {
    return kh_file_in_paths(vulkan_search_paths(), "vulkan-1.dll");
}

static bool try_load_cuda() {
    return kh_file_in_paths(cuda_search_paths(), "cublas64_12.dll");
}

static void detect_gpu_backends() {
    if (try_load_cuda()) {
        g_active_backend = GPUBackend::CUDA;
        g_has_cuda = true;
    }

    if (try_load_vulkan()) {
        if (!g_has_cuda) {
            g_active_backend = GPUBackend::VULKAN;
        }

        g_has_vulkan = true;
    }

    if (!g_has_cuda && !g_has_vulkan) {
        g_active_backend = GPUBackend::CPU;
    }
}

// A message to the debugger now and to the RPT on the next frame (the hook runs inside the loader).
static void kh_delay_load_log(const std::string& msg) {
    OutputDebugStringA((msg + "\n").c_str());
    MainThreadScheduler::instance().schedule([msg]() { sqf::diag_log(msg); });
}

// Loads the DLL at path and keeps it (g_loaded_delay_modules); null when it fails, with the error logged when
// report_failure.
static HMODULE kh_delay_load_file(const std::string& dll_name, const std::string& path, bool report_failure) {
    HMODULE module = LoadLibraryA(path.c_str());
    const DWORD error = GetLastError();   // Before the string building below can change it.

    if (module != NULL) {
        g_loaded_delay_modules[dll_name] = module;
    } else if (report_failure) {
        kh_delay_load_log("KH Framework: failed to load " + dll_name + " from " + path + " - error code " +
                          std::to_string(error));
    }

    return module;
}

// The first copy of dll_name found in the folders, loaded; null (logged) when there is none.
static HMODULE kh_delay_load_from_paths(const std::string& dll_name, const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        HMODULE module = kh_delay_load_file(dll_name, path + dll_name, false);
        if (module != NULL) return module;
    }

    kh_delay_load_log("KH Framework: " + dll_name + " not found in its standard locations");
    return NULL;
}

// The delay-load hook: a delay-loaded DLL is taken from the mod folder (the extension's parent folder), with the
// DLLs it depends on loaded from there first; the CUDA and Vulkan runtimes from their install folders. Null lets
// the loader search as usual.
static FARPROC WINAPI delay_load_hook(unsigned dliNotify, PDelayLoadInfo pdli) {
    std::lock_guard<std::mutex> lock(g_delay_load_mutex);

    if (dliNotify == dliFailLoadLib) {
        std::string dll_name = pdli->szDll;

        if (_stricmp(dll_name.c_str(), "lua51.dll") == 0) {
            kh_delay_load_log("KH Framework: CRITICAL - " + dll_name +
                              " failed to load; the extension cannot function");
        } else {
            kh_delay_load_log("KH Framework: " + dll_name + " failed to load");
        }

        return NULL;
    }

    if (dliNotify != dliNotePreLoadLibrary) return NULL;
    HMODULE hModule = nullptr;

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)&delay_load_hook, &hModule) || hModule == nullptr) {
        return NULL;
    }

    char dllPath[MAX_PATH];
    if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) == 0) return NULL;
    std::string pathStr(dllPath);
    size_t lastSlash = pathStr.find_last_of("\\/");
    if (lastSlash == std::string::npos) return NULL;
    std::string extensionDir = pathStr.substr(0, lastSlash);
    size_t parentSlash = extensionDir.find_last_of("\\/");
    if (parentSlash == std::string::npos) return NULL;
    std::string modDir = extensionDir.substr(0, parentSlash);
    std::string dll_name = pdli->szDll;
    std::string dllFullPath = modDir + "\\" + dll_name;
    auto is = [&dll_name](const char* name) { return _stricmp(dll_name.c_str(), name) == 0; };

    if (is("cublas64_12.dll") || is("cublaslt64_12.dll")) {
        return (FARPROC)kh_delay_load_from_paths(dll_name, cuda_search_paths());
    }

    if (is("vulkan-1.dll")) {
        return (FARPROC)kh_delay_load_from_paths(dll_name, vulkan_search_paths());
    }

    if (is("sherpa-onnx-c-api.dll")) {
        kh_delay_load_file("DirectML.dll", modDir + "\\DirectML.dll", false);
        kh_delay_load_file("onnxruntime.dll", modDir + "\\onnxruntime.dll", false);
    } else if (is("Ultralight.dll")) {
        kh_delay_load_file("UltralightCore.dll", modDir + "\\UltralightCore.dll", false);
        kh_delay_load_file("WebCore.dll", modDir + "\\WebCore.dll", false);
    } else if (is("WebCore.dll")) {
        kh_delay_load_file("UltralightCore.dll", modDir + "\\UltralightCore.dll", false);
    } else if (!is("lua51.dll")) {
        return NULL;
    }

    HMODULE module = kh_delay_load_file(dll_name, dllFullPath, true);
    if (module != NULL) return (FARPROC)module;

    if (is("lua51.dll")) {
        kh_delay_load_log("KH Framework: CRITICAL - " + dll_name + " failed to load; the extension cannot function");
    } else {
        kh_delay_load_log("KH Framework: " + dll_name + " not found in its standard locations");
    }

    return NULL;
}

// Set the delay-load hook
extern "C" const PfnDliHook __pfnDliNotifyHook2 = delay_load_hook;
extern "C" const PfnDliHook __pfnDliFailureHook2 = delay_load_hook;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableProcessWindowsGhosting();
            ShutdownWatchdog::instance().initialize();
            detect_gpu_backends();
            break;
        case DLL_PROCESS_DETACH:
            // Process exit: every other thread is already gone and the frameworks are never destroyed, so there is
            // nothing to stop - the watchdog is disarmed and the TeamSpeak plugin is told to drop the voice effects
            // (bounded, no lock a dead thread may hold, no SQF; the one thing the original's static destructors did
            // that mattered). A runtime unload (FreeLibrary) stops every framework and its threads here, behind one
            // 3 s watchdog that ends the process if a stop hangs.
            if (lpReserved != nullptr) {
                ShutdownWatchdog::instance().shutdown(true);

                __try {
                    TeamspeakFramework::instance().clear_effects_at_exit();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            } else {
                ShutdownWatchdog::instance().arm(3000);
                MainThreadScheduler::instance().clear();

                __try {
                    if (AIFramework::instance().is_initialized()) {
                        AIFramework::instance().stop_all();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    TTSFramework::instance().cleanup();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    STTFramework::instance().cleanup();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    UIFramework::instance().shutdown(true);
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    NetworkFramework::instance().shutdown();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    TeamspeakFramework::instance().cleanup();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (RenderIntegration::rendering_integration_is_initialized()) {
                        RenderIntegration::rendering_integration_process_detach();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    MH_Uninitialize();
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                ShutdownWatchdog::instance().shutdown(false);
            }

            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}