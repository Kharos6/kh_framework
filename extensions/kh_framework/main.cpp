#include "framework.hpp"
#include "search_mod_folders.hpp"
#include "cryptography.hpp"
#include "kh_data.hpp"
#include "lua_integration.hpp"
#include "ai_integration.hpp"
#include "text_to_speech_integration.hpp"
#include "speech_to_text_integration.hpp"
#include "sqf_integration.hpp"

using namespace intercept;
using namespace intercept::types;

static std::unordered_map<std::string, HMODULE> g_loaded_delay_modules;
static std::mutex g_delay_load_mutex;

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    initialize_sqf_integration();
    initialize_lua_state();
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    game["server"] = g_is_server;
    game["dedicated"] = g_is_dedicated_server;
    game["headless"] = g_is_headless;
    game["player"] = g_is_player;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = false;
    KHDataManager::instance().initialize();
    sqf::diag_log("KH Framework Extension - Pre-start");
}

void intercept::pre_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = true;
    game["postInit"] = false;
    g_is_server = sqf::is_server();
    g_is_dedicated_server = sqf::is_dedicated();
    g_is_headless = (!(sqf::is_server()) && !(sqf::has_interface()));
    g_is_player = sqf::has_interface();
    game["server"] = g_is_server;
    game["dedicated"] = g_is_dedicated_server;
    game["headless"] = g_is_headless;
    game["player"] = g_is_player;
    g_mission_time = 0.0f;
    g_mission_frame = 0;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = true;
    clean_lua_state();
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework Extension - Pre-init");
}

void intercept::post_init() {
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    game["postInit"] = true;
    sqf::diag_log("KH Framework Extension - Post-init");
}

void intercept::on_frame() {
    MainThreadScheduler::instance().process_frame();
    LuaStackGuard guard(*g_lua_state);
    float current_delta = sqf::diag_delta_time();
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    g_game_time += current_delta;
    g_game_frame++;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    g_mission_time += current_delta;
    g_mission_frame++;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    LuaFunctions::update_scheduler();
}

void intercept::mission_ended() {
    MainThreadScheduler::instance().clear();
    
    if (AIFramework::instance().is_initialized()) {
        try {
            AIFramework::instance().stop_all();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping AI instances: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping AI instances");
        }
    }

    if (TTSFramework::instance().is_initialized()) {
        try {
            TTSFramework::instance().cleanup();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping TTS instances: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping TTS instances");
        }
    }

    if (STTFramework::instance().is_initialized_public()) {
        try {
            STTFramework::instance().cleanup_public();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping STT: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping STT");
        }
    }

    reset_lua_state();
    LuaStackGuard guard(*g_lua_state);
    sol::table game = (*g_lua_state)["game"];
    sol::table mission = (*g_lua_state)["mission"];
    game["preInit"] = false;
    game["postInit"] = false;
    game["frame"] = g_game_frame;
    game["time"] = g_game_time;
    game["server"] = g_is_server;
    game["dedicated"] = g_is_dedicated_server;
    game["headless"] = g_is_headless;
    game["player"] = g_is_player;
    g_mission_time = 0.0f;
    g_mission_frame = 0;
    mission["frame"] = g_mission_frame;
    mission["time"] = g_mission_time;
    mission["active"] = false;
    KHDataManager::instance().flush_all();
    sqf::diag_log("KH Framework Extension - Mission End");
}

static FARPROC WINAPI delay_load_hook(unsigned dliNotify, PDelayLoadInfo pdli) {
    std::lock_guard<std::mutex> lock(g_delay_load_mutex);
    HMODULE hModule;

    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     (LPCSTR)&delay_load_hook, &hModule);
    
    char dllPath[MAX_PATH];

    if (dliNotify == dliNotePreLoadLibrary) {
        std::string dll_name = pdli->szDll;
        
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) != 0) {
            std::string pathStr(dllPath);
            size_t lastSlash = pathStr.find_last_of("\\/");
            
            if (lastSlash != std::string::npos) {
                std::string extensionDir = pathStr.substr(0, lastSlash);
                size_t parentSlash = extensionDir.find_last_of("\\/");
                
                if (parentSlash != std::string::npos) {
                    std::string modDir = extensionDir.substr(0, parentSlash);
                    std::string dllFullPath = modDir + "\\" + dll_name;
                    
                    if ((_stricmp(dll_name.c_str(), "cublas64_12.dll") == 0) || (_stricmp(dll_name.c_str(), "cublaslt64_12.dll") == 0)) {
                        std::vector<std::string> cuda_paths_to_try;
                        char cudaPath[MAX_PATH];
                        DWORD result = GetEnvironmentVariableA("CUDA_PATH", cudaPath, MAX_PATH);
                        
                        if (result > 0 && result < MAX_PATH) {
                            cuda_paths_to_try.push_back(std::string(cudaPath) + "\\bin\\");
                        }
                        
                        result = GetEnvironmentVariableA("CUDA_PATH_V12_9", cudaPath, MAX_PATH);
                        
                        if (result > 0 && result < MAX_PATH) {
                            cuda_paths_to_try.push_back(std::string(cudaPath) + "\\bin\\");
                        }
                        
                        // Add standard installation paths
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.9\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.8\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.7\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.5\\bin\\");
                        
                        for (const auto& path : cuda_paths_to_try) {
                            std::string fullPath = path + dll_name;
                            HMODULE hCudaDll = LoadLibraryA(fullPath.c_str());
                            
                            if (hCudaDll != NULL) {
                                g_loaded_delay_modules[dll_name] = hCudaDll;
                                return (FARPROC)hCudaDll;
                            }
                        }
                        
                        // None of the paths worked
                        report_error("KH - AI Framework: " + dll_name + " not found in standard locations");
                        g_cuda_available = false;
                    } else {
                        HMODULE hDll = LoadLibraryA(dllFullPath.c_str());
                        
                        if (hDll != NULL) {
                            g_loaded_delay_modules[dll_name] = hDll;
                            return (FARPROC)hDll;
                        } else {
                            DWORD error = GetLastError();
                            report_error("Failed to load " + dll_name + " from " + dllFullPath + " - error code: " + std::to_string(error));
                        }
                    }
                }
            }
        }
    }
    else if (dliNotify == dliFailLoadLib) {
        std::string dll_name = pdli->szDll;
        
        if ((_stricmp(dll_name.c_str(), "cublas64_12.dll") == 0) || 
            (_stricmp(dll_name.c_str(), "cublaslt64_12.dll") == 0)) {
            // Expected on systems without CUDA - don't treat as error
            g_cuda_available = false;
            report_error("KH - AI Framework: " + dll_name + " failed to load");
        } else if (_stricmp(dll_name.c_str(), "lua51.dll") == 0) {
            // This IS critical
            report_error("KH - AI Framework: CRITICAL - " + dll_name + " failed to load - extension cannot function");
        } else {
            // Other DLL failures
            report_error("KH - AI Framework: " + dll_name + " failed to load");
        }
    }
    
    return NULL;
}

static void cleanup_delay_loaded_modules() {
    std::lock_guard<std::mutex> lock(g_delay_load_mutex);
    
    for (auto& pair : g_loaded_delay_modules) {
        try {
            if (pair.second != NULL) {
                FreeLibrary(pair.second);
                sqf::diag_log("Unloaded delay-loaded module: " + pair.first);
            }
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
    
    g_loaded_delay_modules.clear();
}

// Set the delay-load hook
extern "C" const PfnDliHook __pfnDliNotifyHook2 = delay_load_hook;
extern "C" const PfnDliHook __pfnDliFailureHook2 = delay_load_hook;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            if (!lpReserved) {
                if (AIFramework::instance().is_initialized()) {
                    try {
                        AIFramework::instance().stop_all();
                        TTSFramework::instance().cleanup();
                        STTFramework::instance().cleanup_public();
                    } catch (...) {
                        // Ignore errors during forced shutdown
                    }
                }
            }

            cleanup_delay_loaded_modules();
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}