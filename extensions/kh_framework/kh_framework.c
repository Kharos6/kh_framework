#include "common_declarations.h"
#include "framework.h"
#include "rv_extension_utils.h"
#include "process_kh_data.h"
#include "crypto_operations.h"
#include "generate_random_string.h"
#include "lua_integration.h"

__declspec(dllexport) uint64_t RVExtensionFeatureFlags = RVFeature_ContextNoDefaultCall | RVFeature_ArgumentNoEscapeString;

/* Function name lookup table */
typedef int (*kh_function_handler)(char* output, int output_size, const char** argv, int argc);

typedef struct {
    const char* name;
    int min_args;
    int max_args;
    kh_function_handler handler;
} function_info_t;

/* Direct function handlers - no wrappers needed */
static int kh_handle_read_khdata(char* output, int output_size, const char** argv, int argc) {
    return kh_read_khdata_variable(argv[0], argv[1], output, output_size);
}

static int kh_handle_write_khdata(char* output, int output_size, const char** argv, int argc) {
    return kh_write_khdata_variable(argv[0], argv[1], argv[2], output, output_size);
}

static int kh_handle_unbinarize_khdata(char* output, int output_size, const char** argv, int argc) {
    return kh_unbinarize_khdata(argv[0], output, output_size);
}

static int kh_handle_binarize_khdata(char* output, int output_size, const char** argv, int argc) {
    return kh_binarize_khdata(argv[0], output, output_size);
}

static int kh_handle_delete_khdata_file(char* output, int output_size, const char** argv, int argc) {
    return kh_delete_khdata_file(argv[0], output, output_size);
}

static int kh_handle_delete_khdata_variable(char* output, int output_size, const char** argv, int argc) {
    return kh_delete_khdata_variable(argv[0], argv[1], output, output_size);
}

static int kh_handle_flush_khdata(char* output, int output_size, const char** argv, int argc) {
    return kh_flush_khdata(output, output_size);
}

static const function_info_t FUNCTION_TABLE[] = {
    {"GenerateRandomString", 1, 4, kh_process_random_string_generation},
    {"ReadKHData", 2, 2, kh_handle_read_khdata},
    {"WriteKHData", 3, 3, kh_handle_write_khdata},
    {"UnbinarizeKHData", 1, 1, kh_handle_unbinarize_khdata},
    {"BinarizeKHData", 1, 1, kh_handle_binarize_khdata},
    {"CryptoOperation", 2, 2, kh_process_crypto_operation},
    {"DeleteKHDataFile", 1, 1, kh_handle_delete_khdata_file},
    {"DeleteKHDataVariable", 2, 2, kh_handle_delete_khdata_variable},
    {"FlushKHData", 0, 0, kh_handle_flush_khdata},
    {"LuaOperation", 2, 2, kh_process_lua_operation},
    {"LuaCompile", 1, 2, kh_process_lua_compile_operation},
    {"LuaSetVariable", 2, 2, kh_process_lua_set_variable_operation},
    {"LuaGetVariable", 1, 1, kh_process_lua_get_variable_operation},
    {"LuaDeleteVariable", 1, 1, kh_process_lua_delete_variable_operation},
    {"LuaClearVariables", 0, 0, kh_process_lua_clear_variables_operation},
    {"LuaResetState", 0, 0, kh_process_lua_reset_state_operation},
    {"SliceExtensionReturn", 1, 1, kh_process_slice_extension_return}
};

static const int FUNCTION_COUNT = sizeof(FUNCTION_TABLE) / sizeof(function_info_t);

typedef struct {
    uint32_t hash;
    const function_info_t* func_info;
} func_hash_entry_t;

static func_hash_entry_t g_func_hash_table[KH_FUNC_HASH_SIZE];
static int g_func_hash_initialized = 0;

/* Initialize function hash table */
static void kh_init_func_hash_table(void) {
    if (g_func_hash_initialized) return;
    
    memset(g_func_hash_table, 0, sizeof(g_func_hash_table));
    
    for (int i = 0; i < FUNCTION_COUNT; i++) {
        uint32_t hash = kh_hash_name_case_insensitive(FUNCTION_TABLE[i].name);
        uint32_t index = hash & (KH_FUNC_HASH_SIZE - 1);
        
        /* Linear probing for collisions */
        while (g_func_hash_table[index].hash != 0) {
            index = (index + 1) & (KH_FUNC_HASH_SIZE - 1);
        }
        
        g_func_hash_table[index].hash = hash;
        g_func_hash_table[index].func_info = &FUNCTION_TABLE[i];
    }
    
    g_func_hash_initialized = 1;
}

/* Fast function lookup and validation */
static inline int kh_validate_function_call(const char* function, int argc, char* error_output, int error_size) {
    if (!error_output || error_size <= 0) {
        return 0;
    }
    
    error_output[0] = '\0';
    
    if (!function) {
        kh_set_error(error_output, error_size, "NULL FUNCTION NAME");
        return 0;
    }
    
    if (strlen(function) == 0) {
        kh_set_error(error_output, error_size, "EMPTY FUNCTION NAME");
        return 0;
    }
    
    if (strlen(function) > 256) {
        kh_set_error(error_output, error_size, "FUNCTION NAME TOO LONG");
        return 0;
    }

    /* Hash lookup */
    uint32_t hash = kh_hash_name_case_insensitive(function);
    uint32_t index = hash & (KH_FUNC_HASH_SIZE - 1);
    
    /* Linear probe to find match */
    for (int probe = 0; probe < KH_FUNC_HASH_SIZE; probe++) {
        if (g_func_hash_table[index].hash == 0) {
            break; /* Empty slot, not found */
        }
        if (g_func_hash_table[index].hash == hash && 
            kh_strcasecmp(g_func_hash_table[index].func_info->name, function) == 0) {
            
            const function_info_t* func_info = g_func_hash_table[index].func_info;
            
            /* Check minimum argument count */
            if (argc < func_info->min_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, 
                           KH_ERROR_PREFIX "FUNCTION '%s' REQUIRES AT LEAST %d ARGUMENTS, GOT %d",
                           function, func_info->min_args, argc);
                return 0;
            }
            
            /* Check maximum argument count */
            if (func_info->max_args != -1 && argc > func_info->max_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE,
                           KH_ERROR_PREFIX "FUNCTION '%s' ACCEPTS AT MOST %d ARGUMENTS, GOT %d",
                           function, func_info->max_args, argc);
                return 0;
            }
            
            return 1; /* Function found and arguments valid */
        }
        index = (index + 1) & (KH_FUNC_HASH_SIZE - 1);
    }
    
    /* Function not found */
    _snprintf_s(error_output, (size_t)error_size, _TRUNCATE,
               KH_ERROR_PREFIX "UNKNOWN FUNCTION '%s'", function);
    return 0;
}

/* Input validation */
static inline int kh_validate_basic_inputs(char* output, int output_size, const char* function, const char** argv, int argc) {
    if (!output) return 0;
    
    if (output_size <= 1) {
        if (output_size == 1) output[0] = '\0';
        return 0;
    }
    
    /* Initialize output buffer */
    output[0] = '\0';
    
    if (!function) {
        kh_set_error(output, output_size, "NULL FUNCTION NAME");
        return 0;
    }
    
    if (!argv && argc > 0) {
        kh_set_error(output, output_size, "NULL ARGUMENTS WITH NON-ZERO COUNT");
        return 0;
    }
    
    if (argc < 0) {
        kh_set_error(output, output_size, "NEGATIVE ARGUMENT COUNT");
        return 0;
    }
    
    return 1;
}

/* Main extension function for callExtension interface */
__declspec(dllexport) int RVExtensionArgs(char *output, unsigned int output_size, const char *function, const char **argv, unsigned int argc) {    
    /* Input validation */
    if (!kh_validate_basic_inputs(output, output_size, function, argv, argc)) {
        return 1; /* Error */
    }
    
    /* Validate function call */
    if (!kh_validate_function_call(function, argc, output, output_size)) {
        return 1; /* Error */
    }

    /* Check if this is a slice operation using case-insensitive comparison */
    int is_slice_operation = (kh_strcasecmp(function, "SliceExtensionReturn") == 0) ? 1 : 0;
    
    /* Direct dispatch using function pointer from hash table */
    uint32_t hash = kh_hash_name_case_insensitive(function);
    uint32_t index = hash & (KH_FUNC_HASH_SIZE - 1);
    const function_info_t* func_info = NULL;
    int probe_count = 0;
    
    /* Find function in hash table with loop protection */
    while (probe_count < KH_FUNC_HASH_SIZE) {
        if (g_func_hash_table[index].hash == 0) {
            break; /* Empty slot, not found */
        }
        if (g_func_hash_table[index].hash == hash && 
            kh_strcasecmp(g_func_hash_table[index].func_info->name, function) == 0) {
            func_info = g_func_hash_table[index].func_info;
            break;
        }
        index = (index + 1) & (KH_FUNC_HASH_SIZE - 1);
        probe_count++;
    }
    
    if (func_info && func_info->handler) {
        /* Execute function handler - returns 0 for success */
        int result = func_info->handler(output, output_size, argv, argc);
        
        /* Check output limit */
        if (result == 0) {
            if (kh_enforce_output_limit(output, output_size, is_slice_operation) != 0) {
                return 1; /* Error */
            }
        }
        
        return result;
    } else {
        /* This should never happen due to validation, but just in case */
        kh_set_error(output, output_size, "FUNCTION HANDLER NOT FOUND");
        return 1; /* Error */
    }
}

/* Alternative entry point for simple function calls */
__declspec(dllexport) void RVExtension(char *output, unsigned int output_size, const char *function) {
    if (!output || output_size <= 0) {
        return; /* Cannot provide error feedback without valid output buffer */
    }
    
    if (!function) {
        kh_set_error(output, output_size, "NULL FUNCTION NAME");
        return;
    }
    
    /* Convert to RVExtensionArgs format with no arguments */
    const char* empty_argv[1] = {NULL};
    RVExtensionArgs(output, output_size, function, empty_argv, 0);
}

/* DLL Version Info */
__declspec(dllexport) void RVExtensionVersion(char *output, unsigned int output_size) {
    if (!output || output_size <= 0) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "KH Framework 1.0");
}

/* DLL entry point - required for Windows DLLs */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    /* Suppress unused parameter warnings */
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    /* Perform any necessary initialization/cleanup based on reason */
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            /* Reset ALL static variables to ensure clean state */
            g_func_hash_initialized = 0;
            g_type_hash_initialized = 0;
            g_crypto_hash_initialized = 0;
            g_lua_initialized = 0;
            crc32_table_initialized = 0;
            
            /* Clear static structures */
            memset(&g_lua_state, 0, sizeof(g_lua_state));
            memset(&g_memory_manager, 0, sizeof(g_memory_manager));
            memset(&g_extension_return_storage, 0, sizeof(g_extension_return_storage));
            memset(&g_lua_variable_storage, 0, sizeof(g_lua_variable_storage));
            memset(&g_lua_function_storage, 0, sizeof(g_lua_function_storage));
            memset(&g_secure_rng_state, 0, sizeof(g_secure_rng_state));
            memset(&g_crypto_cache, 0, sizeof(g_crypto_cache));
            
            /* Clear hash tables */
            memset(g_func_hash_table, 0, sizeof(g_func_hash_table));
            memset(g_type_hash_table, 0, sizeof(g_type_hash_table));
            memset(g_crypto_hash_table, 0, sizeof(g_crypto_hash_table));
            
            /* Now initialize everything fresh */
            kh_init_random();
            kh_init_type_hash_table();
            kh_init_crypto_hash_table();
            kh_init_func_hash_table();
            lua_init_variable_storage();
            lua_init_function_storage();
            kh_init_crypto_provider();
            kh_init_crc32_table();
            kh_init_memory_manager();
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
            
        case DLL_PROCESS_DETACH:
            if (lpReserved != NULL) {
                /* Process is terminating - skip complex cleanup to avoid deadlocks */
                /* The OS will reclaim all memory and handles automatically */
                break;
            }

            kh_free_extension_return_storage();
            kh_cleanup_random();
            kh_cleanup_memory_manager();
            kh_cleanup_lua_states();
            kh_cleanup_crypto_provider();
            break;
    }
    
    return TRUE; /* Indicate successful DLL operation */
}