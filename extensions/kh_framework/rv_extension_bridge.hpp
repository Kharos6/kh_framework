#pragma once

namespace RVExtBridge {

enum TextureFormat : uint32_t {
    PacAI88 = 1,
    PacARGB8888 = 5
};

struct ProjectionViewTransform {
    typedef float M4x4[4][4];
    M4x4 projection;
    M4x4 view;
};

struct RenderInfo {
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dDeviceContext;
};

struct GraphicsLockGuard {
    virtual void ReleaseLock() const = 0;
    virtual ~GraphicsLockGuard() = default;
};

typedef int (__stdcall RVExtensionCallbackProc)(char const* name, char const* function, char const* data);

typedef void (*TextureSourceFillProc)(const char* uniqueName, uint16_t width, uint16_t height, 
    uint16_t pitch, uint8_t mipLevel, TextureFormat format, void* buffer);

typedef bool (*TextureSourceUpdateCheckProc)(const char* uniqueName);

// Callback
typedef RVExtensionCallbackProc* (*GetCallbackProcFn)();
typedef bool (*IsCallbackReadyFn)();
typedef int (*CallbackFn)(const char* name, const char* function, const char* data);

// Context
typedef void* (*GetRequestContextProcFn)();
typedef void (*RequestContextFn)();
typedef uint64_t (*GetContextSteamIdFn)();
typedef const char* (*GetContextStringFn)();
typedef int16_t (*GetContextRemoteOwnerFn)();

// Graphics
typedef const RenderInfo* (*GetRenderInfoFn)();
typedef ID3D11Device* (*GetD3DDeviceFn)();
typedef ID3D11DeviceContext* (*GetD3DDeviceContextFn)();
typedef void* (*GetGLockProcFn)();
typedef GraphicsLockGuard* (*AcquireGraphicsLockFn)();
typedef void* (*GetProjectionViewTransformProcFn)();
typedef bool (*GetProjectionViewTransformFn)(ProjectionViewTransform* pvTransform);
typedef void* (*GetSetWindowHookProcFn)();
typedef void (*SetWindowHookFn)(bool (__stdcall *hook)(HWND, UINT, WPARAM, LPARAM));
typedef void* (*GetSetResetHookProcFn)();
typedef void (*SetResetHookFn)(void (__stdcall *hook)());
typedef void* (*GetR2TTextureProcFn)();
typedef void (*GetR2TTextureFn)(const char* name, 
    void (__stdcall *onResult)(ID3D11Texture2D*, ID3D11ShaderResourceView*, ID3D11RenderTargetView*, 
        uint32_t, uint32_t, uint32_t, const char*));

// Texture source
typedef void (*SetTextureSourceFillCallbackFn)(TextureSourceFillProc callback);
typedef void (*SetTextureSourceUpdateCheckCallbackFn)(TextureSourceUpdateCheckProc callback);
typedef TextureSourceFillProc (*GetTextureSourceFillCallbackFn)();
typedef TextureSourceUpdateCheckProc (*GetTextureSourceUpdateCheckCallbackFn)();

// Availability checks
typedef bool (*HasFeatureFn)();

// Stats
typedef void (*GetStatsFn)(uint64_t* sent, uint64_t* failed);

// Initialize
typedef void (*InitializeGameFunctionsFn)();

// Bridge
static HMODULE g_bridge_module = nullptr;
static bool g_initialized = false;

// Callback functions
static GetCallbackProcFn g_fn_get_callback_proc = nullptr;
static IsCallbackReadyFn g_fn_is_callback_ready = nullptr;
static CallbackFn g_fn_callback = nullptr;

// Context functions
static GetRequestContextProcFn g_fn_get_request_context_proc = nullptr;
static RequestContextFn g_fn_request_context = nullptr;
static GetContextSteamIdFn g_fn_get_context_steam_id = nullptr;
static GetContextStringFn g_fn_get_context_file_source = nullptr;
static GetContextStringFn g_fn_get_context_mission_name = nullptr;
static GetContextStringFn g_fn_get_context_server_name = nullptr;
static GetContextRemoteOwnerFn g_fn_get_context_remote_owner = nullptr;

// Graphics functions
static GetRenderInfoFn g_fn_get_render_info = nullptr;
static GetD3DDeviceFn g_fn_get_d3d_device = nullptr;
static GetD3DDeviceContextFn g_fn_get_d3d_device_context = nullptr;
static GetGLockProcFn g_fn_get_glock_proc = nullptr;
static AcquireGraphicsLockFn g_fn_acquire_graphics_lock = nullptr;
static GetProjectionViewTransformProcFn g_fn_get_pv_transform_proc = nullptr;
static GetProjectionViewTransformFn g_fn_get_pv_transform = nullptr;
static GetSetWindowHookProcFn g_fn_get_set_window_hook_proc = nullptr;
static SetWindowHookFn g_fn_set_window_hook = nullptr;
static GetSetResetHookProcFn g_fn_get_set_reset_hook_proc = nullptr;
static SetResetHookFn g_fn_set_reset_hook = nullptr;
static GetR2TTextureProcFn g_fn_get_r2t_texture_proc = nullptr;
static GetR2TTextureFn g_fn_get_r2t_texture = nullptr;

// Texture source functions
static SetTextureSourceFillCallbackFn g_fn_set_texture_source_fill_callback = nullptr;
static SetTextureSourceUpdateCheckCallbackFn g_fn_set_texture_source_update_check_callback = nullptr;
static GetTextureSourceFillCallbackFn g_fn_get_texture_source_fill_callback = nullptr;
static GetTextureSourceUpdateCheckCallbackFn g_fn_get_texture_source_update_check_callback = nullptr;

// Availability checks
static HasFeatureFn g_fn_has_request_context = nullptr;
static HasFeatureFn g_fn_has_graphics = nullptr;
static HasFeatureFn g_fn_has_graphics_lock = nullptr;
static HasFeatureFn g_fn_has_pv_transform = nullptr;
static HasFeatureFn g_fn_has_window_hook = nullptr;
static HasFeatureFn g_fn_has_reset_hook = nullptr;
static HasFeatureFn g_fn_has_r2t_texture = nullptr;

// Stats and init
static GetStatsFn g_fn_get_stats = nullptr;
static InitializeGameFunctionsFn g_fn_initialize_game_functions = nullptr;

inline bool initialize() {
    if (g_initialized) return true;
    
    // Try to get the already-loaded module
    g_bridge_module = GetModuleHandleA("kh_rv_extension_x64.dll");
    
    if (!g_bridge_module) {
        // Module not loaded yet, try to load from same directory
        char path[MAX_PATH];
        HMODULE this_module = nullptr;
        
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&initialize), &this_module)) {
            if (GetModuleFileNameA(this_module, path, MAX_PATH)) {
                std::string dir(path);
                size_t pos = dir.find_last_of("\\/");

                if (pos != std::string::npos) {
                    dir = dir.substr(0, pos + 1);
                    std::string bridge_path = dir + "kh_rv_extension_x64.dll";
                    g_bridge_module = LoadLibraryA(bridge_path.c_str());
                }
            }
        }
    }
    
    if (!g_bridge_module) {
        return false;
    }

    #define GET_PROC(name, type) reinterpret_cast<type>(GetProcAddress(g_bridge_module, name))
    
    // Callback
    g_fn_get_callback_proc = GET_PROC("KH_GetCallbackProc", GetCallbackProcFn);
    g_fn_is_callback_ready = GET_PROC("KH_IsCallbackReady", IsCallbackReadyFn);
    g_fn_callback = GET_PROC("KH_Callback", CallbackFn);
    
    // Context
    g_fn_get_request_context_proc = GET_PROC("KH_GetRequestContextProc", GetRequestContextProcFn);
    g_fn_request_context = GET_PROC("KH_RequestContext", RequestContextFn);
    g_fn_get_context_steam_id = GET_PROC("KH_GetContextSteamId", GetContextSteamIdFn);
    g_fn_get_context_file_source = GET_PROC("KH_GetContextFileSource", GetContextStringFn);
    g_fn_get_context_mission_name = GET_PROC("KH_GetContextMissionName", GetContextStringFn);
    g_fn_get_context_server_name = GET_PROC("KH_GetContextServerName", GetContextStringFn);
    g_fn_get_context_remote_owner = GET_PROC("KH_GetContextRemoteOwner", GetContextRemoteOwnerFn);
    
    // Graphics
    g_fn_get_render_info = GET_PROC("KH_GetRenderInfo", GetRenderInfoFn);
    g_fn_get_d3d_device = GET_PROC("KH_GetD3DDevice", GetD3DDeviceFn);
    g_fn_get_d3d_device_context = GET_PROC("KH_GetD3DDeviceContext", GetD3DDeviceContextFn);
    g_fn_get_glock_proc = GET_PROC("KH_GetGLockProc", GetGLockProcFn);
    g_fn_acquire_graphics_lock = GET_PROC("KH_AcquireGraphicsLock", AcquireGraphicsLockFn);
    g_fn_get_pv_transform_proc = GET_PROC("KH_GetProjectionViewTransformProc", GetProjectionViewTransformProcFn);
    g_fn_get_pv_transform = GET_PROC("KH_GetProjectionViewTransform", GetProjectionViewTransformFn);
    g_fn_get_set_window_hook_proc = GET_PROC("KH_GetSetWindowHookProc", GetSetWindowHookProcFn);
    g_fn_set_window_hook = GET_PROC("KH_SetWindowHook", SetWindowHookFn);
    g_fn_get_set_reset_hook_proc = GET_PROC("KH_GetSetResetHookProc", GetSetResetHookProcFn);
    g_fn_set_reset_hook = GET_PROC("KH_SetResetHook", SetResetHookFn);
    g_fn_get_r2t_texture_proc = GET_PROC("KH_GetR2TTextureProc", GetR2TTextureProcFn);
    g_fn_get_r2t_texture = GET_PROC("KH_GetR2TTexture", GetR2TTextureFn);
    
    // Texture source
    g_fn_set_texture_source_fill_callback = GET_PROC("KH_SetTextureSourceFillCallback", SetTextureSourceFillCallbackFn);
    g_fn_set_texture_source_update_check_callback = GET_PROC("KH_SetTextureSourceUpdateCheckCallback", SetTextureSourceUpdateCheckCallbackFn);
    g_fn_get_texture_source_fill_callback = GET_PROC("KH_GetTextureSourceFillCallback", GetTextureSourceFillCallbackFn);
    g_fn_get_texture_source_update_check_callback = GET_PROC("KH_GetTextureSourceUpdateCheckCallback", GetTextureSourceUpdateCheckCallbackFn);
    
    // Availability
    g_fn_has_request_context = GET_PROC("KH_HasRequestContext", HasFeatureFn);
    g_fn_has_graphics = GET_PROC("KH_HasGraphics", HasFeatureFn);
    g_fn_has_graphics_lock = GET_PROC("KH_HasGraphicsLock", HasFeatureFn);
    g_fn_has_pv_transform = GET_PROC("KH_HasProjectionViewTransform", HasFeatureFn);
    g_fn_has_window_hook = GET_PROC("KH_HasWindowHook", HasFeatureFn);
    g_fn_has_reset_hook = GET_PROC("KH_HasResetHook", HasFeatureFn);
    g_fn_has_r2t_texture = GET_PROC("KH_HasR2TTexture", HasFeatureFn);
    
    // Stats and init
    g_fn_get_stats = GET_PROC("KH_GetStats", GetStatsFn);
    g_fn_initialize_game_functions = GET_PROC("KH_InitializeGameFunctions", InitializeGameFunctionsFn);
    
    #undef GET_PROC

    if (!g_fn_callback || !g_fn_is_callback_ready) {
        g_bridge_module = nullptr;
        return false;
    }

    if (g_fn_initialize_game_functions) {
        g_fn_initialize_game_functions();
    }
    
    g_initialized = true;
    return true;
}

inline bool is_initialized() {
    return g_initialized;
}

inline bool is_ready() {
    if (!g_initialized || !g_fn_is_callback_ready) return false;
    return g_fn_is_callback_ready();
}

inline RVExtensionCallbackProc* get_callback_proc() {
    if (!g_initialized || !g_fn_get_callback_proc) return nullptr;
    return g_fn_get_callback_proc();
}

inline int callback(const char* name, const char* function, const char* data) {
    if (!g_initialized || !g_fn_callback) return -1;
    return g_fn_callback(name, function, data);
}

inline int callback(const std::string& name, const std::string& function, const std::string& data) {
    return callback(name.c_str(), function.c_str(), data.c_str());
}

static RVExtensionCallbackProc* g_direct_callback = nullptr;

inline void refresh_callback_ptr() {
    if (g_initialized && g_fn_get_callback_proc) {
        g_direct_callback = g_fn_get_callback_proc();
    }
}

inline int callback_direct(const char* name, const char* function, const char* data) {
    if (!g_direct_callback) return -1;
    return g_direct_callback(name, function, data);
}

inline bool has_request_context() {
    if (!g_initialized || !g_fn_has_request_context) return false;
    return g_fn_has_request_context();
}

inline void request_context() {
    if (g_initialized && g_fn_request_context) {
        g_fn_request_context();
    }
}

inline uint64_t get_context_steam_id() {
    if (!g_initialized || !g_fn_get_context_steam_id) return 0;
    return g_fn_get_context_steam_id();
}

inline const char* get_context_file_source() {
    if (!g_initialized || !g_fn_get_context_file_source) return "";
    return g_fn_get_context_file_source();
}

inline const char* get_context_mission_name() {
    if (!g_initialized || !g_fn_get_context_mission_name) return "";
    return g_fn_get_context_mission_name();
}

inline const char* get_context_server_name() {
    if (!g_initialized || !g_fn_get_context_server_name) return "";
    return g_fn_get_context_server_name();
}

inline int16_t get_context_remote_owner() {
    if (!g_initialized || !g_fn_get_context_remote_owner) return 0;
    return g_fn_get_context_remote_owner();
}

inline bool has_graphics() {
    if (!g_initialized || !g_fn_has_graphics) return false;
    return g_fn_has_graphics();
}

inline const RenderInfo* get_render_info() {
    if (!g_initialized || !g_fn_get_render_info) return nullptr;
    return g_fn_get_render_info();
}

inline ID3D11Device* get_d3d_device() {
    if (!g_initialized || !g_fn_get_d3d_device) return nullptr;
    return g_fn_get_d3d_device();
}

inline ID3D11DeviceContext* get_d3d_device_context() {
    if (!g_initialized || !g_fn_get_d3d_device_context) return nullptr;
    return g_fn_get_d3d_device_context();
}

inline bool has_graphics_lock() {
    if (!g_initialized || !g_fn_has_graphics_lock) return false;
    return g_fn_has_graphics_lock();
}

inline GraphicsLockGuard* acquire_graphics_lock() {
    if (!g_initialized || !g_fn_acquire_graphics_lock) return nullptr;
    return g_fn_acquire_graphics_lock();
}

// RAII wrapper for graphics lock
class ScopedGraphicsLock {
    GraphicsLockGuard* lock_ = nullptr;
public:
    ScopedGraphicsLock() : lock_(acquire_graphics_lock()) {}
    ~ScopedGraphicsLock() { if (lock_) lock_->ReleaseLock(); }
    ScopedGraphicsLock(const ScopedGraphicsLock&) = delete;
    ScopedGraphicsLock& operator=(const ScopedGraphicsLock&) = delete;
    bool acquired() const { return lock_ != nullptr; }
    void release() { if (lock_) { lock_->ReleaseLock(); lock_ = nullptr; } }
};

inline bool has_projection_view_transform() {
    if (!g_initialized || !g_fn_has_pv_transform) return false;
    return g_fn_has_pv_transform();
}

inline bool get_projection_view_transform(ProjectionViewTransform& pvTransform) {
    if (!g_initialized || !g_fn_get_pv_transform) return false;
    return g_fn_get_pv_transform(&pvTransform);
}

inline bool has_window_hook() {
    if (!g_initialized || !g_fn_has_window_hook) return false;
    return g_fn_has_window_hook();
}

inline void set_window_hook(bool (__stdcall *hook)(HWND, UINT, WPARAM, LPARAM)) {
    if (g_initialized && g_fn_set_window_hook) {
        g_fn_set_window_hook(hook);
    }
}

inline bool has_reset_hook() {
    if (!g_initialized || !g_fn_has_reset_hook) return false;
    return g_fn_has_reset_hook();
}

inline void set_reset_hook(void (__stdcall *hook)()) {
    if (g_initialized && g_fn_set_reset_hook) {
        g_fn_set_reset_hook(hook);
    }
}

inline bool has_r2t_texture() {
    if (!g_initialized || !g_fn_has_r2t_texture) return false;
    return g_fn_has_r2t_texture();
}

inline void get_r2t_texture(const char* name, 
    void (__stdcall *onResult)(ID3D11Texture2D*, ID3D11ShaderResourceView*, ID3D11RenderTargetView*, 
        uint32_t, uint32_t, uint32_t, const char*)) {
    if (g_initialized && g_fn_get_r2t_texture) {
        g_fn_get_r2t_texture(name, onResult);
    }
}

// Callback will receive: uniqueName, width, height, pitch, mipLevel, format, buffer
// Buffer must be filled with pixel data
inline void set_texture_source_fill_callback(TextureSourceFillProc callback) {
    if (g_initialized && g_fn_set_texture_source_fill_callback) {
        g_fn_set_texture_source_fill_callback(callback);
    }
}

inline void set_texture_source_update_check_callback(TextureSourceUpdateCheckProc callback) {
    if (g_initialized && g_fn_set_texture_source_update_check_callback) {
        g_fn_set_texture_source_update_check_callback(callback);
    }
}

inline TextureSourceFillProc get_texture_source_fill_callback() {
    if (!g_initialized || !g_fn_get_texture_source_fill_callback) return nullptr;
    return g_fn_get_texture_source_fill_callback();
}

inline TextureSourceUpdateCheckProc get_texture_source_update_check_callback() {
    if (!g_initialized || !g_fn_get_texture_source_update_check_callback) return nullptr;
    return g_fn_get_texture_source_update_check_callback();
}

inline void get_stats(uint64_t& sent, uint64_t& failed) {
    if (!g_initialized || !g_fn_get_stats) {
        sent = 0;
        failed = 0;
        return;
    }
    g_fn_get_stats(&sent, &failed);
}

} // namespace RVExtBridge