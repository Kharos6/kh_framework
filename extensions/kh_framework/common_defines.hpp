#ifndef COMMON_DEFINES_HPP
#define COMMON_DEFINES_HPP

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* Constants */
#define MAX_PATH_LENGTH 512
#define MAX_TOTAL_KHDATA_SIZE_BYTES (1024LL * 1024LL * 1024LL)  /* 1GB total limit */
#define MAX_KHDATA_FILES 128                                      /* Maximum .khdata files allowed */
#define SLICE_SIZE 8192                                          /* 8KB per slice */
#define KHDATA_MAGIC 0x5444484B                                  /* "KHDT" in little endian */
#define KHDATA_VERSION 1
#define KH_ERROR_PREFIX "KH_ERROR: "
#define MATH_PI 3.141592653589793
#define VECTOR_EPSILON 1e-9
#define MAX_VECTOR_COMPONENTS 4
#define MAX_EXPR_LENGTH 4096  /* Increased for dynamic allocation */
#define MAX_TOKEN_LENGTH 128
#define MAX_FUNCTION_NAME 64

/* Forward declarations */
typedef struct kh_variable_s kh_variable_t;

/* Thread-safe random number generator state */
typedef struct {
    unsigned int seed;
    int initialized;
} kh_rng_state_t;

/* Global RNG state with thread safety */
static kh_rng_state_t g_rng_state = {0, 0};

/* Parse boolean string (removes quotes, case-insensitive) */
static inline int kh_parse_boolean(const char* bool_str) {
    if (!bool_str) return 0;
    
    char clean_bool[16];
    int i, j = 0;
    
    /* Clean the boolean string (remove quotes, convert to lowercase) */
    for (i = 0; bool_str[i] != '\0' && j < sizeof(clean_bool) - 1; i++) {
        if (bool_str[i] != '"') {
            if (bool_str[i] >= 'A' && bool_str[i] <= 'Z') {
                clean_bool[j++] = bool_str[i] + 32; /* Convert to lowercase */
            } else {
                clean_bool[j++] = bool_str[i];
            }
        }
    }
    clean_bool[j] = '\0';
    
    /* Check for true values */
    return (strcmp(clean_bool, "true") == 0 || strcmp(clean_bool, "1") == 0) ? 1 : 0;
}

/* Cleanup near zero values */
static inline double kh_cleanup_near_zero(double value) {
    return (fabs(value) < 1e-15) ? 0.0 : value;
}

/* Case-insensitive string comparison */
static inline int kh_strcasecmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return (str1 == str2) ? 0 : (str1 ? 1 : -1);
    
    int i;
    char c1, c2;
    
    for (i = 0; str1[i] != '\0' && str2[i] != '\0'; i++) {
        c1 = str1[i];
        c2 = str2[i];
        
        /* Convert to lowercase */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return (c1 < c2) ? -1 : 1;
        }
    }
    
    /* Check if one string is longer */
    if (str1[i] != '\0') return 1;
    if (str2[i] != '\0') return -1;
    
    return 0;
}

/* Remove surrounding quotes from string - optimized version */
static inline void kh_clean_string(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 1) {
        if (output && output_size > 0) output[0] = '\0';
        return;
    }
    
    const char* start = input;
    int input_len = (int)strlen(input);
    const char* end = input + input_len - 1;
    int len;
    
    /* Skip opening quote if present */
    if (input_len > 0 && *start == '"') {
        start++;
    }
    
    /* Skip closing quote if present */
    if (end >= start && *end == '"') {
        end--;
    }
    
    /* Calculate length without quotes */
    len = (int)(end - start + 1);
    if (len >= output_size) {
        len = output_size - 1;
    }
    
    /* Copy the clean string */
    if (len > 0) {
        memcpy(output, start, (size_t)len);
        output[len] = '\0';
    } else {
        output[0] = '\0';
    }
}

/* Validate that string is not empty after cleaning */
static inline int kh_validate_non_empty_string(const char* input, const char* field_name, char* error_output, int error_size) {
    if (!input || !field_name || !error_output || error_size <= 0) return 0;
    
    char* clean_input = (char*)malloc(strlen(input) + 1);
    if (!clean_input) {
        _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, KH_ERROR_PREFIX "MEMORY ALLOCATION FAILED");
        return 0;
    }
    
    kh_clean_string(input, clean_input, (int)strlen(input) + 1);
    
    int result = (strlen(clean_input) > 0);
    if (!result) {
        _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, KH_ERROR_PREFIX "EMPTY %s NOT ALLOWED", field_name);
    }
    
    free(clean_input);
    return result;
}

/* Set standardized error message */
static inline void kh_set_error(char* output, int output_size, const char* message) {
    if (!output || output_size <= 0 || !message) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "%s", message);
}

/* Thread-safe random number initialization */
static inline void kh_init_random(void) {
    if (!g_rng_state.initialized) {
        g_rng_state.seed = (unsigned int)time(NULL) ^ (unsigned int)GetTickCount();
        /* Ensure seed is never zero */
        if (g_rng_state.seed == 0) g_rng_state.seed = 1;
        g_rng_state.initialized = 1;
    }
}

/* Fast random number generator using LCG */
static inline unsigned int kh_fast_rand(void) {
    kh_init_random();
    g_rng_state.seed = g_rng_state.seed * 1103515245U + 12345U;
    return g_rng_state.seed;
}

/* Get the user's Documents\Arma 3 folder path */
static inline int kh_get_arma3_documents_path(char* path, int path_size) {
    if (!path || path_size <= 0) return 0;
    
    char documents_path[MAX_PATH];
    
    if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path) != S_OK) {
        return 0;
    }
    
    if (_snprintf_s(path, (size_t)path_size, _TRUNCATE, "%s\\Arma 3", documents_path) < 0) {
        return 0;
    }
    
    /* Create directory if it doesn't exist */
    CreateDirectoryA(path, NULL);
    return 1;
}

/* Get file size in bytes */
static inline long kh_get_file_size(const char* file_path) {
    if (!file_path) return -1;
    
    FILE* file;
    long size;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return -1;
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    
    return size;
}

/* Count existing .khdata files in Arma 3 documents folder */
static inline int kh_count_khdata_files(void) {
    char arma3_path[MAX_PATH_LENGTH];
    char search_pattern[MAX_PATH_LENGTH];
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    int file_count = 0;
    
    if (!kh_get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return -1;
    }
    
    _snprintf_s(search_pattern, sizeof(search_pattern), _TRUNCATE, "%s\\*.khdata", arma3_path);
    
    find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return 0; /* No files found */
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            file_count++;
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return file_count;
}

/* Get full path to a .khdata file */
static inline int kh_get_khdata_file_path(const char* filename, char* full_path, int path_size) {
    if (!filename || !full_path || path_size <= 0) return 0;
    
    char arma3_path[MAX_PATH_LENGTH];
    char* clean_filename = NULL;
    int filename_len = (int)strlen(filename);
    int result = 0;
    
    if (!kh_get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return 0;
    }
    
    if (filename_len >= MAX_PATH_LENGTH - 8) {
        return 0; /* Filename too long */
    }
    
    /* Allocate space for clean filename */
    clean_filename = (char*)malloc((size_t)filename_len + 8); /* +8 for .khdata extension */
    if (!clean_filename) return 0;
    
    /* Clean filename - remove quotes */
    kh_clean_string(filename, clean_filename, filename_len + 1);
    int clean_len = (int)strlen(clean_filename);
    
    /* Add .khdata extension if not present */
    if (clean_len < 7 || strcmp(clean_filename + clean_len - 7, ".khdata") != 0) {
        strcat_s(clean_filename, (size_t)filename_len + 8, ".khdata");
    }
    
    /* Build full path */
    result = (_snprintf_s(full_path, (size_t)path_size, _TRUNCATE, "%s\\%s", arma3_path, clean_filename) >= 0) ? 1 : 0;
    
    free(clean_filename);
    return result;
}

/* Check if creating a new file would exceed the limit */
static inline int kh_check_file_limit(const char* filename, char* error_output, int error_size) {
    if (!filename || !error_output || error_size <= 0) return 0;
    
    char file_path[MAX_PATH_LENGTH];
    int current_count = kh_count_khdata_files();
    
    if (current_count < 0) {
        kh_set_error(error_output, error_size, "FAILED TO COUNT EXISTING FILES");
        return 0;
    }
    
    /* Check if file already exists (if so, we're not creating a new one) */
    if (kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        DWORD file_attributes = GetFileAttributesA(file_path);
        if (file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1; /* File exists, no new file creation */
        }
    }
    
    /* Check if we would exceed the limit */
    if (current_count >= MAX_KHDATA_FILES) {
        _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, KH_ERROR_PREFIX "MAXIMUM %d KHDATA FILES EXCEEDED", MAX_KHDATA_FILES);
        return 0;
    }
    
    return 1;
}

#endif /* COMMON_DEFINES_HPP */