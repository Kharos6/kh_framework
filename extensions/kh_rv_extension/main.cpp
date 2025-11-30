#define NOMINMAX

#include <windows.h>
#include <d3d11.h>
#include <cstring>
#include <cstdio>
#include <cstdint>

#define KH_RVEXT_VERSION "1.0.0"
#define KH_RVEXT_NAME "kh_rvextension"

enum DllExtensionFeatureFlags : uint64_t {
    RVFeature_ContextArgumentsVoidPtr = 1 << 0,    // Context args as void** instead of char**
    RVFeature_ContextStackTrace = 1 << 1,          // Include stack trace in context
    RVFeature_ContextNoDefaultCall = 1 << 2,       // Don't auto-call context, must request manually
    RVFeature_ArgumentNoEscapeString = 1 << 3,     // Args not escaped (better performance)
};

struct RVContext_StackTrace {
    struct StackTraceLine {
        uint32_t lineNumber;
        uint32_t fileOffset;
        const char* sourceFile;
        const char* scopeName;
        const char* fileContent;
    };
    
    StackTraceLine* lines = nullptr;
    uint32_t lineCount = 0;
};

enum TextureFormat : uint32_t {
    PacAI88 = 1,
    PacARGB8888 = 5
};

struct ProjectionViewTransform {
    typedef float M4x4[4][4];
    M4x4 projection;
    M4x4 view;
};

struct RVExtensionRenderInfo {
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dDeviceContext;
};

struct RVExtensionGraphicsLockGuard {
    virtual void ReleaseLock() const = 0;
    virtual ~RVExtensionGraphicsLockGuard() = default;
};


typedef int (__stdcall RVExtensionCallbackProc)(char const* name, char const* function, char const* data);
typedef void __stdcall RVExtensionRequestContextProc();
typedef RVExtensionGraphicsLockGuard* (__stdcall RVExtensionGLockProc)();
typedef void (__stdcall RVGetProjectionViewTransformProc)(ProjectionViewTransform& pvTransform);
typedef void (__stdcall RVExtensionGSetWHkProc)(bool (__stdcall *newHook)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam));
typedef void (__stdcall RVExtensionGSetResetHookProc)(void (__stdcall *newHook)());
typedef void (__stdcall RVExtensionGGetR2TTextureProc)(const char* name, void (__stdcall *onResult)(ID3D11Texture2D* texture, ID3D11ShaderResourceView* resView, 
    ID3D11RenderTargetView* rtView, uint32_t width, uint32_t height, uint32_t lastSeenFrameID, const char* name));

typedef void (*TextureSourceFillProc)(const char* uniqueName, uint16_t width, uint16_t height, 
    uint16_t pitch, uint8_t mipLevel, TextureFormat format, void* buffer);

typedef bool (*TextureSourceUpdateCheckProc)(const char* uniqueName);
static RVExtensionCallbackProc* g_callback_proc = nullptr;
static TextureSourceFillProc g_texture_source_fill_callback = nullptr;
static TextureSourceUpdateCheckProc g_texture_source_update_check_callback = nullptr;
static RVExtensionRequestContextProc* g_request_context_proc = nullptr;
static const RVExtensionRenderInfo* const* g_render_info = nullptr;
static RVExtensionGLockProc* g_glock_proc = nullptr;
static RVGetProjectionViewTransformProc* g_get_pv_transform_proc = nullptr;
static RVExtensionGSetWHkProc* g_set_window_hook_proc = nullptr;
static RVExtensionGSetResetHookProc* g_set_reset_hook_proc = nullptr;
static RVExtensionGGetR2TTextureProc* g_get_r2t_texture_proc = nullptr;
static bool g_game_functions_initialized = false;
static uint64_t g_context_steam_id = 0;
static char g_context_file_source[256] = {0};
static char g_context_mission_name[256] = {0};
static char g_context_server_name[256] = {0};
static int16_t g_context_remote_owner = 0;
static uint64_t g_callbacks_sent = 0;
static uint64_t g_callbacks_failed = 0;

static void safe_strcpy(char* dest, const char* src, size_t max_size) {
    if (!dest || !src || max_size == 0) return;
    size_t len = strlen(src);
    if (len >= max_size) len = max_size - 1;
    memcpy(dest, src, len);
    dest[len] = '\0';
}

inline const void* FindRVFunction(const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(GetModuleHandle(nullptr), name));
}

static void InitializeGameFunctions() {
    if (g_game_functions_initialized) return;
    
    g_request_context_proc = reinterpret_cast<RVExtensionRequestContextProc*>(
        FindRVFunction("RVExtensionRequestContext"));
    
    g_render_info = reinterpret_cast<const RVExtensionRenderInfo* const*>(
        FindRVFunction("RVExtensionGData"));
    
    g_glock_proc = reinterpret_cast<RVExtensionGLockProc*>(
        FindRVFunction("RVExtensionGLock"));
    
    g_get_pv_transform_proc = reinterpret_cast<RVGetProjectionViewTransformProc*>(
        FindRVFunction("RVGetProjectionViewTransform"));
    
    g_set_window_hook_proc = reinterpret_cast<RVExtensionGSetWHkProc*>(
        FindRVFunction("RVExtensionGSetWHk"));
    
    g_set_reset_hook_proc = reinterpret_cast<RVExtensionGSetResetHookProc*>(
        FindRVFunction("RVExtensionGSetResetHook"));
    
    g_get_r2t_texture_proc = reinterpret_cast<RVExtensionGGetR2TTextureProc*>(
        FindRVFunction("RVExtensionGGetR2TTexture"));
    
    g_game_functions_initialized = true;
}

extern "C" {
__declspec(dllexport) RVExtensionCallbackProc* KH_GetCallbackProc() {
    return g_callback_proc;
}

__declspec(dllexport) bool KH_IsCallbackReady() {
    return g_callback_proc != nullptr;
}

__declspec(dllexport) int KH_Callback(const char* name, const char* function, const char* data) {
    if (!g_callback_proc) {
        g_callbacks_failed++;
        return -1;
    }
    
    int result = g_callback_proc(name, function, data);
    
    if (result >= 0) {
        g_callbacks_sent++;
    } else {
        g_callbacks_failed++;
    }
    
    return result;
}

__declspec(dllexport) RVExtensionRequestContextProc* KH_GetRequestContextProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_request_context_proc;
}

__declspec(dllexport) void KH_RequestContext() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_request_context_proc) {
        g_request_context_proc();
    }
}

__declspec(dllexport) uint64_t KH_GetContextSteamId() {
    return g_context_steam_id;
}

__declspec(dllexport) const char* KH_GetContextFileSource() {
    return g_context_file_source;
}

__declspec(dllexport) const char* KH_GetContextMissionName() {
    return g_context_mission_name;
}

__declspec(dllexport) const char* KH_GetContextServerName() {
    return g_context_server_name;
}

__declspec(dllexport) int16_t KH_GetContextRemoteOwner() {
    return g_context_remote_owner;
}

__declspec(dllexport) const RVExtensionRenderInfo* KH_GetRenderInfo() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_render_info && *g_render_info) {
        return *g_render_info;
    }
    return nullptr;
}

__declspec(dllexport) ID3D11Device* KH_GetD3DDevice() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_render_info && *g_render_info) {
        return (*g_render_info)->d3dDevice;
    }
    return nullptr;
}

__declspec(dllexport) ID3D11DeviceContext* KH_GetD3DDeviceContext() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_render_info && *g_render_info) {
        return (*g_render_info)->d3dDeviceContext;
    }
    return nullptr;
}

__declspec(dllexport) RVExtensionGLockProc* KH_GetGLockProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_glock_proc;
}

__declspec(dllexport) RVExtensionGraphicsLockGuard* KH_AcquireGraphicsLock() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_glock_proc) {
        return g_glock_proc();
    }
    return nullptr;
}

__declspec(dllexport) RVGetProjectionViewTransformProc* KH_GetProjectionViewTransformProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_get_pv_transform_proc;
}

__declspec(dllexport) bool KH_GetProjectionViewTransform(ProjectionViewTransform* pvTransform) {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_get_pv_transform_proc && pvTransform) {
        g_get_pv_transform_proc(*pvTransform);
        return true;
    }
    return false;
}

__declspec(dllexport) RVExtensionGSetWHkProc* KH_GetSetWindowHookProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_set_window_hook_proc;
}

__declspec(dllexport) void KH_SetWindowHook(bool (__stdcall *hook)(HWND, UINT, WPARAM, LPARAM)) {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_set_window_hook_proc) {
        g_set_window_hook_proc(hook);
    }
}

__declspec(dllexport) RVExtensionGSetResetHookProc* KH_GetSetResetHookProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_set_reset_hook_proc;
}

__declspec(dllexport) void KH_SetResetHook(void (__stdcall *hook)()) {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_set_reset_hook_proc) {
        g_set_reset_hook_proc(hook);
    }
}

__declspec(dllexport) RVExtensionGGetR2TTextureProc* KH_GetR2TTextureProc() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_get_r2t_texture_proc;
}

__declspec(dllexport) void KH_GetR2TTexture(const char* name, 
    void (__stdcall *onResult)(ID3D11Texture2D*, ID3D11ShaderResourceView*, ID3D11RenderTargetView*, 
        uint32_t, uint32_t, uint32_t, const char*)) {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    if (g_get_r2t_texture_proc) {
        g_get_r2t_texture_proc(name, onResult);
    }
}

__declspec(dllexport) bool KH_HasRequestContext() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_request_context_proc != nullptr;
}

__declspec(dllexport) bool KH_HasGraphics() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_render_info != nullptr && *g_render_info != nullptr;
}

__declspec(dllexport) bool KH_HasGraphicsLock() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_glock_proc != nullptr;
}

__declspec(dllexport) bool KH_HasProjectionViewTransform() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_get_pv_transform_proc != nullptr;
}

__declspec(dllexport) bool KH_HasWindowHook() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_set_window_hook_proc != nullptr;
}

__declspec(dllexport) bool KH_HasResetHook() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_set_reset_hook_proc != nullptr;
}

__declspec(dllexport) bool KH_HasR2TTexture() {
    if (!g_game_functions_initialized) InitializeGameFunctions();
    return g_get_r2t_texture_proc != nullptr;
}

__declspec(dllexport) void KH_SetTextureSourceFillCallback(TextureSourceFillProc callback) {
    g_texture_source_fill_callback = callback;
}

__declspec(dllexport) void KH_SetTextureSourceUpdateCheckCallback(TextureSourceUpdateCheckProc callback) {
    g_texture_source_update_check_callback = callback;
}

__declspec(dllexport) TextureSourceFillProc KH_GetTextureSourceFillCallback() {
    return g_texture_source_fill_callback;
}

__declspec(dllexport) TextureSourceUpdateCheckProc KH_GetTextureSourceUpdateCheckCallback() {
    return g_texture_source_update_check_callback;
}

__declspec(dllexport) void KH_GetStats(uint64_t* sent, uint64_t* failed) {
    if (sent) *sent = g_callbacks_sent;
    if (failed) *failed = g_callbacks_failed;
}

__declspec(dllexport) void KH_InitializeGameFunctions() {
    InitializeGameFunctions();
}

__declspec(dllexport) void __stdcall RVExtensionVersion(char* output, unsigned int outputSize) {
    if (!output || outputSize == 0) return;
    std::strncpy(output, KH_RVEXT_NAME " v" KH_RVEXT_VERSION, outputSize - 1);
    output[outputSize - 1] = '\0';
    
    // Initialize game functions when version is queried (early initialization)
    InitializeGameFunctions();
}

__declspec(dllexport) void __stdcall RVExtensionRegisterCallback(RVExtensionCallbackProc* callbackProc) {
    g_callback_proc = callbackProc;
}

__declspec(dllexport) void __stdcall RVExtensionContext(const char** args, unsigned int argsCnt) {
    if (argsCnt >= 1 && args[0]) {
        g_context_steam_id = strtoull(args[0], nullptr, 10);
    }

    if (argsCnt >= 2 && args[1]) {
        safe_strcpy(g_context_file_source, args[1], sizeof(g_context_file_source));
    }

    if (argsCnt >= 3 && args[2]) {
        safe_strcpy(g_context_mission_name, args[2], sizeof(g_context_mission_name));
    }

    if (argsCnt >= 4 && args[3]) {
        safe_strcpy(g_context_server_name, args[3], sizeof(g_context_server_name));
    }

    if (argsCnt >= 5 && args[4]) {
        g_context_remote_owner = static_cast<int16_t>(atoi(args[4]));
    }
}

// STRING callExtension STRING
__declspec(dllexport) void __stdcall RVExtension(char* output, unsigned int outputSize, const char* function) {
    if (!output || outputSize == 0) return;
    output[0] = '\0';
    
    if (!function || !function[0]) {
        safe_strcpy(output, "ERROR: Empty function", outputSize);
        return;
    }
    
    if (strcmp(function, "version") == 0) {
        snprintf(output, outputSize, "[\"%s\",\"%s\"]", KH_RVEXT_NAME, KH_RVEXT_VERSION);
        return;
    }
    
    if (strcmp(function, "status") == 0) {
        snprintf(output, outputSize, "[%s,%llu,%llu]",
                 g_callback_proc ? "true" : "false",
                 static_cast<unsigned long long>(g_callbacks_sent),
                 static_cast<unsigned long long>(g_callbacks_failed));
                 
        return;
    }
    
    if (strcmp(function, "ready") == 0) {
        safe_strcpy(output, g_callback_proc ? "true" : "false", outputSize);
        return;
    }
    
    if (strcmp(function, "capabilities") == 0) {
        if (!g_game_functions_initialized) InitializeGameFunctions();

        snprintf(output, outputSize, "[%s,%s,%s,%s,%s,%s,%s]",
                 g_callback_proc ? "true" : "false",
                 g_request_context_proc ? "true" : "false",
                 (g_render_info && *g_render_info) ? "true" : "false",
                 g_glock_proc ? "true" : "false",
                 g_get_pv_transform_proc ? "true" : "false",
                 g_set_window_hook_proc ? "true" : "false",
                 g_set_reset_hook_proc ? "true" : "false");

        return;
    }
    
    snprintf(output, outputSize, "ERROR: Unknown command '%s'", function);
}

// STRING callExtension ARRAY
__declspec(dllexport) int __stdcall RVExtensionArgs(
    char* output, unsigned int outputSize, 
    const char* function, const char** argv, unsigned int argc) {
    if (!output || outputSize == 0) return -1;
    output[0] = '\0';
    
    if (!function || !function[0]) {
        safe_strcpy(output, "ERROR: Empty function", outputSize);
        return -1;
    }
    
    if (strcmp(function, "callback") == 0) {
        if (argc < 3) {
            safe_strcpy(output, "ERROR: callback requires [name, function, data]", outputSize);
            return -1;
        }
        
        const char* name = argv[0] ? argv[0] : "";
        const char* func = argv[1] ? argv[1] : "";
        const char* data = argv[2] ? argv[2] : "";
        int result = KH_Callback(name, func, data);
        snprintf(output, outputSize, "%d", result);
        return result >= 0 ? 0 : -1;
    }
    
    snprintf(output, outputSize, "ERROR: Unknown function '%s'", function);
    return -1;
}

__declspec(dllexport) uint64_t RVExtensionFeatureFlags = 
    RVFeature_ContextNoDefaultCall | RVFeature_ArgumentNoEscapeString;

__declspec(dllexport) void __stdcall RVExtensionFillTextureSource(
    const char* uniqueName, uint16_t width, uint16_t height, 
    uint16_t pitch, uint8_t mipLevel, TextureFormat format, void* buffer) {
    
    if (g_texture_source_fill_callback) {
        g_texture_source_fill_callback(uniqueName, width, height, pitch, mipLevel, format, buffer);
    }
}

__declspec(dllexport) bool __stdcall RVExtensionHasTextureSourceUpdate(const char* uniqueName) {
    if (g_texture_source_update_check_callback) {
        return g_texture_source_update_check_callback(uniqueName);
    }

    return false;
}

} // extern "C"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableProcessWindowsGhosting();
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}