#ifndef RV_EXTENSION_UTILS_H
#define RV_EXTENSION_UTILS_H

/* bitset */
typedef enum {
    RVFeature_ContextArgumentsVoidPtr = 1,      /* 1 << 0 */
    RVFeature_ContextStackTrace = 2,            /* 1 << 1 */
    RVFeature_ContextNoDefaultCall = 4,         /* 1 << 2 */
    RVFeature_ArgumentNoEscapeString = 8        /* 1 << 3 */
} DllExtensionFeatureFlags;

struct StackTraceLine {
    /* Line number in file (before preprocessing if preprocessed with line numbers) */
    uint32_t lineNumber;
    /* File offset in bytes from the start of the file (after preprocessing) */
    uint32_t fileOffset;
    /* Filepath to the source file */
    const char* sourceFile;
    /* scopeName set on that level */
    const char* scopeName;
    /* Complete fileContent of the sourceFile (after preprocessing, can be combined with fileOffset to find exact location) */
    const char* fileContent;
};

struct RVContext_StackTrace {
    struct StackTraceLine* lines;
    uint32_t lineCount;
};

typedef float M4x4[4][4];

struct ProjectionViewTransform {
    M4x4 projection;
    M4x4 view;
};

typedef enum {
    CExtensionControlInterface_None = 0,
    CExtensionControlInterface_DynamicTexture = 1,      /* If set we do not set render target, and instead pass a texture that can be mapped with D3D11_MAP_WRITE_DISCARD */
    CExtensionControlInterface_TextureFormatBGRA = 2    /* If set the texture is BGRA, if not set its ARGB */
} CExtensionControlInterface_Flags;

struct CExtensionControlInterface {
    uint32_t size;
    CExtensionControlInterface_Flags flags;
    
    /* This is passed to every callback */
    void* context;
    
    /* [required] */
    /* Render into currently active target */
    /* target is a ID3D11RenderTargetView if Flags::DynamicTexture is not set */
    /* target is a ID3D11Texture2D if Flags::DynamicTexture is set */
    void (*OnDraw)(void* context, float alpha, void* target);
    /* [required] */
    /* Called after "Unload" display EH and "Destroy" control EH */
    /* This is the last call before the control is destroyed. Release all your resources */
    void (*OnDestroy)(void* context, int code);
    
    /* optional */
    /* Called before OnDraw, if the control (including backing render target) size has changed */
    void (*OnSizeChanged)(void* context, unsigned int width, unsigned int height);
    
    /* Interactions */
    /* Optional all of these */
    
    /* Return if you accept the focus */
    int (*OnSetFocus)(void* context, int focus);  /* changed bool to int */
    
    void (*OnLButtonDown)(void* context, float x, float y);
    void (*OnLButtonUp)(void* context, float x, float y);
    void (*OnLButtonClick)(void* context, float x, float y);
    void (*OnLButtonDblClick)(void* context, float x, float y);
    void (*OnRButtonDown)(void* context, float x, float y);
    void (*OnRButtonUp)(void* context, float x, float y);
    void (*OnRButtonClick)(void* context, float x, float y);
    void (*OnMouseMove)(void* context, float x, float y);
    /* Return if the input was handled, if not it will be passed to the parent controlsgroup/display */
    int (*OnMouseZChanged)(void* context, float dz);  /* changed bool to int */
    
    void (*OnMouseEnter)(void* context, float x, float y);
    void (*OnMouseExit)(void* context, float x, float y);
    
    /* Return if the key was handled, if not it will be passed to the parent controlsgroup/display */
    int (*OnKeyDown)(void* context, int dikCode);     /* changed bool to int */
    int (*OnKeyUp)(void* context, int dikCode);       /* changed bool to int */
    int (*OnChar)(void* context, unsigned nChar, unsigned nRepCnt, unsigned nFlags);  /* changed bool to int */
};

/* Helper function to initialize CExtensionControlInterface */
static inline void CExtensionControlInterface_Init(struct CExtensionControlInterface* iface) {
    if (iface) {
        memset(iface, 0, sizeof(struct CExtensionControlInterface));
        iface->size = sizeof(struct CExtensionControlInterface);
        iface->flags = CExtensionControlInterface_None;
    }
}

#ifdef _D3D11_CONSTANTS /* Only available with d3d11.h header */
struct RVExtensionRenderInfo {
    void* d3dDevice;            /* ID3D11Device* */
    void* d3dDeviceContext;     /* ID3D11DeviceContext* */
};

struct RVExtensionGraphicsLockGuard {
    /* Call this when done with the lock, forgetting to call this will freeze the game. */
    void (*ReleaseLock)(const void* self);
    void (*Destroy)(void* self);
};
#endif

#ifdef _WIN32
#define DLLEXPORT __declspec (dllexport)
#define CALL_CONVENTION __stdcall
#endif

typedef int (CALL_CONVENTION *RVExtensionCallbackProc)(const char* name, const char* function, const char* data);
typedef void (CALL_CONVENTION *RVExtensionRequestContextProc)(void);
typedef void (CALL_CONVENTION *RVGetProjectionViewTransformProc)(struct ProjectionViewTransform* pvTransform);

#ifdef _D3D11_CONSTANTS /* Only available with d3d11.h header */
typedef struct RVExtensionGraphicsLockGuard* (CALL_CONVENTION *RVExtensionGLockProc)(void);
typedef void (CALL_CONVENTION *RVExtensionGSetWHkProc)(int (CALL_CONVENTION *newHook)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam));
#endif

/*--- Called by Engine on extension load */
DLLEXPORT void CALL_CONVENTION RVExtensionVersion(char* output, unsigned int outputSize);
/*--- STRING callExtension STRING */
DLLEXPORT void CALL_CONVENTION RVExtension(char* output, unsigned int outputSize, const char* function);
/*--- STRING callExtension ARRAY */
DLLEXPORT int CALL_CONVENTION RVExtensionArgs(char* output, unsigned int outputSize, const char* function, const char** argv, unsigned int argc);
/*--- Called by Engine on extension load to pass RVExtensionCallbackProc to it */
DLLEXPORT void CALL_CONVENTION RVExtensionRegisterCallback(RVExtensionCallbackProc callbackProc);
/*--- Request creation of UI element */
DLLEXPORT int CALL_CONVENTION RVExtensionRequestUI(const char* uiClass, struct CExtensionControlInterface* interfaceStruct);

/*--- Finds a game exported function by its name */
static inline const void* FindRVFunction(const char* name) {
    return (void*)GetProcAddress(GetModuleHandle(NULL), name);
}

#endif /* RV_EXTENSION_UTILS_H */