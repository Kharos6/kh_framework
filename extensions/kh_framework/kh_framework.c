#include <math.h>
#include <shlobj.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wincrypt.h>
#include <windows.h>

#include "rv_extension_utils.h"
#include "common_defines.h"
#include "crypto_operations.h"
#include "generate_random_string.h"
#include "process_kh_data.h"
#include "lua_integration.h"

__declspec(dllexport) uint64_t RVExtensionFeatureFlags = RVFeature_ContextNoDefaultCall;

/* Function name lookup table for performance optimization */
typedef struct {
    const char* name;
    int min_args;
    int max_args;
} function_info_t;

static const function_info_t FUNCTION_TABLE[] = {
    {"GenerateRandomString", 1, 4},
    {"SliceKHData", 2, 2},
    {"ReadKHData", 2, 3},
    {"WriteKHData", 3, 3},
    {"UnbinarizeKHData", 1, 1},
    {"BinarizeKHData", 1, 1},
    {"CryptoOperation", 2, 2},
    {"DeleteKHDataFile", 1, 1},
    {"DeleteKHDataVariable", 2, 2},
    {"LuaOperation", 2, 2},
    {"LuaCompile", 1, 2},
    {"LuaSetVariable", 2, 2},
    {"LuaGetVariable", 1, 1},
    {"LuaDeleteVariable", 1, 1},
    {"LuaClearVariables", 0, 0},
    {"LuaResetState", 0, 0}
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
            strcmp(g_func_hash_table[index].func_info->name, function) == 0) {
            
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

/* Enhanced input validation */
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
    int function_result = 1;  /* Default to error */
    
    /* Input validation */
    if (!kh_validate_basic_inputs(output, output_size, function, argv, argc)) {
        return 1;
    }
    
    /* Validate function call */
    if (!kh_validate_function_call(function, argc, output, output_size)) {
        return 1;
    }
    
    /* Direct dispatch using strcmp - function is already validated */
    if (strcmp(function, "CryptoOperation") == 0) {
        function_result = kh_process_crypto_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "DeleteKHDataFile") == 0) {
        function_result = kh_delete_khdata_file(argv[0], output, output_size);
    } else if (strcmp(function, "DeleteKHDataVariable") == 0) {
        function_result = kh_delete_khdata_variable(argv[0], argv[1], output, output_size);
    } else if (strcmp(function, "GenerateRandomString") == 0) {
        function_result = kh_process_random_string_generation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaOperation") == 0) {
        function_result = kh_process_lua_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaCompile") == 0) {
        function_result = kh_process_lua_compile_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaSetVariable") == 0) {
        function_result = kh_process_lua_set_variable_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaGetVariable") == 0) {
        function_result = kh_process_lua_get_variable_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaDeleteVariable") == 0) {
        function_result = kh_process_lua_delete_variable_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaClearVariables") == 0) {
        function_result = kh_process_lua_clear_variables_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "LuaResetState") == 0) {
        function_result = kh_process_lua_reset_state_operation(output, output_size, argv, argc);
    } else if (strcmp(function, "SliceKHData") == 0) {
        function_result = kh_calculate_slice_count(argv[0], argv[1], output, output_size);
    } else if (strcmp(function, "ReadKHData") == 0) {
        const char* slice_index = (argc >= 3) ? argv[2] : NULL;
        function_result = kh_read_khdata_variable_slice(argv[0], argv[1], slice_index, output, output_size);
    } else if (strcmp(function, "WriteKHData") == 0) {
        function_result = kh_write_khdata_variable(argv[0], argv[1], argv[2], output, output_size);
    } else if (strcmp(function, "UnbinarizeKHData") == 0) {
        function_result = kh_unbinarize_khdata(argv[0], output, output_size);
    } else if (strcmp(function, "BinarizeKHData") == 0) {
        function_result = kh_binarize_khdata(argv[0], output, output_size);
    } else {
        /* This should never happen due to validation, but just in case */
        kh_set_error(output, output_size, "UNKNOWN FUNCTION");
        return 1;
    }
    
    /* If function executed successfully (returned 0), check output limit */
    if (function_result == 0) {
        if (kh_enforce_output_limit(output, output_size)) {
            return 1;
        }
        return 0;
    }
    
    kh_enforce_output_limit(output, output_size);
    return function_result;
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
            kh_init_random();
            kh_init_type_hash_table();
            kh_init_crypto_hash_table();
            kh_init_func_hash_table();
            lua_init_variable_storage();
            lua_init_function_storage();
            kh_init_memory_manager();
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
            
        case DLL_PROCESS_DETACH:
            kh_cleanup_memory_manager();
            kh_cleanup_lua_states();
            kh_cleanup_crypto_provider();
            break;
    }
    
    return TRUE; /* Indicate successful DLL operation */
}