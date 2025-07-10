#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "process_kh_data.hpp"
#include "generate_random_string.hpp"
#include "slice_data.hpp"

/* Main function that Arma 3 calls (backward compatibility) */
__declspec(dllexport) void RVExtension(char *output, int outputSize, const char *function) {
    int length;
    char* charset;
    int charset_size;
    
    init_random();
    output[0] = '\0';
    
    length = atoi(function);
    
    if (length <= 0) {
        strcpy_s(output, outputSize, "KH_ERROR: INVALID LENGTH");
        return;
    }
    
    if (length >= outputSize) {
        strcpy_s(output, outputSize, "KH_ERROR: LENGTH TOO LARGE");
        return;
    }
    
    /* Default charset for old syntax (numbers + letters) */
    charset = (char*)malloc(strlen(NUMBERS) + strlen(LETTERS) + 1);
    if (!charset) {
        strcpy_s(output, outputSize, "KH_ERROR: MEMORY ALLOCATION FAILED");
        return;
    }
    
    strcpy_s(charset, strlen(NUMBERS) + strlen(LETTERS) + 1, NUMBERS);
    strcat_s(charset, strlen(NUMBERS) + strlen(LETTERS) + 1, LETTERS);
    charset_size = strlen(charset);
    
    generate_string(output, length, charset, charset_size);
    output[length] = '\0';
    
    free(charset);
}

/* Function arguments version for the new callExtension syntax */
__declspec(dllexport) int RVExtensionArgs(char *output, int outputSize, const char *function, const char **argv, int argc) {
    output[0] = '\0';
    
    /* Handle GenerateRandomString function */
    if (strcmp(function, "GenerateRandomString") == 0) {
        return process_random_string_generation(output, outputSize, argv, argc);
    }
    
    /* Handle SliceData function */
    else if (strcmp(function, "SliceData") == 0) {
        if (argc < 2) {
            strcpy_s(output, outputSize, "KH_ERROR: INSUFFICIENT ARGUMENTS");
            return 1;
        }
        
        return calculate_slice_count(argv[0], argv[1], output, outputSize);
    }
    
    /* Handle ReadKHData function with optional slice index */
    else if (strcmp(function, "ReadKHData") == 0) {
        if (argc < 2) {
            strcpy_s(output, outputSize, "KH_ERROR: INSUFFICIENT ARGUMENTS");
            return 1;
        }
        
        /* Check if slice index is provided (3rd argument) */
        const char* slice_index = (argc >= 3) ? argv[2] : NULL;
        
        return read_khdata_variable_slice(argv[0], argv[1], slice_index, output, outputSize);
    }
    
    /* Handle WriteKHData function with new signature: filename, variable_name, value, type, overwrite_flag */
    else if (strcmp(function, "WriteKHData") == 0) {
        if (argc < 4) {
            strcpy_s(output, outputSize, "KH_ERROR: INSUFFICIENT ARGUMENTS - NEED FILENAME, VARIABLE_NAME, VALUE, TYPE");
            return 1;
        }
        
        /* 5th argument (overwrite flag) is optional, defaults to true */
        const char* overwrite_flag = (argc >= 5) ? argv[4] : "true";
        
        return write_khdata_variable(argv[0], argv[1], argv[2], argv[3], overwrite_flag, output, outputSize);
    }
    
    /* Unknown function */
    else {
        strcpy_s(output, outputSize, "KH_ERROR: UNKNOWN FUNCTION");
        return 1;
    }
}

/* DLL entry point */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}