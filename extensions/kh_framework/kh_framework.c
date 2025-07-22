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

/* Include all module headers in dependency order */
#include "rv_extension_utils.h"
#include "array_operations.h"
#include "common_defines.h"
#include "crypto_operations.h"
#include "generate_random_string.h"
#include "math_operations.h"
#include "process_kh_data.h"
#include "string_operations.h"
#include "vector_operations.h"
#include "lua_integration.h"

__declspec(dllexport) uint64_t RVExtensionFeatureFlags = RVFeature_ContextNoDefaultCall;

/* Function name lookup table for performance optimization */
typedef struct {
    const char* name;
    int min_args;
    int max_args; /* -1 for variable args */
    char first_char; /* For fast dispatch optimization */
} function_info_t;

static const function_info_t FUNCTION_TABLE[] = {
    {"ArrayOperation", 1, 4, 'A'},
    {"GenerateRandomString", 1, 4, 'G'},
    {"SliceKHData", 2, 2, 'S'},
    {"StringOperation", 2, 4, 'S'},
    {"ReadKHData", 2, 3, 'R'},
    {"WriteKHData", 4, 5, 'W'},
    {"MathOperation", 1, 1, 'M'},
    {"VectorOperation", 1, 4, 'V'},
    {"UnbinarizeKHData", 1, 1, 'U'},
    {"BinarizeKHData", 1, 1, 'B'},
    {"CryptoOperation", 2, 2, 'C'},
    {"DeleteKHDataFile", 1, 1, 'D'},
    {"DeleteKHDataVariable", 2, 2, 'D'},
    {"LuaOperation", 2, 2, 'L'},
    {"LuaCompile", 1, 1, 'L'},
    {"LuaSetVariable", 3, 4, 'L'},
    {"LuaGetVariable", 1, 1, 'L'},
    {"LuaDeleteVariable", 1, 1, 'L'},
    {"LuaClearVariables", 0, 0, 'L'},
    {"LuaClearFunctions", 0, 0, 'L'}
};

static const int FUNCTION_COUNT = sizeof(FUNCTION_TABLE) / sizeof(function_info_t);

/* Fast function lookup and validation - optimized with hash-like first character check */
static inline int kh_validate_function_call(const char* function, int argc, char* error_output, int error_size) {
    if (!error_output || error_size <= 0) {
        return 0; /* Cannot report errors without valid output buffer */
    }
    
    /* Initialize output buffer */
    error_output[0] = '\0';
    
    if (!function) {
        kh_set_error(error_output, error_size, "NULL FUNCTION NAME");
        return 0;
    }
    
    /* Check for empty function name */
    if (strlen(function) == 0) {
        kh_set_error(error_output, error_size, "EMPTY FUNCTION NAME");
        return 0;
    }
    
    /* Check for excessively long function name (potential buffer overflow) */
    if (strlen(function) > 256) {
        kh_set_error(error_output, error_size, "FUNCTION NAME TOO LONG");
        return 0;
    }
    
    char first_char = function[0];
    int i;
    int function_found = 0;
    
    /* Fast first-character filtering */
    for (i = 0; i < FUNCTION_COUNT; i++) {
        if (FUNCTION_TABLE[i].first_char == first_char && 
            strcmp(FUNCTION_TABLE[i].name, function) == 0) {
            
            function_found = 1;
            
            /* Check minimum argument count */
            if (argc < FUNCTION_TABLE[i].min_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, 
                           KH_ERROR_PREFIX "FUNCTION '%s' REQUIRES AT LEAST %d ARGUMENTS, GOT %d",
                           function, FUNCTION_TABLE[i].min_args, argc);
                return 0;
            }
            
            /* Check maximum argument count */
            if (FUNCTION_TABLE[i].max_args != -1 && argc > FUNCTION_TABLE[i].max_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE,
                           KH_ERROR_PREFIX "FUNCTION '%s' ACCEPTS AT MOST %d ARGUMENTS, GOT %d",
                           function, FUNCTION_TABLE[i].max_args, argc);
                return 0;
            }
            
            /* Check for unreasonable argument count (potential attack) */
            if (argc > 100) {
                kh_set_error(error_output, error_size, "TOO MANY ARGUMENTS");
                return 0;
            }
            
            return 1; /* Function found and arguments valid */
        }
    }
    
    /* Function not found - provide helpful error message */
    if (!function_found) {
        _snprintf_s(error_output, (size_t)error_size, _TRUNCATE,
                   KH_ERROR_PREFIX "UNKNOWN FUNCTION '%s'", function);
        return 0;
    }
    
    /* This should never be reached */
    kh_set_error(error_output, error_size, "FUNCTION VALIDATION FAILED");
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

/* Main extension function for ARMA 3 callExtension interface - enhanced */
__declspec(dllexport) int RVExtensionArgs(char *output, unsigned int output_size, const char *function, const char **argv, unsigned int argc) {
    int function_result = 1;  /* Default to error */
    
    /* Enhanced input validation */
    if (!kh_validate_basic_inputs(output, output_size, function, argv, argc)) {
        return 1;
    }
    
    /* Validate function call */
    if (!kh_validate_function_call(function, argc, output, output_size)) {
        return 1;
    }
    
    /* Dispatch to appropriate function handler using optimized comparisons */
    char first_char = function[0];
    
    switch (first_char) {
        case 'A': /* ArrayOperation */
            if (strcmp(function, "ArrayOperation") == 0) {
                function_result = kh_process_array_operation(output, output_size, argv, argc);
            }
            break;

        case 'C': /* CryptoOperation */
            if (strcmp(function, "CryptoOperation") == 0) {
                function_result = kh_process_crypto_operation(output, output_size, argv, argc);
            }
            break;

        case 'D': /* DeleteKHDataFile and DeleteKHDataVariable */
            if (strcmp(function, "DeleteKHDataFile") == 0) {
                function_result = kh_delete_khdata_file(argv[0], output, output_size);
            } else if (strcmp(function, "DeleteKHDataVariable") == 0) {
                function_result = kh_delete_khdata_variable(argv[0], argv[1], output, output_size);
            }
            break;

        case 'G': /* GenerateRandomString */
            if (strcmp(function, "GenerateRandomString") == 0) {
                function_result = kh_process_random_string_generation(output, output_size, argv, argc);
            }
            break;

        case 'L': /* Lua operations - UPDATED with new functions */
            if (strcmp(function, "LuaOperation") == 0) {
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
                /* NEW: Clear all persistent Lua variables */
                function_result = kh_process_lua_clear_variables_operation(output, output_size, argv, argc);
            } else if (strcmp(function, "LuaClearFunctions") == 0) {
                /* NEW: Clear all cached Lua functions/bytecode */
                function_result = kh_process_lua_clear_functions_operation(output, output_size, argv, argc);
            }
            break;

        case 'S': /* SliceKHData and StringOperation */
            if (strcmp(function, "SliceKHData") == 0) {
                function_result = kh_calculate_slice_count(argv[0], argv[1], output, output_size);
            } else if (strcmp(function, "StringOperation") == 0) {
                function_result = kh_process_string_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'R': /* ReadKHData */
            if (strcmp(function, "ReadKHData") == 0) {
                /* Check if slice index is provided (3rd argument) */
                const char* slice_index = (argc >= 3) ? argv[2] : NULL;
                function_result = kh_read_khdata_variable_slice(argv[0], argv[1], slice_index, output, output_size);
            }
            break;
            
        case 'W': /* WriteKHData */
            if (strcmp(function, "WriteKHData") == 0) {
                /* 5th argument (overwrite flag) is optional, defaults to true */
                const char* overwrite_flag = (argc >= 5) ? argv[4] : "true";
                function_result = kh_write_khdata_variable(argv[0], argv[1], argv[2], argv[3], overwrite_flag, output, output_size);
            }
            break;
            
        case 'M': /* MathOperation */
            if (strcmp(function, "MathOperation") == 0) {
                function_result = kh_process_math_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'V': /* VectorOperation */
            if (strcmp(function, "VectorOperation") == 0) {
                function_result = kh_process_vector_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'U': /* UnbinarizeKHData */
            if (strcmp(function, "UnbinarizeKHData") == 0) {
                function_result = kh_unbinarize_khdata(argv[0], output, output_size);
            }
            break;
            
        case 'B': /* BinarizeKHData */
            if (strcmp(function, "BinarizeKHData") == 0) {
                function_result = kh_binarize_khdata(argv[0], output, output_size);
            }
            break;
            
        default:
            /* This should never happen due to validation, but just in case */
            kh_set_error(output, output_size, "UNKNOWN FUNCTION");
            return 1;
    }
    
    /* If function executed successfully (returned 0), check output limit */
    if (function_result == 0) {
        /* Enforce the global 8192KB output limit */
        if (kh_enforce_output_limit(output, output_size)) {
            /* Output exceeded limit and was replaced with error message */
            return 1;
        }
        /* Output is within limits, return success */
        return 0;
    }
    
    /* Function failed, but still check if error message itself exceeds limit (unlikely but safe) */
    kh_enforce_output_limit(output, output_size);
    
    /* Return the original function result */
    return function_result;
}

/* Alternative entry point for simple string-based calls (optional) - enhanced */
__declspec(dllexport) void RVExtension(char *output, unsigned int output_size, const char *function) {
    /* Enhanced validation for legacy interface */
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

/* DLL entry point - required for Windows DLLs - Enhanced with Memory Manager */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    /* Suppress unused parameter warnings */
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    /* Perform any necessary initialization/cleanup based on reason */
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            /* DLL is being loaded into a process */
            /* Initialize random seed here to avoid repeated initialization */
            kh_init_random();
            
            /* Initialize memory manager and load all .khdata files */
            if (!kh_init_memory_manager()) {
                /* Memory manager initialization failed - this is not fatal */
                /* The system will fall back to disk-based operations */
                /* Could log this error in a real system */
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            /* A thread is exiting cleanly */
            /* No thread-specific cleanup needed */
            break;
            
        case DLL_PROCESS_DETACH:
            /* DLL is being unloaded from the process */
            /* Clean up memory manager - save all dirty files and free memory */
            kh_cleanup_memory_manager();

            /* Clean up LUA states */
            kh_cleanup_lua_states();
            
            /* Clean up string operations tables */
            kh_cleanup_string_operations();
            break;
    }
    
    return TRUE; /* Indicate successful DLL operation */
}