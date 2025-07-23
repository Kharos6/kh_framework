#ifndef COMMON_DEFINES_HPP
#define COMMON_DEFINES_HPP

#include <ctype.h>
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

/* Constants */
#define KH_MAX_OUTPUT_SIZE 8192
#define MAX_FILE_PATH_LENGTH 512
#define MAX_TOTAL_KHDATA_SIZE_BYTES (1024LL * 1024LL * 1024LL)  /* 1GB total limit */
#define MAX_KHDATA_FILES 1024                                      /* Maximum .khdata files allowed */
#define KH_ARRAY_MAX_ELEMENTS 10000               /* Maximum elements per array */
#define KH_ARRAY_MAX_DEPTH 32                     /* Maximum nesting depth */
#define KH_ARRAY_MAX_INPUT_SIZE (32 * 1024 * 1024) /* 32MB max input */
#define KH_ARRAY_PARSE_OPERATIONS_LIMIT 50000     /* Prevent infinite parsing loops */
#define KH_HASH_TABLE_MIN_SIZE 16
#define KH_HASH_TABLE_LOAD_FACTOR 0.5
#define KH_HASH_EMPTY 0                 /* Empty hash table entry marker */
#define KH_STRING_ENCODE_MAGIC 0x4B48          /* "KH" in little endian */
#define KH_STRING_MAX_INPUT_SIZE (32 * 1024 * 1024) /* 32MB max input */
#define KH_STRING_ESCAPE_CHAR 0xFF             /* Escape character for unmapped chars */
#define KH_STRING_RLE_CHAR 0xFE                /* Run-length encoding marker */
#define KH_STRING_TABLE_SIZE 256               /* Size of encoding table */
#define KH_STRING_MIN_RLE_COUNT 3              /* Minimum count for RLE */
#define KH_STRING_MAX_SPLIT_PARTS 1024         /* Maximum parts for string splitting */
#define KH_STRING_MAX_REPLACE_COUNT 10000      /* Maximum replacements to prevent infinite loops */
#define KH_CRYPTO_MAX_INPUT_SIZE (32 * 1024 * 1024)  /* 32MB max input */
#define KH_FNV1A_32_OFFSET_BASIS 0x811c9dc5U
#define KH_FNV1A_32_PRIME 0x01000193U
#define KH_FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define KH_FNV1A_64_PRIME 0x100000001b3ULL
#define KH_CRC32_POLYNOMIAL 0xEDB88320U
#define MAX_MATH_FUNCTION_ARGS 3
#define INITIAL_MATH_TOKEN_SIZE 64  /* Initial size for dynamic token allocation */
#define MAX_MATH_RECURSION_DEPTH 50 /* Maximum recursion depth to prevent stack overflow */
#define MAX_MATH_PARSE_OPERATIONS 10000 /* Maximum operations to prevent infinite loops */
#define SLICE_SIZE 8192                                          /* 8KB per slice */
#define KHDATA_MAGIC 0x5444484B                                  /* "KHDT" in little endian */
#define KHDATA_VERSION 1
#define KH_ERROR_PREFIX "KH_ERROR: "
#define MATH_PI 3.141592653589793
#define VECTOR_EPSILON 1e-9
#define KH_MAX_FRAGMENTATION_RATIO 0.3  /* Compact when 30% fragmented */
#define KH_MIN_COMPACTION_INTERVAL 60  /* Minimum 1 minute between compactions */
#define LUA_STATE_POOL_SIZE 8                    /* Pool of reusable Lua states */
#define LUA_BYTECODE_CACHE_SIZE 1024             /* Bytecode cache entries */
#define LUA_MEMORY_POOL_SIZE (128 * 1024 * 1024)  /* 64MB memory pool */
#define LUA_MAX_SIMPLE_ARGS 64                   /* Fast path argument limit */
#define LUA_MAX_RECURSION_DEPTH 24        /* Reduced for safety */
#define LUA_STACK_SAFETY_MARGIN 10        /* Safety margin for Lua stack */
#define LUA_POOL_ALIGNMENT 16             /* Memory alignment for performance */
#define LUA_VAR_HASH_TABLE_MIN_SIZE 16
#define LUA_VAR_HASH_TABLE_LOAD_FACTOR 0.5
#define LUA_VAR_HASH_EMPTY 0

/* Forward declarations */
typedef struct kh_variable_s kh_variable_t;

/* Thread-local random number generator state */
typedef struct {
    unsigned int seed;
    int initialized;
} kh_rng_state_t;

/* Thread-local RNG state to avoid race conditions */
static __declspec(thread) kh_rng_state_t tls_rng_state = {0, 0};

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

/* UTF-8 utility functions */
static inline int kh_utf8_byte_length(unsigned char first_byte) {
    if (first_byte < 0x80) return 1;
    if ((first_byte & 0xE0) == 0xC0) return 2;
    if ((first_byte & 0xF0) == 0xE0) return 3;
    if ((first_byte & 0xF8) == 0xF0) return 4;
    return 0; /* Invalid UTF-8 */
}

static inline int kh_is_valid_utf8_continuation(unsigned char byte) {
    return (byte & 0xC0) == 0x80;
}

static inline int kh_validate_utf8_sequence(const char* str, int pos, int len) {
    if (pos >= len) return 0;
    
    unsigned char first = (unsigned char)str[pos];
    int char_len = kh_utf8_byte_length(first);
    
    if (char_len == 0 || pos + char_len > len) return 0;
    
    /* Check continuation bytes */
    for (int i = 1; i < char_len; i++) {
        if (!kh_is_valid_utf8_continuation((unsigned char)str[pos + i])) {
            return 0;
        }
    }
    
    return char_len;
}

static inline int kh_validate_utf8_string(const char* str, int byte_len) {
    if (!str) return 0;
    
    int pos = 0;
    while (pos < byte_len) {
        int char_len = kh_validate_utf8_sequence(str, pos, byte_len);
        if (char_len == 0) return 0;
        pos += char_len;
    }
    
    return 1;
}

static inline int kh_utf8_safe_slice_end(const char* str, int byte_len, int target_end) {
    if (!str || target_end <= 0) return 0;
    if (target_end >= byte_len) return byte_len;
    
    /* Find the start of the UTF-8 character that contains or follows target_end */
    int pos = target_end;
    
    /* If we're in the middle of a UTF-8 sequence, back up to the start */
    while (pos > 0 && ((unsigned char)str[pos] & 0xC0) == 0x80) {
        pos--;
    }
    
    /* Validate this is a proper UTF-8 character start */
    int char_len = kh_validate_utf8_sequence(str, pos, byte_len);
    if (char_len == 0) {
        /* Invalid UTF-8, fall back to target_end */
        return target_end;
    }
    
    /* If this character would extend beyond target_end, end before it */
    if (pos + char_len > target_end) {
        return pos;
    }
    
    return target_end;
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
    
    /* Calculate length without quotes - add safety check */
    len = (int)(end - start + 1);
    if (len < 0) len = 0; /* Safety check for edge cases */
    if (len >= output_size) {
        len = output_size - 1;
    }
    
    /* Copy the clean string */
    if (len > 0) {
        memcpy(output, start, (size_t)len);
        output[len] = '\0';
        
        /* Validate UTF-8 and adjust length if needed */
        if (!kh_validate_utf8_string(output, len)) {
            /* Find the last valid UTF-8 character position */
            int safe_len = 0;
            int pos = 0;
            while (pos < len) {
                int char_len = kh_validate_utf8_sequence(output, pos, len);
                if (char_len == 0) break;
                pos += char_len;
                safe_len = pos;
            }
            output[safe_len] = '\0';
        }
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

/* Validate filename for security - prevent path traversal and enforce .khdata extension */
static inline int kh_validate_filename(const char* filename) {
    if (!filename) return 0;
    
    const char* ptr = filename;
    int has_extension = 0;
    int filename_len = (int)strlen(filename);
    
    /* Check for empty filename */
    if (filename_len == 0) return 0;
    
    /* Check each character */
    while (*ptr) {
        char c = *ptr;
        
        /* Reject path separators and dangerous characters */
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || 
            c == '"' || c == '<' || c == '>' || c == '|' || c == '\0') {
            return 0;
        }
        
        /* Reject control characters */
        if (c < 32 || c == 127) {
            return 0;
        }
        
        /* Reject leading/trailing dots or spaces */
        if ((ptr == filename || *(ptr + 1) == '\0') && (c == '.' || c == ' ')) {
            return 0;
        }
        
        ptr++;
    }
    
    /* Check for .khdata extension or allow it to be added later */
    if (filename_len >= 7) {
        if (strcmp(filename + filename_len - 7, ".khdata") == 0) {
            has_extension = 1;
        }
    }
    
    /* Filename is valid - extension will be added if not present */
    return 1;
}

/* Validate variable name - only alphanumeric and underscore allowed */
static inline int kh_validate_variable_name(const char* name) {
    if (!name || strlen(name) == 0) return 0;
    
    const char* ptr = name;
    
    /* First character must be letter or underscore */
    if (!(((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || *ptr == '_'))) {
        return 0;
    }
    
    /* Rest can be alphanumeric or underscore */
    ptr++;
    while (*ptr) {
        if (!((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || 
              (*ptr >= '0' && *ptr <= '9') || *ptr == '_')) {
            return 0;
        }
        ptr++;
    }
    
    return 1;
}

/* Validate scalar value - must be a valid number */
static inline int kh_validate_scalar_value(const char* value) {
    if (!value || strlen(value) == 0) return 0;
    
    char* end_ptr;
    const char* start = value;
    
    /* Skip leading whitespace */
    while (*start && (*start == ' ' || *start == '\t')) start++;
    
    /* Check if it's a valid number */
    strtod(start, &end_ptr);
    
    /* Skip trailing whitespace */
    while (*end_ptr && (*end_ptr == ' ' || *end_ptr == '\t')) end_ptr++;
    
    /* Must consume the entire string (after trimming whitespace) */
    return (*end_ptr == '\0' && end_ptr != start);
}

/* Validate boolean value - must be true/false/1/0 */
static inline int kh_validate_bool_value(const char* value) {
    if (!value) return 0;
    
    char* clean_value = (char*)malloc(strlen(value) + 1);
    if (!clean_value) return 0;
    
    kh_clean_string(value, clean_value, (int)strlen(value) + 1);
    
    /* Convert to lowercase for comparison */
    char* ptr = clean_value;
    while (*ptr) {
        if (*ptr >= 'A' && *ptr <= 'Z') {
            *ptr = *ptr + 32;
        }
        ptr++;
    }
    
    int result = (strcmp(clean_value, "true") == 0 || strcmp(clean_value, "false") == 0 ||
                  strcmp(clean_value, "1") == 0 || strcmp(clean_value, "0") == 0);
    
    free(clean_value);
    return result;
}

/* Validate array format - check bracket matching and basic structure */
static inline int kh_validate_array_format(const char* value) {
    if (!value) return 0;
    
    const char* ptr = value;
    int bracket_count = 0;
    int square_bracket_count = 0;
    int has_opening_bracket = 0;
    int has_content = 0;
    
    /* Skip leading whitespace */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Must start with opening bracket */
    if (*ptr != '[') return 0;
    
    /* Check bracket matching and basic structure */
    while (*ptr) {
        if (*ptr == '[') {
            square_bracket_count++;
            if (!has_opening_bracket) has_opening_bracket = 1;
        } else if (*ptr == ']') {
            square_bracket_count--;
            if (square_bracket_count < 0) return 0; /* Mismatched brackets */
        } else if (*ptr == '(' || *ptr == '{') {
            bracket_count++;
        } else if (*ptr == ')' || *ptr == '}') {
            bracket_count--;
            if (bracket_count < 0) return 0; /* Mismatched brackets */
        } else if (*ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r' && *ptr != ',') {
            has_content = 1;
        }
        ptr++;
    }
    
    /* Must have balanced brackets and proper structure */
    return (square_bracket_count == 0 && bracket_count == 0 && has_opening_bracket);
}

/* Validate hashmap format - check bracket matching and key-value pair structure */
static inline int kh_validate_hashmap_format(const char* value) {
    if (!value) return 0;
    
    const char* ptr = value;
    int bracket_count = 0;
    int square_bracket_count = 0;
    int paren_count = 0;
    int brace_count = 0;
    int has_opening_bracket = 0;
    size_t input_len = strlen(value);

    if (input_len == 0) return 0;
    
    /* Skip leading whitespace */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Must start with opening bracket */
    if (*ptr != '[') return 0;
    
    /* Check bracket matching */
    while (*ptr && (size_t)(ptr - value) < input_len) {
        switch (*ptr) {
            case '[':
                square_bracket_count++;
                if (square_bracket_count == 1) has_opening_bracket = 1;
                break;
            case ']':
                square_bracket_count--;
                if (square_bracket_count < 0) return 0;
                break;
            case '(':
                paren_count++;
                break;
            case ')':
                paren_count--;
                if (paren_count < 0) return 0;
                break;
            case '{':
                brace_count++;
                break;
            case '}':
                brace_count--;
                if (brace_count < 0) return 0;
                break;
        }
        ptr++;
    }
    
    /* All brackets must be balanced */
    return (square_bracket_count == 0 && paren_count == 0 && brace_count == 0 && has_opening_bracket);
}

/* Main value format validation dispatcher */
static inline int kh_validate_value_format(int data_type, const char* value) {
    if (!value) return 0;
    
    /* Forward declaration for type constants - these should match process_kh_data.h */
    typedef enum {
        KH_TYPE_ARRAY = 0,
        KH_TYPE_STRING = 1,
        KH_TYPE_SCALAR = 2,
        KH_TYPE_HASHMAP = 3,
        KH_TYPE_BOOL = 4,
        KH_TYPE_CODE = 5,
        KH_TYPE_TEXT = 6
    } kh_data_type_local_t;
    
    switch ((kh_data_type_local_t)data_type) {
        case KH_TYPE_SCALAR:
            return kh_validate_scalar_value(value);
            
        case KH_TYPE_ARRAY:
            return kh_validate_array_format(value);
            
        case KH_TYPE_HASHMAP:
            return kh_validate_hashmap_format(value);
            
        case KH_TYPE_BOOL:
            return kh_validate_bool_value(value);
            
        case KH_TYPE_STRING:
        case KH_TYPE_TEXT:
        case KH_TYPE_CODE:
            /* These types accept any string content */
            return 1;
            
        default:
            return 0; /* Unknown type */
    }
}

/* Set standardized error message */
static inline void kh_set_error(char* output, int output_size, const char* message) {
    if (!output || output_size <= 0 || !message) return;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "%s", message);
}

/* Check and enforce the global output limit */
static inline int kh_enforce_output_limit(char* output, int output_size) {
    if (!output || output_size <= 0) return 0;
    
    size_t actual_length = strlen(output);
    
    /* Check if output exceeds the global limit */
    if (actual_length > KH_MAX_OUTPUT_SIZE) {
        /* Replace output with error message */
        kh_set_error(output, output_size, "OUTPUT EXCEEDS LIMIT");
        return 1;  /* Indicate limit exceeded */
    }
    
    return 0;  /* Output is within limits */
}

/* Thread-safe random number initialization */
static inline void kh_init_random(void) {
    if (!tls_rng_state.initialized) {
        tls_rng_state.seed = (unsigned int)time(NULL) ^ (unsigned int)GetTickCount() ^ (unsigned int)GetCurrentThreadId();
        /* Ensure seed is never zero */
        if (tls_rng_state.seed == 0) tls_rng_state.seed = 1;
        tls_rng_state.initialized = 1;
    }
}

/* Fast random number generator using LCG */
static inline unsigned int kh_fast_rand(void) {
    kh_init_random();
    tls_rng_state.seed = tls_rng_state.seed * 1103515245U + 12345U;
    return tls_rng_state.seed;
}

/* Get the user's Documents\Arma 3 folder path */
static inline int kh_get_arma3_documents_path(char* path, int path_size) {
    if (!path || path_size <= 0) return 0;
    
    char documents_path[MAX_PATH];
    char arma3_path[MAX_PATH];
    char framework_path[MAX_PATH];
    
    if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path) != S_OK) {
        return 0;
    }
    
    /* Build Documents\Arma 3 path */
    if (_snprintf_s(arma3_path, sizeof(arma3_path), _TRUNCATE, "%s\\Arma 3", documents_path) < 0) {
        return 0;
    }
    
    /* Build Documents\Arma 3\kh_framework path */
    if (_snprintf_s(framework_path, sizeof(framework_path), _TRUNCATE, "%s\\kh_framework", arma3_path) < 0) {
        return 0;
    }
    
    /* Build final Documents\Arma 3\kh_framework\kh_data path */
    if (_snprintf_s(path, (size_t)path_size, _TRUNCATE, "%s\\kh_data", framework_path) < 0) {
        return 0;
    }
    
    /* Create directories if they don't exist (nested creation) */
    CreateDirectoryA(arma3_path, NULL);      /* Create Arma 3 folder */
    CreateDirectoryA(framework_path, NULL);   /* Create kh_framework folder */
    CreateDirectoryA(path, NULL);             /* Create kh_data folder */
    
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
    char arma3_path[MAX_FILE_PATH_LENGTH];
    char search_pattern[MAX_FILE_PATH_LENGTH];
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

/* Get full path to a .khdata file with enhanced security validation */
static inline int kh_get_khdata_file_path(const char* filename, char* full_path, int path_size) {
    if (!filename || !full_path || path_size <= 0) return 0;
    
    char arma3_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int filename_len = (int)strlen(filename);
    int result = 0;
    
    if (!kh_get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return 0;
    }
    
    if (filename_len >= MAX_FILE_PATH_LENGTH - 8) {
        return 0; /* Filename too long */
    }
    
    /* Allocate space for clean filename */
    clean_filename = (char*)malloc((size_t)filename_len + 8); /* +8 for .khdata extension */
    if (!clean_filename) return 0;
    
    /* Clean filename - remove quotes */
    kh_clean_string(filename, clean_filename, filename_len + 1);
    int clean_len = (int)strlen(clean_filename);
    
    /* Validate filename for security - prevent path traversal */
    if (!kh_validate_filename(clean_filename)) {
        free(clean_filename);
        return 0;
    }
    
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
    
    char file_path[MAX_FILE_PATH_LENGTH];
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