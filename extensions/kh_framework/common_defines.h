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
#define EXTENSION_NAME "kh_framework"
#define KH_MAX_OUTPUT_SIZE 8192
#define MAX_FILE_PATH_LENGTH 512
#define MAX_TOTAL_KHDATA_SIZE_BYTES (1024LL * 1024LL * 1024LL)   /* 1GB total limit */
#define MAX_KHDATA_FILES 1024                                    /* Maximum .khdata files allowed */
#define KH_HASH_TABLE_MIN_SIZE 16
#define KH_HASH_EMPTY 0                                          /* Empty hash table entry marker */
#define KH_FNV1A_32_OFFSET_BASIS 0x811c9dc5U
#define KH_FNV1A_32_PRIME 0x01000193U
#define KH_FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define KH_FNV1A_64_PRIME 0x100000001b3ULL
#define KH_CRC32_POLYNOMIAL 0xEDB88320U
#define SLICE_SIZE 8192                                          /* 8KB per slice */
#define KHDATA_MAGIC 0x5444484B                                  /* "KHDT" in little endian */
#define KHDATA_VERSION 1
#define KH_ERROR_PREFIX "KH_ERROR: "
#define LUA_MAX_SIMPLE_ARGS 128
#define LUA_VAR_HASH_TABLE_MIN_SIZE 16
#define KH_TYPE_HASH_SIZE 64
#define KH_CRYPTO_HASH_SIZE 64
#define KH_FUNC_HASH_SIZE 64

/* Data type enumeration - Complete list for KHData */
typedef enum {
    KH_TYPE_BOOL = 0,
    KH_TYPE_SCALAR = 1,
    KH_TYPE_STRING = 2,
    KH_TYPE_MARKER = 3,
    KH_TYPE_TEXT = 4,
    KH_TYPE_CONFIG = 5,
    KH_TYPE_MISSION_CONFIG = 6,
    KH_TYPE_ARRAY = 7,
    KH_TYPE_HASHMAP = 8,
    KH_TYPE_CODE = 9,
    KH_TYPE_OBJECT = 10,
    KH_TYPE_TEAM_MEMBER = 11,
    KH_TYPE_GROUP = 12,
    KH_TYPE_NAMESPACE = 13,
    KH_TYPE_SIDE = 14,
    KH_TYPE_LOCATION = 15,
    KH_TYPE_MISSION_NAMESPACE_REFERENCE = 16,
    KH_TYPE_MISSION_PROFILE_NAMESPACE_REFERENCE = 17,
    KH_TYPE_PROFILE_NAMESPACE_REFERENCE = 18,
    KH_TYPE_UI_NAMESPACE_REFERENCE = 19,
    KH_TYPE_SERVER_NAMESPACE_REFERENCE = 20,
    KH_TYPE_PARSING_NAMESPACE_REFERENCE = 21,
    KH_TYPE_KH_DATA_NAMESPACE_REFERENCE = 22,
    KH_TYPE_LUA_NAMESPACE_REFERENCE = 23,
    KH_TYPE_TYPED_ARRAY = 24,
    KH_TYPE_TYPED_HASHMAP = 25,
    KH_TYPE_NIL = 26,
    KH_TYPE_UNKNOWN = -1
} kh_type_t;

/* Type string lookup table for better performance */
typedef struct {
    const char* name;
    kh_type_t type;
} kh_type_mapping_t;

static const kh_type_mapping_t KH_TYPE_MAPPINGS[] = {
    {"BOOL", KH_TYPE_BOOL},
    {"SCALAR", KH_TYPE_SCALAR},
    {"STRING", KH_TYPE_STRING},
    {"MARKER", KH_TYPE_MARKER},
    {"TEXT", KH_TYPE_TEXT},
    {"CONFIG", KH_TYPE_CONFIG},
    {"MISSION_CONFIG", KH_TYPE_MISSION_CONFIG},
    {"ARRAY", KH_TYPE_ARRAY},
    {"HASHMAP", KH_TYPE_HASHMAP},
    {"CODE", KH_TYPE_CODE},
    {"OBJECT", KH_TYPE_OBJECT},
    {"TEAM_MEMBER", KH_TYPE_TEAM_MEMBER},
    {"GROUP", KH_TYPE_GROUP},
    {"NAMESPACE", KH_TYPE_NAMESPACE},
    {"SIDE", KH_TYPE_SIDE},
    {"LOCATION", KH_TYPE_LOCATION},
    {"MISSION_NAMESPACE_REFERENCE", KH_TYPE_MISSION_NAMESPACE_REFERENCE},
    {"MISSION_PROFILE_NAMESPACE_REFERENCE", KH_TYPE_MISSION_PROFILE_NAMESPACE_REFERENCE},
    {"PROFILE_NAMESPACE_REFERENCE", KH_TYPE_PROFILE_NAMESPACE_REFERENCE},
    {"UI_NAMESPACE_REFERENCE", KH_TYPE_UI_NAMESPACE_REFERENCE},
    {"SERVER_NAMESPACE_REFERENCE", KH_TYPE_SERVER_NAMESPACE_REFERENCE},
    {"PARSING_NAMESPACE_REFERENCE", KH_TYPE_PARSING_NAMESPACE_REFERENCE},
    {"KH_DATA_NAMESPACE_REFERENCE", KH_TYPE_KH_DATA_NAMESPACE_REFERENCE},
    {"LUA_NAMESPACE_REFERENCE", KH_TYPE_LUA_NAMESPACE_REFERENCE},
    {"TYPED_ARRAY", KH_TYPE_TYPED_ARRAY},
    {"TYPED_HASHMAP", KH_TYPE_TYPED_HASHMAP},
    {"NIL", KH_TYPE_NIL}
};

static const int KH_TYPE_MAPPING_COUNT = sizeof(KH_TYPE_MAPPINGS) / sizeof(kh_type_mapping_t);
typedef struct kh_variable_s kh_variable_t;

/* Thread-local random number generator state */
static struct {
    unsigned int seed;
    int initialized;
} g_rng_state = {0, 0};

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
    int backtrack_count = 0;
    const int max_backtrack = 4; /* Maximum UTF-8 sequence length */
    
    /* If we're in the middle of a UTF-8 sequence, back up to the start */
    while (pos > 0 && ((unsigned char)str[pos] & 0xC0) == 0x80) {
        pos--;
        backtrack_count++;
        if (backtrack_count >= max_backtrack) {
            /* Invalid UTF-8 sequence, use target_end */
            return target_end;
        }
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

/* Remove surrounding quotes from string */
static inline void kh_clean_string(const char* input, char* output, int output_size) {
    if (!output || output_size <= 0) return;
    
    /* Handle special case where output_size is 1 */
    if (output_size == 1) {
        output[0] = '\0';
        return;
    }
    
    if (!input) {
        output[0] = '\0';
        return;
    }
    
    const char* start = input;
    int input_len = (int)strlen(input);
    const char* end = input + input_len - 1;
    int len;
    
    /* Skip opening quote if present (both single and double quotes) */
    if (input_len > 0 && (*start == '"' || *start == '\'')) {
        start++;
    }
    
    /* Skip closing quote if present (both single and double quotes) */
    if (end >= start && (*end == '"' || *end == '\'')) {
        end--;
    }
    
    /* Calculate length without quotes */
    len = (int)(end - start + 1);
    if (len < 0) len = 0;
    
    /* Ensure we don't overflow output buffer */
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

/* Generic hash entry structure for Robin Hood hashing */
typedef struct {
    uint32_t name_hash;      
    uint32_t data_index;     
    uint32_t reserved;       
    uint8_t deleted;         
    uint8_t distance;        
    uint16_t padding;        
} kh_generic_hash_entry_t;

/* Generic hash table info */
typedef struct {
    kh_generic_hash_entry_t* entries;
    uint32_t size;
    uint32_t used_count;
    uint32_t deleted_count;
    int needs_rebuild;
} kh_generic_hash_table_t;

/* Case-insensitive hash for variable names */
static inline uint32_t kh_hash_name_case_insensitive(const char* name) {
    if (!name) return 0;
    
    uint32_t hash = 2166136261U;
    const uint32_t fnv_prime = 16777619U;
    
    const char* ptr = name;
    while (*ptr) {
        char c = *ptr;
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
        hash ^= (uint32_t)c;
        hash *= fnv_prime;
        ptr++;
    }
    
    return (hash == 0) ? 1 : hash;
}

/* Calculate optimal hash table size */
static inline uint32_t kh_calculate_optimal_hash_size(uint32_t count) {
    if (count > UINT32_MAX / 2) {
        return 16;
    }
    
    uint32_t min_size = (uint32_t)((double)count / 0.5);
    uint32_t size = 16;
    
    while (size < min_size && size <= UINT32_MAX / 2) {
        size <<= 1;
    }
    
    if (size < 16) size = 16;
    return size;
}

/* Generic Robin Hood find */
static inline uint32_t kh_generic_hash_find(kh_generic_hash_entry_t* hash_table, 
                                            uint32_t hash_table_size,
                                            uint32_t name_hash, int* found) {
    if (!hash_table || hash_table_size == 0 || name_hash == 0) {
        if (found) *found = 0;
        return 0;
    }
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    uint32_t first_deleted = UINT32_MAX;
    
    do {
        kh_generic_hash_entry_t* entry = &hash_table[index];
        
        /* Track first deleted slot for potential reuse */
        if (entry->deleted && first_deleted == UINT32_MAX) {
            first_deleted = index;
        }
        
        /* Empty slot means not found */
        if (entry->name_hash == 0 && !entry->deleted) {
            if (found) *found = 0;
            /* Return first deleted slot if we found one, otherwise this empty slot */
            return (first_deleted != UINT32_MAX) ? first_deleted : index;
        }
        
        /* Found matching hash and not deleted */
        if (!entry->deleted && entry->name_hash == name_hash) {
            if (found) *found = 1;
            return index;
        }
        
        /* Robin Hood: if current entry's distance is less than ours, element isn't here */
        if (!entry->deleted && entry->distance < distance) {
            if (found) *found = 0;
            /* Return first deleted slot if we found one, otherwise this position */
            return (first_deleted != UINT32_MAX) ? first_deleted : index;
        }
        
        distance++;
        if (distance > 65535) break;
        
        uint64_t next_offset = (uint64_t)distance * distance;
        if (next_offset > hash_table_size) {
            index = (original_index + distance) % hash_table_size;
        } else {
            index = (original_index + (uint32_t)next_offset) % hash_table_size;
        }
        
        if (distance >= hash_table_size) break;
        
    } while (1);
    
    if (found) *found = 0;
    return (first_deleted != UINT32_MAX) ? first_deleted : 0;
}

/* Generic Robin Hood insert */
static inline int kh_generic_hash_insert(kh_generic_hash_entry_t* hash_table, 
                                         uint32_t hash_table_size,
                                         uint32_t name_hash, uint32_t data_index) {
    if (!hash_table || hash_table_size == 0 || name_hash == 0) return 0;
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    
    kh_generic_hash_entry_t new_entry = {name_hash, data_index, 0, 0, 0, 0};
    
    while (distance < hash_table_size) {
        kh_generic_hash_entry_t* entry = &hash_table[index];
        
        if (entry->name_hash == 0 || entry->deleted) {
            *entry = new_entry;
            entry->distance = (uint8_t)distance;
            return 1;
        }
        
        if (entry->name_hash == name_hash) {
            entry->data_index = data_index;
            return 1;
        }
        
        if (distance > entry->distance) {
            kh_generic_hash_entry_t temp = *entry;
            *entry = new_entry;
            entry->distance = (uint8_t)distance;
            new_entry = temp;
            distance = temp.distance;
        }
        
        distance++;
        if (distance > 65535) break;
        
        uint64_t next_offset = (uint64_t)distance * distance;
        if (next_offset > hash_table_size) {
            index = (original_index + distance) % hash_table_size;
        } else {
            index = (original_index + (uint32_t)next_offset) % hash_table_size;
        }
        
        if (distance >= hash_table_size) break;
    }
    
    return 0;
}

/* Generic hash delete */
static inline int kh_generic_hash_delete(kh_generic_hash_entry_t* hash_table, 
                                         uint32_t hash_table_size, 
                                         uint32_t name_hash) {
    if (!hash_table || hash_table_size == 0 || name_hash == 0) return 0;
    
    int found = 0;
    uint32_t index = kh_generic_hash_find(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        hash_table[index].deleted = 1;
        hash_table[index].name_hash = 0;
        hash_table[index].data_index = 0;
        return 1;
    }
    
    return 0;
}

/* Check if hash table needs rebuilding */
static inline int kh_generic_hash_needs_rebuild(kh_generic_hash_table_t* hash_info, uint32_t item_count) {
    if (!hash_info || !hash_info->entries) return 1;
    
    double load_factor = (double)(hash_info->used_count + hash_info->deleted_count) / hash_info->size;
    if (load_factor > 0.9) return 1;
    
    /* Only rebuild if deletions are very high */
    if (hash_info->deleted_count > hash_info->used_count) return 1;
    
    uint32_t optimal_size = kh_calculate_optimal_hash_size(item_count);
    if (hash_info->size < optimal_size / 2 || hash_info->size > optimal_size * 2) return 1;
    
    if (hash_info->needs_rebuild) return 1;
    
    return 0;
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
    int square_bracket_count = 0;
    int has_opening_bracket = 0;
    
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
        }
        ptr++;
    }
    
    /* Must have balanced brackets and proper structure */
    return (square_bracket_count == 0 && has_opening_bracket);
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

/* Validate namespace value */
static inline int kh_validate_namespace_value(const char* value) {
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
    
    int result = (strcmp(clean_value, "missionnamespace") == 0 ||
                  strcmp(clean_value, "missionprofilenamespace") == 0 ||
                  strcmp(clean_value, "profilenamespace") == 0 ||
                  strcmp(clean_value, "servernamespace") == 0 ||
                  strcmp(clean_value, "uinamespace") == 0 ||
                  strcmp(clean_value, "parsingnamespace") == 0);
    
    free(clean_value);
    return result;
}

/* Validate side value */
static inline int kh_validate_side_value(const char* value) {
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
    
    int result = (strcmp(clean_value, "west") == 0 ||
                  strcmp(clean_value, "east") == 0 ||
                  strcmp(clean_value, "resistance") == 0 ||
                  strcmp(clean_value, "civilian") == 0 ||
                  strcmp(clean_value, "blufor") == 0 ||
                  strcmp(clean_value, "opfor") == 0 ||
                  strcmp(clean_value, "independent") == 0 ||
                  strcmp(clean_value, "sidefriendly") == 0 ||
                  strcmp(clean_value, "sideenemy") == 0 ||
                  strcmp(clean_value, "sideambientlife") == 0 ||
                  strcmp(clean_value, "sideempty") == 0 ||
                  strcmp(clean_value, "sideunknown") == 0 ||
                  strcmp(clean_value, "sidelogic") == 0);
    
    free(clean_value);
    return result;
}

/* Validate nil value */
static inline int kh_validate_nil_value(const char* value) {
    if (!value) return 1; /* NULL is valid for nil */
    
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
    
    int result = (strcmp(clean_value, "nil") == 0 || 
                  strcmp(clean_value, "null") == 0 ||
                  strlen(clean_value) == 0);
    
    free(clean_value);
    return result;
}

/* Main value format validation dispatcher */
static inline int kh_validate_value_format(kh_type_t data_type, const char* value) {
    if (!value && data_type != KH_TYPE_NIL) return 0;
    
    switch (data_type) {
        case KH_TYPE_SCALAR:
            return kh_validate_scalar_value(value);
            
        case KH_TYPE_ARRAY:
            return kh_validate_array_format(value);
            
        case KH_TYPE_HASHMAP:
            return kh_validate_hashmap_format(value);
            
        case KH_TYPE_BOOL:
            return kh_validate_bool_value(value);
            
        case KH_TYPE_NAMESPACE:
            return kh_validate_namespace_value(value);
            
        case KH_TYPE_SIDE:
            return kh_validate_side_value(value);
            
        case KH_TYPE_NIL:
            return kh_validate_nil_value(value);
            
        case KH_TYPE_TYPED_ARRAY:
        case KH_TYPE_TYPED_HASHMAP:
            /* These require special validation that will be done elsewhere */
            return kh_validate_array_format(value);
            
        case KH_TYPE_STRING:
        case KH_TYPE_MARKER:
        case KH_TYPE_TEXT:
        case KH_TYPE_CONFIG:
        case KH_TYPE_MISSION_CONFIG:
        case KH_TYPE_CODE:
        case KH_TYPE_OBJECT:
        case KH_TYPE_TEAM_MEMBER:
        case KH_TYPE_GROUP:
        case KH_TYPE_LOCATION:
        case KH_TYPE_MISSION_NAMESPACE_REFERENCE:
        case KH_TYPE_MISSION_PROFILE_NAMESPACE_REFERENCE:
        case KH_TYPE_PROFILE_NAMESPACE_REFERENCE:
        case KH_TYPE_UI_NAMESPACE_REFERENCE:
        case KH_TYPE_SERVER_NAMESPACE_REFERENCE:
        case KH_TYPE_PARSING_NAMESPACE_REFERENCE:
        case KH_TYPE_KH_DATA_NAMESPACE_REFERENCE:
        case KH_TYPE_LUA_NAMESPACE_REFERENCE:
            /* These types accept any string content */
            return 1;
            
        default:
            return 0; /* Unknown type */
    }
}

typedef struct {
    uint32_t hash;
    kh_type_t type;
    const char* name;
} kh_type_hash_entry_t;

static kh_type_hash_entry_t g_type_hash_table[KH_TYPE_HASH_SIZE];
static int g_type_hash_initialized = 0;

/* Initialize type hash table */
static inline void kh_init_type_hash_table(void) {
    if (g_type_hash_initialized) return;
    
    memset(g_type_hash_table, 0, sizeof(g_type_hash_table));
    
    for (int i = 0; i < KH_TYPE_MAPPING_COUNT; i++) {
        uint32_t hash = kh_hash_name_case_insensitive(KH_TYPE_MAPPINGS[i].name);
        uint32_t index = hash & (KH_TYPE_HASH_SIZE - 1);
        
        /* Linear probing for collisions */
        while (g_type_hash_table[index].hash != 0) {
            index = (index + 1) & (KH_TYPE_HASH_SIZE - 1);
        }
        
        g_type_hash_table[index].hash = hash;
        g_type_hash_table[index].type = KH_TYPE_MAPPINGS[i].type;
        g_type_hash_table[index].name = KH_TYPE_MAPPINGS[i].name;
    }
    
    g_type_hash_initialized = 1;
}

/* Convert type string to enum */
static inline kh_type_t kh_get_type_from_string(const char* type_str) {
    if (!type_str) return KH_TYPE_UNKNOWN;
    
    char* clean_type = (char*)malloc(strlen(type_str) + 1);
    if (!clean_type) return KH_TYPE_UNKNOWN;
    
    int i, j = 0;
    
    /* Clean and convert to uppercase */
    for (i = 0; type_str[i] != '\0'; i++) {
        if (type_str[i] != '"' && type_str[i] != '\'') {
            if (type_str[i] >= 'a' && type_str[i] <= 'z') {
                clean_type[j++] = type_str[i] - 32;
            } else {
                clean_type[j++] = type_str[i];
            }
        }
    }
    clean_type[j] = '\0';
    
    /* Hash lookup */
    uint32_t hash = kh_hash_name_case_insensitive(clean_type);
    uint32_t index = hash & (KH_TYPE_HASH_SIZE - 1);
    
    /* Linear probe to find match */
    for (int probe = 0; probe < KH_TYPE_HASH_SIZE; probe++) {
        if (g_type_hash_table[index].hash == 0) {
            break; /* Empty slot, not found */
        }
        if (g_type_hash_table[index].hash == hash && 
            strcmp(g_type_hash_table[index].name, clean_type) == 0) {
            free(clean_type);
            return g_type_hash_table[index].type;
        }
        index = (index + 1) & (KH_TYPE_HASH_SIZE - 1);
    }
    
    free(clean_type);
    return KH_TYPE_UNKNOWN;
}

/* Convert type enum to string */
static inline const char* kh_get_string_from_type(kh_type_t type) {
    for (int i = 0; i < KH_TYPE_MAPPING_COUNT; i++) {
        if (KH_TYPE_MAPPINGS[i].type == type) {
            return KH_TYPE_MAPPINGS[i].name;
        }
    }
    return "UNKNOWN";
}

/* Parse [type, value] array from input string */
static int kh_parse_type_value_array(const char* input, char** type_str, char** value_str) {
    if (!input || !type_str || !value_str) return 0;
    
    *type_str = NULL;
    *value_str = NULL;
    
    const char* ptr = input;
    int bracket_depth = 0;
    const char* array_start = NULL;
    const char* comma_pos = NULL;
    const char* type_start = NULL;
    const char* type_end = NULL;
    const char* value_start = NULL;
    const char* value_end = NULL;
    
    /* Skip leading whitespace */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Must start with [ */
    if (*ptr != '[') return 0;
    array_start = ptr;
    ptr++;
    bracket_depth = 1;
    
    /* Skip whitespace after opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Find type string */
    type_start = ptr;
    
    /* Type should be a quoted string */
    if (*ptr == '"' || *ptr == '\'') {
        char quote = *ptr;
        ptr++;
        while (*ptr) {
            if (*ptr == '\\' && *(ptr+1)) {
                ptr += 2; /* Skip escaped character and the character after it */
            } else if (*ptr == quote) {
                type_end = ptr + 1;
                ptr++;
                break;
            } else {
                ptr++;
            }
        }
        if (!type_end) {
            return 0; /* Unterminated string */
        }
    } else {
        return 0; /* Type must be quoted */
    }
    
    /* Skip whitespace and find comma */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != ',') return 0;
    comma_pos = ptr;
    ptr++;
    
    /* Skip whitespace after comma */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    /* Find value - can be any format (string, number, array, etc.) */
    value_start = ptr;
    
    /* Track brackets to find the end of the value */
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
                if (bracket_depth == 0) {
                    value_end = ptr;
                    break;
                }
            }
        } else {
            if (*ptr == '\\' && *(ptr+1)) {
                ptr++; /* Skip next character */
            } else if (*ptr == string_quote) {
                in_string = 0;
            }
        }
        ptr++;
    }
    
    if (!value_end) return 0;
    
    /* Trim whitespace from value end */
    while (value_end > value_start && (*(value_end-1) == ' ' || *(value_end-1) == '\t' || 
                                       *(value_end-1) == '\n' || *(value_end-1) == '\r')) {
        value_end--;
    }
    
    /* Allocate and copy type string (removing outer quotes) */
    const char* type_content_start = type_start + 1; /* Skip opening quote */
    size_t type_len = type_end - type_content_start - 1; /* Exclude closing quote */
    *type_str = (char*)malloc(type_len + 1);
    if (!*type_str) return 0;
    
    /* Copy type, handling escaped characters */
    size_t out_pos = 0;
    for (size_t i = 0; i < type_len; i++) {
        if (type_content_start[i] == '\\' && i + 1 < type_len) {
            /* Handle escape sequences */
            i++;
            switch (type_content_start[i]) {
                case 'n': (*type_str)[out_pos++] = '\n'; break;
                case 't': (*type_str)[out_pos++] = '\t'; break;
                case 'r': (*type_str)[out_pos++] = '\r'; break;
                case '\\': (*type_str)[out_pos++] = '\\'; break;
                case '"': (*type_str)[out_pos++] = '"'; break;
                case '\'': (*type_str)[out_pos++] = '\''; break;
                default: (*type_str)[out_pos++] = type_content_start[i]; break;
            }
        } else {
            (*type_str)[out_pos++] = type_content_start[i];
        }
    }
    (*type_str)[out_pos] = '\0';
    
    /* Allocate and copy value string */
    size_t value_len = value_end - value_start;
    *value_str = (char*)malloc(value_len + 1);
    if (!*value_str) {
        free(*type_str);
        *type_str = NULL;
        return 0;
    }
    memcpy(*value_str, value_start, value_len);
    (*value_str)[value_len] = '\0';
    
    return 1;
}

/* Remove string quotes for types that shouldn't be stringified */
static int kh_destringify_value(kh_type_t type, const char* input, char** output) {
    if (!input || !output) return 0;
    
    *output = NULL;
    size_t len = strlen(input);
    
    /* ALL types should have their quotes removed for storage */
    /* Output formatting will add them back as needed */
    if (len >= 2 && ((input[0] == '"' && input[len-1] == '"') || 
                    (input[0] == '\'' && input[len-1] == '\''))) {
        /* Remove quotes */
        *output = (char*)malloc(len - 1);
        if (!*output) return 0;
        memcpy(*output, input + 1, len - 2);
        (*output)[len - 2] = '\0';
        return 1;
    }
    
    /* No quotes to remove - just copy */
    *output = (char*)malloc(len + 1);
    if (!*output) return 0;
    strcpy(*output, input);
    return 1;
}

static int kh_validate_typed_hashmap(const char* value);

/* Parse a sub-array and count its elements */
static inline int kh_count_array_elements(const char* array_str) {
    if (!array_str) return -1;
    
    const char* ptr = array_str;
    int bracket_depth = 0;
    int in_quotes = 0;
    char quote_char = 0;
    int element_count = 0;
    int has_content = 0;
    
    /* Skip leading whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Must start with opening bracket */
    if (*ptr != '[') return -1;
    ptr++; /* Skip opening bracket */
    bracket_depth = 1;
    
    /* Skip whitespace after opening bracket */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Check for empty array */
    if (*ptr == ']') return 0;
    
    while (*ptr && bracket_depth > 0) {
        char c = *ptr;
        
        if (!in_quotes) {
            if (c == '"' || c == '\'') {
                in_quotes = 1;
                quote_char = c;
                has_content = 1;
            } else if (c == '[') {
                bracket_depth++;
                has_content = 1;
            } else if (c == ']') {
                bracket_depth--;
                if (bracket_depth == 0) {
                    /* End of array - count the last element if there was content */
                    if (has_content) {
                        element_count++;
                    }
                    break;
                }
                has_content = 1;
            } else if (c == ',' && bracket_depth == 1) {
                /* Element separator at top level */
                if (has_content) {
                    element_count++;
                    has_content = 0;
                }
            } else if (!isspace(c)) {
                has_content = 1;
            }
        } else {
            /* Inside quotes */
            if (c == '\\' && *(ptr + 1)) {
                /* Skip escaped character */
                ptr++;
            } else if (c == quote_char) {
                /* Check if this quote is escaped */
                int escape_count = 0;
                const char* check_ptr = ptr - 1;
                while (check_ptr >= array_str && *check_ptr == '\\') {
                    escape_count++;
                    check_ptr--;
                }
                /* If even number of backslashes (including 0), the quote is not escaped */
                if (escape_count % 2 == 0) {
                    in_quotes = 0;
                    quote_char = 0;
                }
            }
        }
        
        ptr++;
    }
    
    return (bracket_depth == 0) ? element_count : -1;
}

/* Validate TYPED_ARRAY format - each element must be [type, value] */
static int kh_validate_typed_array(const char* value) {
    if (!value) return 0;
    
    /* Basic array validation first */
    if (!kh_validate_array_format(value)) return 0;
    
    const char* ptr = value;
    int bracket_depth = 0;
    int in_string = 0;
    char string_quote = 0;
    
    /* Skip to opening bracket */
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr != '[') return 0;
    ptr++;
    bracket_depth = 1;
    
    /* Skip whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Empty array is valid */
    if (*ptr == ']') return 1;
    
    /* Parse each element */
    while (*ptr && bracket_depth > 0) {
        /* Skip whitespace */
        while (*ptr && isspace(*ptr)) ptr++;
        
        /* Each element must start with '[' */
        if (*ptr != '[') return 0;
        
        /* Find the end of this element */
        const char* elem_start = ptr;
        int elem_bracket_depth = 0;
        ptr++;
        elem_bracket_depth = 1;
        
        while (*ptr && elem_bracket_depth > 0) {
            if (!in_string) {
                if (*ptr == '"' || *ptr == '\'') {
                    in_string = 1;
                    string_quote = *ptr;
                } else if (*ptr == '[') {
                    elem_bracket_depth++;
                } else if (*ptr == ']') {
                    elem_bracket_depth--;
                }
            } else {
                if (*ptr == '\\' && *(ptr + 1)) {
                    ptr++; /* Skip escaped character */
                } else if (*ptr == string_quote) {
                    in_string = 0;
                }
            }
            ptr++;
        }
        
        /* Extract element and validate it has exactly 2 parts */
        int elem_len = (int)(ptr - elem_start);
        char* elem_str = (char*)malloc((size_t)elem_len + 1);
        if (!elem_str) return 0;
        
        memcpy(elem_str, elem_start, (size_t)elem_len);
        elem_str[elem_len] = '\0';
        
        /* Parse the [type, value] pair */
        char* type_str = NULL;
        char* value_str = NULL;
        int valid = kh_parse_type_value_array(elem_str, &type_str, &value_str);
        
        if (valid) {
            /* Validate the type */
            kh_type_t type = kh_get_type_from_string(type_str);
            if (type == KH_TYPE_UNKNOWN) {
                valid = 0;
            } else {
                /* Recursively validate nested TYPED_ARRAY and TYPED_HASHMAP */
                if (type == KH_TYPE_TYPED_ARRAY) {
                    valid = kh_validate_typed_array(value_str);
                } else if (type == KH_TYPE_TYPED_HASHMAP) {
                    valid = kh_validate_typed_hashmap(value_str);
                } else {
                    /* Validate the value format for the type */
                    valid = kh_validate_value_format(type, value_str);
                }
            }
        }
        
        free(elem_str);
        free(type_str);
        free(value_str);
        
        if (!valid) return 0;
        
        /* Skip whitespace and check for comma or end */
        while (*ptr && isspace(*ptr)) ptr++;
        
        if (*ptr == ',') {
            ptr++;
            /* Skip whitespace after comma */
            while (*ptr && isspace(*ptr)) ptr++;
        } else if (*ptr == ']') {
            bracket_depth--;
            if (bracket_depth == 0) break;
        } else if (*ptr != '\0') {
            return 0; /* Invalid separator */
        }
    }
    
    return (bracket_depth == 0) ? 1 : 0;
}

/* Validate TYPED_HASHMAP format - each element must be [[type, key], [type, value]] */
static int kh_validate_typed_hashmap(const char* value) {
    if (!value) return 0;
    
    /* Basic array validation first */
    if (!kh_validate_array_format(value)) return 0;
    
    const char* ptr = value;
    int bracket_depth = 0;
    int in_string = 0;
    char string_quote = 0;
    
    /* Skip to opening bracket */
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr != '[') return 0;
    ptr++;
    bracket_depth = 1;
    
    /* Skip whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Empty array is valid hashmap */
    if (*ptr == ']') return 1;
    
    /* Parse each key-value pair */
    while (*ptr && bracket_depth > 0) {
        /* Skip whitespace */
        while (*ptr && isspace(*ptr)) ptr++;
        
        /* Each element must start with '[' for the pair */
        if (*ptr != '[') return 0;
        
        /* Find the end of this key-value pair */
        const char* pair_start = ptr;
        int pair_bracket_depth = 0;
        ptr++;
        pair_bracket_depth = 1;
        
        while (*ptr && pair_bracket_depth > 0) {
            if (!in_string) {
                if (*ptr == '"' || *ptr == '\'') {
                    in_string = 1;
                    string_quote = *ptr;
                } else if (*ptr == '[') {
                    pair_bracket_depth++;
                } else if (*ptr == ']') {
                    pair_bracket_depth--;
                }
            } else {
                /* Check bounds before accessing ptr-1 */
                if (*ptr == string_quote && (ptr == pair_start + 1 || *(ptr - 1) != '\\')) {
                    in_string = 0;
                } else if (*ptr == '\\' && *(ptr + 1)) {
                    /* Skip escaped character */
                    ptr++;
                }
            }
            ptr++;
        }
        
        /* Extract pair and validate it has exactly 2 elements (key and value) */
        int pair_len = (int)(ptr - pair_start);
        char* pair_str = (char*)malloc((size_t)pair_len + 1);
        if (!pair_str) return 0;
        
        memcpy(pair_str, pair_start, (size_t)pair_len);
        pair_str[pair_len] = '\0';
        
        /* Count elements in this pair - should be exactly 2 */
        int pair_element_count = kh_count_array_elements(pair_str);
        if (pair_element_count != 2) {
            free(pair_str);
            return 0;
        }
        
        /* Now validate each part is a valid [type, value] pair */
        const char* p = pair_str + 1; /* Skip opening bracket */
        int key_validated = 0;
        int value_validated = 0;
        
        /* Find and validate key [type, key] */
        while (*p && isspace(*p)) p++;
        if (*p == '[') {
            const char* key_start = p;
            int key_bracket_depth = 1;
            p++;
            in_string = 0;
            
            while (*p && key_bracket_depth > 0) {
                if (!in_string) {
                    if (*p == '"' || *p == '\'') {
                        in_string = 1;
                        string_quote = *p;
                    } else if (*p == '[') {
                        key_bracket_depth++;
                    } else if (*p == ']') {
                        key_bracket_depth--;
                    }
                } else {
                    /* Fixed: Check bounds before accessing p-1 */
                    if (*p == string_quote && (p == key_start + 1 || *(p - 1) != '\\')) {
                        in_string = 0;
                    } else if (*p == '\\' && *(p + 1)) {
                        /* Skip escaped character */
                        p++;
                    }
                }
                p++;
            }
            
            int key_len = (int)(p - key_start);
            char* key_str = (char*)malloc((size_t)key_len + 1);
            if (!key_str) {
                free(pair_str);
                return 0;
            }
            
            memcpy(key_str, key_start, (size_t)key_len);
            key_str[key_len] = '\0';
            
            /* Validate key format */
            char* key_type_str = NULL;
            char* key_value_str = NULL;
            if (kh_parse_type_value_array(key_str, &key_type_str, &key_value_str)) {
                kh_type_t key_type = kh_get_type_from_string(key_type_str);
                /* Keys can only be STRING or SCALAR */
                if (key_type == KH_TYPE_STRING || key_type == KH_TYPE_SCALAR) {
                    if (kh_validate_value_format(key_type, key_value_str)) {
                        key_validated = 1;
                    }
                }
            }
            
            free(key_str);
            free(key_type_str);
            free(key_value_str);
            
            if (!key_validated) {
                free(pair_str);
                return 0;
            }
        }
        
        /* Skip comma between key and value */
        while (*p && isspace(*p)) p++;
        if (*p == ',') p++;
        while (*p && isspace(*p)) p++;
        
        /* Find and validate value [type, value] */
        if (*p == '[') {
            const char* val_start = p;
            int val_bracket_depth = 1;
            p++;
            in_string = 0;
            
            while (*p && val_bracket_depth > 0) {
                if (!in_string) {
                    if (*p == '"' || *p == '\'') {
                        in_string = 1;
                        string_quote = *p;
                    } else if (*p == '[') {
                        val_bracket_depth++;
                    } else if (*p == ']') {
                        val_bracket_depth--;
                    }
                } else {
                    /* Fixed: Check bounds before accessing p-1 */
                    if (*p == string_quote && (p == val_start + 1 || *(p - 1) != '\\')) {
                        in_string = 0;
                    } else if (*p == '\\' && *(p + 1)) {
                        /* Skip escaped character */
                        p++;
                    }
                }
                p++;
            }
            
            int val_len = (int)(p - val_start);
            char* val_str = (char*)malloc((size_t)val_len + 1);
            if (!val_str) {
                free(pair_str);
                return 0;
            }
            
            memcpy(val_str, val_start, (size_t)val_len);
            val_str[val_len] = '\0';
            
            /* Validate value format */
            char* val_type_str = NULL;
            char* val_value_str = NULL;
            if (kh_parse_type_value_array(val_str, &val_type_str, &val_value_str)) {
                kh_type_t val_type = kh_get_type_from_string(val_type_str);
                if (val_type != KH_TYPE_UNKNOWN) {
                    /* Recursively validate nested types */
                    if (val_type == KH_TYPE_TYPED_ARRAY) {
                        value_validated = kh_validate_typed_array(val_value_str);
                    } else if (val_type == KH_TYPE_TYPED_HASHMAP) {
                        value_validated = kh_validate_typed_hashmap(val_value_str);
                    } else {
                        value_validated = kh_validate_value_format(val_type, val_value_str);
                    }
                }
            }
            
            free(val_str);
            free(val_type_str);
            free(val_value_str);
            
            if (!value_validated) {
                free(pair_str);
                return 0;
            }
        }
        
        free(pair_str);
        
        if (!key_validated || !value_validated) {
            return 0;
        }
        
        /* Skip whitespace and check for comma or end */
        while (*ptr && isspace(*ptr)) ptr++;
        
        if (*ptr == ',') {
            ptr++;
            /* Skip whitespace after comma */
            while (*ptr && isspace(*ptr)) ptr++;
        } else if (*ptr == ']') {
            bracket_depth--;
            if (bracket_depth == 0) break;
        } else if (*ptr != '\0') {
            return 0; /* Invalid separator */
        }
    }
    
    return (bracket_depth == 0) ? 1 : 0;
}

/* Normalize namespace value to camelCase */
static int kh_normalize_namespace_value(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    char* clean = (char*)malloc(strlen(input) + 1);
    if (!clean) return 0;
    
    kh_clean_string(input, clean, (int)strlen(input) + 1);
    
    /* Convert to lowercase for comparison */
    char* lower = (char*)malloc(strlen(clean) + 1);
    if (!lower) {
        free(clean);
        return 0;
    }
    strcpy(lower, clean);
    for (char* p = lower; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    }
    
    const char* result = NULL;
    if (strcmp(lower, "missionnamespace") == 0) {
        result = "missionNamespace";
    } else if (strcmp(lower, "missionprofilenamespace") == 0) {
        result = "missionProfileNamespace";
    } else if (strcmp(lower, "profilenamespace") == 0) {
        result = "profileNamespace";
    } else if (strcmp(lower, "servernamespace") == 0) {
        result = "serverNamespace";
    } else if (strcmp(lower, "uinamespace") == 0) {
        result = "uiNamespace";
    } else if (strcmp(lower, "parsingnamespace") == 0) {
        result = "parsingNamespace";
    }
    
    free(clean);
    free(lower);
    
    if (result) {
        strncpy_s(output, output_size, result, _TRUNCATE);
        return 1;
    }
    
    return 0;
}

/* Normalize side value to camelCase */
static int kh_normalize_side_value(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    char* clean = (char*)malloc(strlen(input) + 1);
    if (!clean) return 0;
    
    kh_clean_string(input, clean, (int)strlen(input) + 1);
    
    /* Convert to lowercase for comparison */
    char* lower = (char*)malloc(strlen(clean) + 1);
    if (!lower) {
        free(clean);
        return 0;
    }
    strcpy(lower, clean);
    for (char* p = lower; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    }
    
    const char* result = NULL;
    if (strcmp(lower, "west") == 0) {
        result = "west";
    } else if (strcmp(lower, "east") == 0) {
        result = "east";
    } else if (strcmp(lower, "resistance") == 0) {
        result = "resistance";
    } else if (strcmp(lower, "civilian") == 0) {
        result = "civilian";
    } else if (strcmp(lower, "blufor") == 0) {
        result = "blufor";
    } else if (strcmp(lower, "opfor") == 0) {
        result = "opfor";
    } else if (strcmp(lower, "independent") == 0) {
        result = "independent";
    } else if (strcmp(lower, "sidefriendly") == 0) {
        result = "sideFriendly";
    } else if (strcmp(lower, "sideenemy") == 0) {
        result = "sideEnemy";
    } else if (strcmp(lower, "sideambientlife") == 0) {
        result = "sideAmbientLife";
    } else if (strcmp(lower, "sideempty") == 0) {
        result = "sideEmpty";
    } else if (strcmp(lower, "sideunknown") == 0) {
        result = "sideUnknown";
    } else if (strcmp(lower, "sidelogic") == 0) {
        result = "sideLogic";
    }
    
    free(clean);
    free(lower);
    
    if (result) {
        strncpy_s(output, output_size, result, _TRUNCATE);
        return 1;
    }
    
    return 0;
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
    if (!g_rng_state.initialized) {
        g_rng_state.seed = (unsigned int)time(NULL) ^ (unsigned int)GetTickCount();
        /* Ensure seed is never zero */
        if (g_rng_state.seed == 0) g_rng_state.seed = 1;
        g_rng_state.initialized = 1;
    }
}

/* Fast random number generator using LCG */
static inline unsigned int kh_fast_rand(void) {
    g_rng_state.seed = g_rng_state.seed * 1103515245U + 12345U;
    return g_rng_state.seed;
}

#endif /* COMMON_DEFINES_HPP */