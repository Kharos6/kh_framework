/* Lua framework communication */
typedef const char* (*KHLuaFrameworkInterface_t)(const char* function, const char* parameters);
static HMODULE g_lua_framework_module = NULL;
static KHLuaFrameworkInterface_t g_lua_interface = NULL;

/* Initialize Lua framework communication */
static void kh_init_lua_framework_communication(void) {
    if (g_lua_framework_module) return; /* Already initialized */
    
    /* Try to load the Lua framework DLL */
    g_lua_framework_module = GetModuleHandle("kh_framework_lua_x64.dll");
    
    if (g_lua_framework_module) {
        g_lua_interface = (KHLuaFrameworkInterface_t)GetProcAddress(
            g_lua_framework_module, "KHLuaFrameworkInterface");
    }
}

/* Parse and validate parameters for Lua communication */
static char* kh_parse_lua_parameters(const char* params_str) {
    if (!params_str) return NULL;
    
    size_t len = strlen(params_str);
    char* result = (char*)malloc(len + 3); /* Extra space for brackets if needed */
    if (!result) return NULL;
    
    const char* ptr = params_str;
    char* out = result;
    
    /* Skip leading whitespace */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Ensure it starts with bracket */
    if (*ptr != '[') {
        *out++ = '[';
    }
    
    /* Copy the content */
    int bracket_depth = 0;
    int in_string = 0;
    char string_quote = 0;
    
    while (*ptr) {
        if (!in_string) {
            if (*ptr == '"' || *ptr == '\'') {
                in_string = 1;
                string_quote = *ptr;
            } else if (*ptr == '[') {
                bracket_depth++;
            } else if (*ptr == ']') {
                bracket_depth--;
            }
        } else {
            if (*ptr == string_quote) {
                /* Check for doubled quote */
                if (*(ptr + 1) == string_quote) {
                    *out++ = *ptr;
                    *out++ = *(ptr + 1);
                    ptr++;
                    ptr++;
                    continue;
                } else {
                    in_string = 0;
                }
            }
        }
        *out++ = *ptr++;
    }
    
    /* Ensure it ends with bracket */
    if (bracket_depth > 0) {
        *out++ = ']';
    }
    
    *out = '\0';
    return result;
}

/* Handle communication with Lua framework */
static int kh_handle_communicate_lua_framework(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    /* Ensure Lua framework is available */
    if (!g_lua_interface) {
        kh_init_lua_framework_communication();
        if (!g_lua_interface) {
            kh_set_error(output, output_size, "LUA FRAMEWORK NOT AVAILABLE");
            return 1;
        }
    }
    
    /* Parse function name */
    char* clean_function = (char*)malloc(strlen(argv[0]) + 1);
    if (!clean_function) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    kh_clean_string(argv[0], clean_function, (int)strlen(argv[0]) + 1);
    
    /* Parse parameters */
    char* parameters = kh_parse_lua_parameters(argv[1]);
    if (!parameters) {
        free(clean_function);
        kh_set_error(output, output_size, "FAILED TO PARSE PARAMETERS");
        return 1;
    }
    
    /* Call the Lua framework */
    const char* result = g_lua_interface(clean_function, parameters);
    
    if (result) {
        /* Simply copy the result - let enforce_output_limit handle slicing */
        size_t result_len = strlen(result);
        if (result_len < (size_t)output_size) {
            strcpy_s(output, (size_t)output_size, result);
        } else {
            /* Copy full result for slicing */
            memcpy(output, result, result_len);
            output[result_len] = '\0';
        }
    } else {
        kh_set_error(output, output_size, "LUA FRAMEWORK RETURNED NULL");
    }
    
    free(clean_function);
    free(parameters);
    
    /* Check if result indicates an error */
    if (strncmp(output, "ERROR:", 6) == 0) {
        return 1;
    }
    
    return 0;
}