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
#include "sqf_integration.hpp"

using namespace intercept;
using namespace intercept::types;

static std::unordered_map<std::string, HMODULE> g_loaded_delay_modules;
static std::mutex g_delay_load_mutex;

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::pre_start() {
    (void)AIFramework::instance();
    (void)TTSFramework::instance();
    (void)STTFramework::instance();
    (void)UIFramework::instance();
    (void)NetworkFramework::instance();
    (void)TeamspeakFramework::instance();
    sqf::call_extension("kh_rv_extension", "ready");

    if (RVExtBridge::initialize()) {
        sqf::diag_log("KH Framework Extension - RV Extension Initialized");
    } else {
        sqf::diag_log("KH Framework Extension - RV Extension Failed");
    }

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

    if (g_has_cuda) {
        sqf::diag_log("KH Framework Extension - CUDA Detected");
    }
    else {
        sqf::diag_log("KH Framework Extension - CUDA Not Detected");
    }

    if (g_has_vulkan) {
        sqf::diag_log("KH Framework Extension - Vulkan Detected");
    }
    else {
        sqf::diag_log("KH Framework Extension - Vulkan Not Detected");
    }

    teamspeak_check_and_install_plugin();
    sqf::diag_log("KH Framework Extension - Pre-start");
}

void intercept::pre_init() {
    auto displays = sqf::all_displays();
    g_is_menu = (displays.size() == 1 && displays[0] == sqf::find_display(0));

    if (!g_is_menu) {
        g_last_ts_connect_attempt = -1.0f;
        g_is_server = sqf::is_server();
        g_is_dedicated_server = sqf::is_dedicated();
        g_is_headless = (!(sqf::is_server()) && !(sqf::has_interface()));
        g_is_player = sqf::has_interface();
        g_mission_time = 0.0f;
        g_mission_frame = 0;
        LuaStackGuard guard(*g_lua_state);
        sol::table game = (*g_lua_state)["game"];
        sol::table mission = (*g_lua_state)["mission"];
        game["preInit"] = true;
        game["postInit"] = false;
        game["server"] = g_is_server;
        game["dedicated"] = g_is_dedicated_server;
        game["headless"] = g_is_headless;
        game["player"] = g_is_player;
        mission["frame"] = g_mission_frame;
        mission["time"] = g_mission_time;
        mission["active"] = true;
        clean_lua_state();
        KHDataManager::instance().flush_all();
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

        if (UIFramework::instance().is_initialized()) {
            try {
                UIFramework::instance().shutdown();
            } catch (const std::exception& e) {
                report_error("KH Framework Extension - Error stopping UI framework: " + std::string(e.what()));
            } catch (...) {
                report_error("KH Framework Extension - Unknown error stopping UI framework");
            }
        }

        if (NetworkFramework::instance().is_initialized()) {
            try {
                NetworkFramework::instance().shutdown();
            } catch (const std::exception& e) {
                report_error("KH Framework Extension - Error stopping Network framework: " + std::string(e.what()));
            } catch (...) {
                report_error("KH Framework Extension - Unknown error stopping Network framework");
            }
        }

        if (TeamspeakFramework::instance().is_initialized()) {
            try {
                TeamspeakFramework::instance().clear_voice_effects();
            } catch (const std::exception& e) {
                report_error("KH Framework Extension - Error clearing TeamSpeak effects: " + std::string(e.what()));
            } catch (...) {
                report_error("KH Framework Extension - Unknown error clearing TeamSpeak effects");
            }
        }

        sqf::diag_log("KH Framework Extension - Pre-init");
    }
}

void intercept::post_init() {
    if (!g_is_menu) {
        LuaStackGuard guard(*g_lua_state);
        sol::table game = (*g_lua_state)["game"];
        game["postInit"] = true;
        sqf::diag_log("KH Framework Extension - Post-init");
    }
}

void intercept::on_frame() {
    if (!g_is_menu) {
        if (!g_is_dedicated_server) {
            if (!sqf::is_eden() && sqf::is_multiplayer()) {
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
        network_on_frame();
    }
}

void intercept::mission_ended() {
    reset_lua_state();
    g_mission_time = 0.0f;
    g_mission_frame = 0;
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
    KHDataManager::instance().flush_all();
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
            report_error("KH Framework Extension - Error stopping STT instance: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping STT instance");
        }
    }

    if (UIFramework::instance().is_initialized()) {
        try {
            UIFramework::instance().shutdown();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping UI framework: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping UI framework");
        }
    }

    if (NetworkFramework::instance().is_initialized()) {
        try {
            NetworkFramework::instance().shutdown();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping Network framework: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping Network framework");
        }
    }

    if (TeamspeakFramework::instance().is_initialized()) {
        try {
            TeamspeakFramework::instance().cleanup();
        } catch (const std::exception& e) {
            report_error("KH Framework Extension - Error stopping TeamSpeak bridge: " + std::string(e.what()));
        } catch (...) {
            report_error("KH Framework Extension - Unknown error stopping TeamSpeak bridge");
        }
    }

    sqf::diag_log("KH Framework Extension - Mission End");
}

static bool try_load_vulkan() {
    std::vector<std::string> vulkan_paths_to_try;
    char vulkanPath[MAX_PATH];
    DWORD result = GetEnvironmentVariableA("VULKAN_SDK", vulkanPath, MAX_PATH);
    
    if (result > 0 && result < MAX_PATH) {
        vulkan_paths_to_try.push_back(std::string(vulkanPath) + "\\Bin\\");
    }
    
    result = GetEnvironmentVariableA("VK_SDK_PATH", vulkanPath, MAX_PATH);

    if (result > 0 && result < MAX_PATH) {
        vulkan_paths_to_try.push_back(std::string(vulkanPath) + "\\Bin\\");
    }

    vulkan_paths_to_try.push_back("C:\\Windows\\System32\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.4.309.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.4.304.1\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.296.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.290.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.283.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.280.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.275.0\\Bin\\");
    vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.268.0\\Bin\\");

    for (const auto& path : vulkan_paths_to_try) {
        std::string fullPath = path + "vulkan-1.dll";
        DWORD attrs = GetFileAttributesA(fullPath.c_str());

        if (attrs != INVALID_FILE_ATTRIBUTES) {
            return true;
        }
    }

    return false;
}

static bool try_load_cuda() {
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
    
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.9\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.8\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.7\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.5\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.4\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.3\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.2\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.1\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.0\\bin\\");
    cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12\\bin\\");
    
    for (const auto& path : cuda_paths_to_try) {
        std::string fullPath = path + "cublas64_12.dll";
        DWORD attrs = GetFileAttributesA(fullPath.c_str());

        if (attrs != INVALID_FILE_ATTRIBUTES) {
            return true;
        }
    }
    
    return false;
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

                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.9\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.8\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.7\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.5\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.4\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.3\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.2\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.1\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.0\\bin\\");
                        cuda_paths_to_try.push_back("C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12\\bin\\");
                        
                        for (const auto& path : cuda_paths_to_try) {
                            std::string fullPath = path + dll_name;
                            HMODULE hCudaDll = LoadLibraryA(fullPath.c_str());
                            
                            if (hCudaDll != NULL) {
                                g_loaded_delay_modules[dll_name] = hCudaDll;
                                return (FARPROC)hCudaDll;
                            }
                        }

                        sqf::diag_log("KH Framework Extension - " + dll_name + " not found in standard locations");
                    } else if (_stricmp(dll_name.c_str(), "vulkan-1.dll") == 0) {
                        std::vector<std::string> vulkan_paths_to_try;
                        char vulkanPath[MAX_PATH];
                        DWORD result = GetEnvironmentVariableA("VULKAN_SDK", vulkanPath, MAX_PATH);
                        
                        if (result > 0 && result < MAX_PATH) {
                            vulkan_paths_to_try.push_back(std::string(vulkanPath) + "\\Bin\\");
                        }
                        
                        result = GetEnvironmentVariableA("VK_SDK_PATH", vulkanPath, MAX_PATH);
                        
                        if (result > 0 && result < MAX_PATH) {
                            vulkan_paths_to_try.push_back(std::string(vulkanPath) + "\\Bin\\");
                        }

                        vulkan_paths_to_try.push_back("C:\\Windows\\System32\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.4.309.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.4.304.1\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.296.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.290.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.283.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.280.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.275.0\\Bin\\");
                        vulkan_paths_to_try.push_back("C:\\VulkanSDK\\1.3.268.0\\Bin\\");
                        
                        for (const auto& path : vulkan_paths_to_try) {
                            std::string fullPath = path + dll_name;
                            HMODULE hVulkanDll = LoadLibraryA(fullPath.c_str());
                            
                            if (hVulkanDll != NULL) {
                                g_loaded_delay_modules[dll_name] = hVulkanDll;
                                return (FARPROC)hVulkanDll;
                            }
                        }

                        sqf::diag_log("KH Framework Extension - " + dll_name + " not found in standard locations");
                    } else if (_stricmp(dll_name.c_str(), "sherpa-onnx-c-api.dll") == 0) {
                        std::string directml_path = modDir + "\\DirectML.dll";
                        std::string onnxruntime_path = modDir + "\\onnxruntime.dll";
                        HMODULE hDirectML = LoadLibraryA(directml_path.c_str());

                        if (hDirectML != NULL) {
                            g_loaded_delay_modules["DirectML.dll"] = hDirectML;
                        }
                        
                        HMODULE hOnnx = LoadLibraryA(onnxruntime_path.c_str());

                        if (hOnnx != NULL) {
                            g_loaded_delay_modules["onnxruntime.dll"] = hOnnx;
                        }

                        HMODULE hDll = LoadLibraryA(dllFullPath.c_str());

                        if (hDll != NULL) {
                            g_loaded_delay_modules[dll_name] = hDll;
                            return (FARPROC)hDll;
                        } else {
                            DWORD error = GetLastError();
                            report_error("KH Framework Extension - Failed to load " + dll_name + " from " + dllFullPath + " - error code: " + std::to_string(error));
                        }

                        sqf::diag_log("KH Framework Extension - " + dll_name + " not found in standard locations");
                    } else if (_stricmp(dll_name.c_str(), "Ultralight.dll") == 0) {
                        std::string core_path = modDir + "\\UltralightCore.dll";
                        std::string webcore_path = modDir + "\\WebCore.dll";
                        HMODULE hCore = LoadLibraryA(core_path.c_str());

                        if (hCore != NULL) {
                            g_loaded_delay_modules["UltralightCore.dll"] = hCore;
                        }

                        HMODULE hWebCore = LoadLibraryA(webcore_path.c_str());

                        if (hWebCore != NULL) {
                            g_loaded_delay_modules["WebCore.dll"] = hWebCore;
                        }

                        HMODULE hDll = LoadLibraryA(dllFullPath.c_str());
                        
                        if (hDll != NULL) {
                            g_loaded_delay_modules[dll_name] = hDll;
                            return (FARPROC)hDll;
                        } else {
                            DWORD error = GetLastError();
                            report_error("KH Framework Extension - Failed to load " + dll_name + " from " + dllFullPath + " - error code: " + std::to_string(error));
                        }

                        sqf::diag_log("KH Framework Extension - " + dll_name + " not found in standard locations");
                    } else if (_stricmp(dll_name.c_str(), "WebCore.dll") == 0) {
                        std::string core_path = modDir + "\\UltralightCore.dll";
                        HMODULE hCore = LoadLibraryA(core_path.c_str());

                        if (hCore != NULL) {
                            g_loaded_delay_modules["UltralightCore.dll"] = hCore;
                        }

                        HMODULE hDll = LoadLibraryA(dllFullPath.c_str());
                        
                        if (hDll != NULL) {
                            g_loaded_delay_modules[dll_name] = hDll;
                            return (FARPROC)hDll;
                        } else {
                            DWORD error = GetLastError();
                            report_error("KH Framework Extension - Failed to load " + dll_name + " from " + dllFullPath + " - error code: " + std::to_string(error));
                        }

                        sqf::diag_log("KH Framework Extension - " + dll_name + " not found in standard locations");
                    } else if (_stricmp(dll_name.c_str(), "lua51.dll") == 0) {
                        HMODULE hDll = LoadLibraryA(dllFullPath.c_str());
                        
                        if (hDll != NULL) {
                            g_loaded_delay_modules[dll_name] = hDll;
                            return (FARPROC)hDll;
                        } else {
                            DWORD error = GetLastError();
                            report_error("KH Framework Extension - Failed to load " + dll_name + " from " + dllFullPath + " - error code: " + std::to_string(error));
                        }

                        report_error("KH Framework Extension - CRITICAL - " + dll_name + " failed to load - extension cannot function");
                    } else {
                        return NULL;
                    }
                }
            }
        }
    }
    else if (dliNotify == dliFailLoadLib) {
        std::string dll_name = pdli->szDll;
        
        if (_stricmp(dll_name.c_str(), "lua51.dll") == 0) {
            report_error("KH Framework Extension - CRITICAL - " + dll_name + " failed to load - extension cannot function");
        } else {
            report_error("KH Framework Extension - " + dll_name + " failed to load");
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
            DisableProcessWindowsGhosting();
            ShutdownWatchdog::instance().initialize();
            detect_gpu_backends();
            break;
        case DLL_PROCESS_DETACH:
            if (lpReserved != nullptr) {                                
                ShutdownWatchdog::instance().shutdown(true);
            } else {
                ShutdownWatchdog::instance().arm(3000);
                reset_lua_state();
                MainThreadScheduler::instance().clear();
                
                __try { 
                    if (AIFramework::instance().is_initialized()) {
                        AIFramework::instance().stop_all();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (TTSFramework::instance().is_initialized()) {
                        TTSFramework::instance().cleanup();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (STTFramework::instance().is_initialized_public()) {
                        STTFramework::instance().cleanup_public();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (UIFramework::instance().is_initialized()) {
                        UIFramework::instance().shutdown();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (NetworkFramework::instance().is_initialized()) {
                        NetworkFramework::instance().shutdown();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}

                __try {
                    if (TeamspeakFramework::instance().is_initialized()) {
                        TeamspeakFramework::instance().cleanup();
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
                
                __try { 
                    MH_Uninitialize(); 
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
                
                __try { 
                    cleanup_delay_loaded_modules(); 
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