#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Include all module headers in dependency order */
#include "rv_extension_utils.hpp"
#include "common_defines.hpp"
#include "process_kh_data.hpp"
#include "generate_random_string.hpp"
#include "math_operations.hpp"
#include "vector_operations.hpp"
#include "string_operations.hpp"
#include "crypto_operations.hpp"

__declspec(dllexport) uint64_t RVExtensionFeatureFlags = RVFeature_ContextNoDefaultCall;

/* Function name lookup table for performance optimization */
typedef struct {
    const char* name;
    int min_args;
    int max_args; /* -1 for variable args */
    char first_char; /* For fast dispatch optimization */
} function_info_t;

static const function_info_t FUNCTION_TABLE[] = {
    {"GenerateRandomString", 1, 4, 'G'},
    {"SliceData", 2, 2, 'S'},
    {"StringOperation", 2, 2, 'S'},
    {"ReadKHData", 2, 3, 'R'},
    {"WriteKHData", 4, 5, 'W'},
    {"MathOperation", 1, 1, 'M'},
    {"VectorOperation", 1, 4, 'V'},
    {"UnbinarizeKHData", 1, 1, 'U'},
    {"BinarizeKHData", 1, 1, 'B'},
    {"CryptoOperation", 2, 2, 'C'}
};

static const int FUNCTION_COUNT = sizeof(FUNCTION_TABLE) / sizeof(function_info_t);

/* Fast function lookup and validation - optimized with hash-like first character check */
static inline int kh_validate_function_call(const char* function, int argc, char* error_output, int error_size) {
    if (!function || !error_output || error_size <= 0) {
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "NULL FUNCTION NAME OR OUTPUT BUFFER");
        }
        return 0;
    }
    
    char first_char = function[0];
    int i;
    
    /* Fast first-character filtering */
    for (i = 0; i < FUNCTION_COUNT; i++) {
        if (FUNCTION_TABLE[i].first_char == first_char && 
            strcmp(FUNCTION_TABLE[i].name, function) == 0) {
            
            /* Check argument count */
            if (argc < FUNCTION_TABLE[i].min_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, 
                           KH_ERROR_PREFIX "FUNCTION '%s' REQUIRES AT LEAST %d ARGUMENTS, GOT %d",
                           function, FUNCTION_TABLE[i].min_args, argc);
                return 0;
            }
            
            if (FUNCTION_TABLE[i].max_args != -1 && argc > FUNCTION_TABLE[i].max_args) {
                _snprintf_s(error_output, (size_t)error_size, _TRUNCATE,
                           KH_ERROR_PREFIX "FUNCTION '%s' ACCEPTS AT MOST %d ARGUMENTS, GOT %d",
                           function, FUNCTION_TABLE[i].max_args, argc);
                return 0;
            }
            
            return 1; /* Function found and arguments valid */
        }
    }
    
    kh_set_error(error_output, error_size, "UNKNOWN FUNCTION");
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
        case 'C': /* CryptoOperation */
            if (strcmp(function, "CryptoOperation") == 0) {
                return kh_process_crypto_operation(output, output_size, argv, argc);
            }
            break;

        case 'G': /* GenerateRandomString */
            if (strcmp(function, "GenerateRandomString") == 0) {
                return kh_process_random_string_generation(output, output_size, argv, argc);
            }
            break;
            
        case 'S': /* SliceData and StringOperation */
            if (strcmp(function, "SliceData") == 0) {
                return kh_calculate_slice_count(argv[0], argv[1], output, output_size);
            }
            if (strcmp(function, "StringOperation") == 0) {
                return kh_process_string_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'R': /* ReadKHData */
            if (strcmp(function, "ReadKHData") == 0) {
                /* Check if slice index is provided (3rd argument) */
                const char* slice_index = (argc >= 3) ? argv[2] : NULL;
                return kh_read_khdata_variable_slice(argv[0], argv[1], slice_index, output, output_size);
            }
            break;
            
        case 'W': /* WriteKHData */
            if (strcmp(function, "WriteKHData") == 0) {
                /* 5th argument (overwrite flag) is optional, defaults to true */
                const char* overwrite_flag = (argc >= 5) ? argv[4] : "true";
                return kh_write_khdata_variable(argv[0], argv[1], argv[2], argv[3], overwrite_flag, output, output_size);
            }
            break;
            
        case 'M': /* MathOperation */
            if (strcmp(function, "MathOperation") == 0) {
                return kh_process_math_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'V': /* VectorOperation */
            if (strcmp(function, "VectorOperation") == 0) {
                return kh_process_vector_operation(output, output_size, argv, argc);
            }
            break;
            
        case 'U': /* UnbinarizeKHData */
            if (strcmp(function, "UnbinarizeKHData") == 0) {
                return kh_unbinarize_khdata(argv[0], output, output_size);
            }
            break;
            
        case 'B': /* BinarizeKHData */
            if (strcmp(function, "BinarizeKHData") == 0) {
                return kh_binarize_khdata(argv[0], output, output_size);
            }
            break;
            
        default:
            /* This should never happen due to validation, but just in case */
            kh_set_error(output, output_size, "UNKNOWN FUNCTION");
            return 1;
    }
    
    /* This should never be reached due to validation */
    kh_set_error(output, output_size, "FUNCTION DISPATCH FAILED");
    return 1;
}

/* Alternative entry point for simple string-based calls (optional) - enhanced */
__declspec(dllexport) void RVExtension(char *output, unsigned int output_size, const char *function) {
    /* Enhanced validation for legacy interface */
    if (!output || output_size <= 0) {
        return; /* Cannot provide error feedback without valid output buffer */
    }
    
    if (!function) {
        kh_set_error(output, output_size, "NULL FUNCTION NAME IN LEGACY CALL");
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
            /* DLL is being loaded into a process */
            /* Initialize random seed here to avoid repeated initialization */
            kh_init_random();
            break;
            
        case DLL_THREAD_ATTACH:
            /* A new thread is being created in the process */
            /* No thread-specific initialization needed */
            break;
            
        case DLL_THREAD_DETACH:
            /* A thread is exiting cleanly */
            /* No thread-specific cleanup needed */
            break;
            
        case DLL_PROCESS_DETACH:
            /* DLL is being unloaded from the process */
            /* Clean up string operations tables */
            kh_cleanup_string_operations();
            break;
    }
    
    return TRUE; /* Indicate successful DLL operation */
}