#ifndef PROCESS_KH_DATA_HPP
#define PROCESS_KH_DATA_HPP

#include "common_defines.h"
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

/* Data type enumeration */
typedef enum {
    KH_TYPE_ARRAY = 0,
    KH_TYPE_STRING = 1,
    KH_TYPE_SCALAR = 2,
    KH_TYPE_HASHMAP = 3,
    KH_TYPE_BOOL = 4,
    KH_TYPE_CODE = 5,
    KH_TYPE_TEXT = 6,
    KH_TYPE_UNKNOWN = -1
} kh_data_type_t;

/* Variable data structure */
struct kh_variable_s {
    char* name;
    kh_data_type_t type;
    char* value;
};

/* Hash table entry for indexed access - properly aligned */
#pragma pack(push, 4)
typedef struct {
    uint32_t name_hash;      /* Hash of variable name */
    uint32_t data_offset;    /* Offset to variable data in file */
    uint32_t reserved;       /* Reserved for future use/alignment */
    uint8_t deleted;         /* Tombstone for Robin Hood hashing */
    uint8_t distance;        /* Distance from ideal position (Robin Hood) */
    uint16_t padding;        /* Alignment padding */
} kh_hash_entry_t;
#pragma pack(pop)

/* Hash table info structure to pass around hash table data */
typedef struct {
    kh_hash_entry_t* entries;
    uint32_t size;
    uint32_t used_count;     /* Number of used entries */
    uint32_t deleted_count;  /* Number of deleted entries */
    int needs_rebuild;       /* Flag to indicate if rebuild is needed */
} kh_hash_table_info_t;

/* Binary file header structure - enhanced for indexed format with alignment */
#pragma pack(push, 4)
typedef struct {
    unsigned int magic;              /* Magic number */
    unsigned int version;            /* Format version */
    unsigned int variable_count;     /* Number of variables */
    unsigned int hash_table_size;    /* Size of hash table (0 for old format) */
    unsigned int data_section_offset; /* Offset to data section */
    unsigned int reserved1;          /* Reserved for alignment */
    unsigned int reserved2;          /* Reserved for future use */
    unsigned int reserved3;          /* Reserved for future use */
} kh_file_header_t;
#pragma pack(pop)

/* In-memory file representation */
typedef struct {
    char* filename;  /* Just the filename without path */
    char* full_path; /* Full path to the file */
    kh_variable_t* variables;
    int variable_count;
    kh_hash_table_info_t hash_info;
    int dirty;  /* Flag to track if file needs saving */
    int capacity; /* Current allocated capacity for variables */
} khdata_file_t;

/* Memory manager for all .khdata files */
typedef struct {
    khdata_file_t* files;
    int file_count;
    int file_capacity;
    int initialized;
    char base_path[MAX_FILE_PATH_LENGTH]; /* Base path to kh_data folder */
} khdata_memory_manager_t;

/* Global memory manager instance */
static khdata_memory_manager_t g_memory_manager = {0};

/* Type string lookup table for better performance */
typedef struct {
    const char* name;
    kh_data_type_t type;
} kh_type_mapping_t;

static const kh_type_mapping_t KH_TYPE_MAPPINGS[] = {
    {"ARRAY", KH_TYPE_ARRAY},
    {"STRING", KH_TYPE_STRING},
    {"SCALAR", KH_TYPE_SCALAR},
    {"HASHMAP", KH_TYPE_HASHMAP},
    {"BOOL", KH_TYPE_BOOL},
    {"CODE", KH_TYPE_CODE},
    {"TEXT", KH_TYPE_TEXT}
};

static const int KH_TYPE_MAPPING_COUNT = sizeof(KH_TYPE_MAPPINGS) / sizeof(kh_type_mapping_t);

/* Hash function for variable names - case insensitive */
static inline uint32_t kh_hash_variable_name(const char* name) {
    if (!name) return KH_HASH_EMPTY;
    
    /* FNV-1a constants for 32-bit */
    uint32_t hash = 2166136261U;  /* FNV offset basis */
    const uint32_t fnv_prime = 16777619U;  /* FNV prime */
    
    const char* ptr = name;
    
    while (*ptr) {
        /* Convert to lowercase for case-insensitive hashing */
        char c = *ptr;
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
        
        /* FNV-1a: hash = (hash XOR byte) * prime */
        hash ^= (uint32_t)c;
        hash *= fnv_prime;
        
        ptr++;
    }
    
    /* Ensure hash is never 0 (reserved for empty entries) */
    return (hash == KH_HASH_EMPTY) ? 1 : hash;
}

/* Calculate optimal hash table size */
static inline uint32_t kh_calculate_hash_table_size(uint32_t variable_count) {
    /* Check for potential overflow in load factor calculation */
    if (variable_count > UINT32_MAX / 2) {
        return KH_HASH_TABLE_MIN_SIZE; /* Fallback to minimum size */
    }
    
    /* Use next power of 2 that gives load factor <= 0.75 */
    uint32_t min_size = (uint32_t)((double)variable_count / KH_HASH_TABLE_LOAD_FACTOR);
    uint32_t size = KH_HASH_TABLE_MIN_SIZE;
    
    /* Ensure we don't overflow when doubling */
    while (size < min_size && size <= UINT32_MAX / 2) {
        size <<= 1;
    }
    
    /* Final bounds check */
    if (size < KH_HASH_TABLE_MIN_SIZE) {
        size = KH_HASH_TABLE_MIN_SIZE;
    }
    
    return size;
}

/* Robin Hood hashing with quadratic probing fallback - eliminates clustering */
static inline uint32_t kh_hash_table_find_robin_hood(kh_hash_entry_t* hash_table, uint32_t hash_table_size, 
                                                      uint32_t name_hash, int* found) {
    if (!hash_table || hash_table_size == 0 || name_hash == KH_HASH_EMPTY) {
        if (found) *found = 0;
        return 0;
    }
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    
    do {
        kh_hash_entry_t* entry = &hash_table[index];
        
        if (entry->name_hash == KH_HASH_EMPTY && !entry->deleted) {
            if (found) *found = 0;
            return index;
        }
        
        if (!entry->deleted && entry->name_hash == name_hash) {
            if (found) *found = 1;
            return index;
        }
        
        if (!entry->deleted && entry->distance < distance) {
            if (found) *found = 0;
            return index;
        }
        
        distance++;
        
        /* FIX: Prevent integer overflow in quadratic probing */
        if (distance > 65535) break; /* Reasonable upper limit */
        
        uint64_t next_offset = (uint64_t)distance * distance;
        if (next_offset > hash_table_size) {
            /* Linear probing fallback when quadratic would overflow */
            index = (original_index + distance) % hash_table_size;
        } else {
            index = (original_index + (uint32_t)next_offset) % hash_table_size;
        }
        
        if (distance >= hash_table_size) break; /* Prevent infinite loops */
        
    } while (1);
    
    if (found) *found = 0;
    return 0;
}

/* Robin Hood hash table insertion - more efficient than linear probing */
static inline int kh_hash_table_insert_robin_hood(kh_hash_entry_t* hash_table, uint32_t hash_table_size,
                                                   uint32_t name_hash, uint32_t data_offset) {
    if (!hash_table || hash_table_size == 0 || name_hash == KH_HASH_EMPTY) return 0;
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    
    /* Current entry to insert */
    kh_hash_entry_t new_entry = {name_hash, data_offset, 0, 0, 0, 0};
    
    while (distance < hash_table_size) {
        kh_hash_entry_t* entry = &hash_table[index];
        
        /* Empty slot or deleted slot */
        if (entry->name_hash == KH_HASH_EMPTY || entry->deleted) {
            *entry = new_entry;
            entry->distance = (uint8_t)distance;
            return 1;
        }
        
        /* Update existing entry */
        if (entry->name_hash == name_hash) {
            entry->data_offset = data_offset;
            return 1;
        }
        
        /* Robin Hood: if new entry is further from home, swap */
        if (distance > entry->distance) {
            kh_hash_entry_t temp = *entry;
            *entry = new_entry;
            entry->distance = (uint8_t)distance;
            
            /* Continue inserting the displaced entry */
            new_entry = temp;
            distance = temp.distance;
        }
        
        /* Move to next position with quadratic probing */
        distance++;
        index = (original_index + distance * distance) % hash_table_size;
    }
    
    return 0; /* Table full */
}

/* Enhanced hash table deletion with tombstoning */
static inline int kh_hash_table_delete(kh_hash_entry_t* hash_table, uint32_t hash_table_size, uint32_t name_hash) {
    if (!hash_table || hash_table_size == 0 || name_hash == KH_HASH_EMPTY) return 0;
    
    int found = 0;
    uint32_t index = kh_hash_table_find_robin_hood(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        hash_table[index].deleted = 1;
        hash_table[index].name_hash = KH_HASH_EMPTY;
        hash_table[index].data_offset = 0;
        return 1;
    }
    
    return 0;
}

/* Check if hash table needs rebuilding */
static inline int kh_should_rebuild_hash_table(kh_hash_table_info_t* hash_info, int variable_count) {
    if (!hash_info || !hash_info->entries) return 1;
    
    /* Rebuild if load factor too high */
    double load_factor = (double)(hash_info->used_count + hash_info->deleted_count) / hash_info->size;
    if (load_factor > 0.8) return 1;
    
    /* Rebuild if too many deletions */
    if (hash_info->deleted_count > hash_info->used_count / 2) return 1;
    
    /* Rebuild if size mismatch */
    uint32_t optimal_size = kh_calculate_hash_table_size(variable_count);
    if (hash_info->size < optimal_size / 2 || hash_info->size > optimal_size * 2) return 1;
    
    /* Rebuild if explicitly flagged */
    if (hash_info->needs_rebuild) return 1;
    
    return 0;
}

/* Convert type string to enum - optimized lookup */
static inline kh_data_type_t kh_get_type_from_string(const char* type_str) {
    if (!type_str) return KH_TYPE_UNKNOWN;
    
    char* clean_type = (char*)malloc(strlen(type_str) + 1);
    if (!clean_type) return KH_TYPE_UNKNOWN;
    
    int i, j = 0;
    
    /* Clean and convert to uppercase */
    for (i = 0; type_str[i] != '\0'; i++) {
        if (type_str[i] != '"') {
            if (type_str[i] >= 'a' && type_str[i] <= 'z') {
                clean_type[j++] = type_str[i] - 32; /* Convert to uppercase */
            } else {
                clean_type[j++] = type_str[i];
            }
        }
    }
    clean_type[j] = '\0';
    
    /* Use lookup table for better performance */
    kh_data_type_t result = KH_TYPE_UNKNOWN;
    for (i = 0; i < KH_TYPE_MAPPING_COUNT; i++) {
        if (strcmp(clean_type, KH_TYPE_MAPPINGS[i].name) == 0) {
            result = KH_TYPE_MAPPINGS[i].type;
            break;
        }
    }
    
    free(clean_type);
    return result;
}

/* Convert type enum to string */
static inline const char* kh_get_string_from_type(kh_data_type_t type) {
    if (type >= 0 && type < KH_TYPE_MAPPING_COUNT) {
        return KH_TYPE_MAPPINGS[type].name;
    }
    return "UNKNOWN";
}

/* Free variables array */
static inline void kh_free_variables(kh_variable_t* variables, int count) {
    int i;
    if (variables) {
        for (i = 0; i < count; i++) {
            free(variables[i].name);
            free(variables[i].value);
        }
        free(variables);
    }
}

/* Free hash table info structure */
static inline void kh_free_hash_table_info(kh_hash_table_info_t* hash_info) {
    if (hash_info) {
        free(hash_info->entries);
        hash_info->entries = NULL;
        hash_info->size = 0;
        hash_info->used_count = 0;
        hash_info->deleted_count = 0;
        hash_info->needs_rebuild = 0;
    }
}

/* Free a single khdata file structure */
static inline void kh_free_khdata_file(khdata_file_t* file) {
    if (!file) return;
    
    free(file->filename);
    free(file->full_path);
    kh_free_variables(file->variables, file->variable_count);
    kh_free_hash_table_info(&file->hash_info);
    
    memset(file, 0, sizeof(khdata_file_t));
}

/* Optimized hash table rebuild - only when necessary */
static inline int kh_rebuild_hash_table(khdata_file_t* file) {
    if (!file || file->variable_count <= 0) return 1;
    
    /* Check if rebuild is actually needed */
    if (!kh_should_rebuild_hash_table(&file->hash_info, file->variable_count)) {
        return 1; /* No rebuild needed */
    }
    
    /* Free existing hash table */
    kh_free_hash_table_info(&file->hash_info);
    
    /* Calculate new hash table size */
    uint32_t hash_table_size = kh_calculate_hash_table_size(file->variable_count);
    
    /* Allocate new hash table */
    file->hash_info.entries = (kh_hash_entry_t*)calloc(hash_table_size, sizeof(kh_hash_entry_t));
    if (!file->hash_info.entries) return 0;
    
    file->hash_info.size = hash_table_size;
    file->hash_info.used_count = 0;
    file->hash_info.deleted_count = 0;
    file->hash_info.needs_rebuild = 0;
    
    /* Rebuild hash table using Robin Hood hashing */
    for (int i = 0; i < file->variable_count; i++) {
        if (file->variables[i].name) {
            uint32_t name_hash = kh_hash_variable_name(file->variables[i].name);
            if (!kh_hash_table_insert_robin_hood(file->hash_info.entries, hash_table_size, name_hash, i)) {
                kh_free_hash_table_info(&file->hash_info);
                return 0;
            }
            file->hash_info.used_count++;
        }
    }
    
    return 1;
}

/* Smart hash table update - add single entry without full rebuild */
static inline int kh_hash_table_add_entry(kh_hash_table_info_t* hash_info, const char* name, int var_index) {
    if (!hash_info || !name || var_index < 0) return 0;
    
    /* Check if we need to rebuild first */
    if (kh_should_rebuild_hash_table(hash_info, hash_info->used_count + 1)) {
        hash_info->needs_rebuild = 1;
        return 0; /* Signal that rebuild is needed */
    }
    
    uint32_t name_hash = kh_hash_variable_name(name);
    if (kh_hash_table_insert_robin_hood(hash_info->entries, hash_info->size, name_hash, var_index)) {
        hash_info->used_count++;
        return 1;
    }
    
    /* Insertion failed, mark for rebuild */
    hash_info->needs_rebuild = 1;
    return 0;
}

/* Smart hash table removal - mark deleted without full rebuild */
static inline int kh_hash_table_remove_entry(kh_hash_table_info_t* hash_info, const char* name) {
    if (!hash_info || !name) return 0;
    
    uint32_t name_hash = kh_hash_variable_name(name);
    if (kh_hash_table_delete(hash_info->entries, hash_info->size, name_hash)) {
        hash_info->used_count--;
        hash_info->deleted_count++;
        
        /* Check if we have too many deletions */
        if (hash_info->deleted_count > hash_info->used_count / 2) {
            hash_info->needs_rebuild = 1;
        }
        
        return 1;
    }
    
    return 0;
}

/* Find file in memory by filename */
static inline khdata_file_t* kh_find_file_in_memory(const char* filename) {
    if (!filename || !g_memory_manager.initialized) return NULL;
    
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (g_memory_manager.files[i].filename && 
            kh_strcasecmp(g_memory_manager.files[i].filename, filename) == 0) {
            return &g_memory_manager.files[i];
        }
    }
    
    return NULL;
}

/* Check if file is in binary format and get version */
static int kh_get_file_format_version(const char* file_path, uint32_t* version) {
    if (!file_path || !version) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int result = 0;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0; /* File doesn't exist or can't be opened */
    }
    
    /* Try to read header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) == 1) {
        /* Check magic number */
        if (header.magic == KHDATA_MAGIC && header.version >= 1) {
            *version = header.version;
            result = 1;
        }
    }
    
    fclose(file);
    return result;
}

/* Check if file is already in binary format */
static int kh_is_file_binary(const char* file_path) {
    uint32_t version;
    return kh_get_file_format_version(file_path, &version);
}

/* Parse a single line from text format: variable_name=TYPE:value */
static int kh_parse_text_line(const char* line, char** name, kh_data_type_t* type, char** value) {
    if (!line || !name || !type || !value) return 0;
    
    *name = NULL;
    *value = NULL;
    *type = KH_TYPE_UNKNOWN;
    
    /* Skip empty lines and comments */
    const char* ptr = line;
    while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
    if (*ptr == '\0' || *ptr == '#' || *ptr == '\n' || *ptr == '\r') {
        return 0; /* Empty line or comment */
    }
    
    /* Find the equals sign */
    const char* equals_pos = strchr(ptr, '=');
    if (!equals_pos) return 0;
    
    /* Find the colon that separates type from value */
    const char* colon_pos = strchr(equals_pos + 1, ':');
    if (!colon_pos) return 0;
    
    /* Extract variable name */
    int name_len = (int)(equals_pos - ptr);
    while (name_len > 0 && (ptr[name_len-1] == ' ' || ptr[name_len-1] == '\t')) {
        name_len--; /* Trim trailing whitespace */
    }
    
    if (name_len <= 0) return 0;
    
    *name = (char*)malloc((size_t)name_len + 1);
    if (!*name) return 0;
    
    memcpy(*name, ptr, (size_t)name_len);
    (*name)[name_len] = '\0';
    
    /* Extract type */
    const char* type_start = equals_pos + 1;
    while (*type_start && (*type_start == ' ' || *type_start == '\t')) type_start++;
    
    int type_len = (int)(colon_pos - type_start);
    while (type_len > 0 && (type_start[type_len-1] == ' ' || type_start[type_len-1] == '\t')) {
        type_len--; /* Trim trailing whitespace */
    }
    
    if (type_len <= 0) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    char* type_str = (char*)malloc((size_t)type_len + 1);
    if (!type_str) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    memcpy(type_str, type_start, (size_t)type_len);
    type_str[type_len] = '\0';
    
    *type = kh_get_type_from_string(type_str);
    free(type_str);
    
    if (*type == KH_TYPE_UNKNOWN) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    /* Extract value */
    const char* value_start = colon_pos + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) value_start++;
    
    /* Calculate value length, trimming trailing whitespace and newlines */
    int value_len = (int)strlen(value_start);
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t' || 
                            value_start[value_len-1] == '\n' || value_start[value_len-1] == '\r')) {
        value_len--;
    }
    
    *value = (char*)malloc((size_t)value_len + 1);
    if (!*value) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    if (value_len > 0) {
        memcpy(*value, value_start, (size_t)value_len);
    }
    (*value)[value_len] = '\0';
    
    return 1;
}

/* Read text format file and convert to variables array */
static int kh_read_text_file(const char* file_path, kh_variable_t** variables, int* count) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file;
    char* line_buffer = NULL;
    int buffer_size = 1024;
    kh_variable_t* temp_variables = NULL;
    int temp_count = 0;
    int temp_capacity = 16;
    int success = 1;
    
    *variables = NULL;
    *count = 0;
    
    if (fopen_s(&file, file_path, "r") != 0) {
        return 0;
    }
    
    /* Allocate initial arrays */
    line_buffer = (char*)malloc((size_t)buffer_size);
    temp_variables = (kh_variable_t*)calloc((size_t)temp_capacity, sizeof(kh_variable_t));
    
    if (!line_buffer || !temp_variables) {
        success = 0;
        goto cleanup;
    }
    
    /* Read line by line */
    while (fgets(line_buffer, buffer_size, file)) {
        /* Resize line buffer if needed */
        while (strlen(line_buffer) == (size_t)buffer_size - 1 && line_buffer[buffer_size - 2] != '\n') {
            buffer_size *= 2;
            char* new_buffer = (char*)realloc(line_buffer, (size_t)buffer_size);
            if (!new_buffer) {
                success = 0;
                goto cleanup;
            }
            line_buffer = new_buffer;
            
            /* Continue reading the rest of the line */
            if (!fgets(line_buffer + strlen(line_buffer), buffer_size - (int)strlen(line_buffer), file)) {
                break;
            }
        }
        
        char* name = NULL;
        kh_data_type_t type;
        char* value = NULL;
        
        if (kh_parse_text_line(line_buffer, &name, &type, &value)) {
            /* Resize variables array if needed */
            if (temp_count >= temp_capacity) {
                temp_capacity *= 2;
                kh_variable_t* new_variables = (kh_variable_t*)realloc(temp_variables, 
                                                (size_t)temp_capacity * sizeof(kh_variable_t));
                if (!new_variables) {
                    free(name);
                    free(value);
                    success = 0;
                    goto cleanup;
                }
                temp_variables = new_variables;
                
                /* Initialize new memory to zero */
                memset(&temp_variables[temp_count], 0, 
                       (size_t)(temp_capacity - temp_count) * sizeof(kh_variable_t));
            }
            
            /* Store the variable */
            temp_variables[temp_count].name = name;
            temp_variables[temp_count].type = type;
            temp_variables[temp_count].value = value;
            temp_count++;
        }
    }
    
    /* Success - transfer ownership */
    *variables = temp_variables;
    *count = temp_count;
    temp_variables = NULL; /* Prevent cleanup */

cleanup:
    fclose(file);
    free(line_buffer);
    
    if (!success && temp_variables) {
        /* Free partially allocated variables on failure */
        for (int i = 0; i < temp_count; i++) {
            free(temp_variables[i].name);
            free(temp_variables[i].value);
        }
        free(temp_variables);
    }
    
    return success;
}

/* Write variables array to text format file */
static int kh_write_text_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0)) return 0;
    
    FILE* file;
    int success = 1;
    
    if (fopen_s(&file, file_path, "w") != 0) {
        return 0;
    }
    
    /* Write header comment */
    fprintf(file, "# Unbinarized KHDATA\n");
    fprintf(file, "# Format: variable_name=TYPE:value\n");
    fprintf(file, "# Allowed Types:\n     # BOOL:true\n     # SCALAR:0\n     # ARRAY:[element1, element2, element3]\n     # HASHMAP:[[key, value]]\n     # STRING:string\n     # TEXT:text\n     # CODE:code\n\n");
    
    /* Write variables */
    for (int i = 0; i < count; i++) {
        if (!variables[i].name || !variables[i].value) {
            success = 0;
            break;
        }
        
        const char* type_str = kh_get_string_from_type(variables[i].type);
        
        if (fprintf(file, "%s=%s:%s\n", 
                   variables[i].name, type_str, variables[i].value) < 0) {
            success = 0;
            break;
        }
    }
    
    fclose(file);
    return success;
}

/* Validate file integrity before reading */
static int kh_validate_file_integrity(const char* file_path, long file_size) {
    if (!file_path || file_size < 0) return 0;
    
    /* File must be at least large enough for header */
    if (file_size < (long)sizeof(kh_file_header_t)) return 0;
    
    /* File shouldn't be larger than our maximum limit */
    if (file_size > MAX_TOTAL_KHDATA_SIZE_BYTES) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int result = 1;
    
    if (fopen_s(&file, file_path, "rb") != 0) return 0;
    
    /* Read header for basic validation */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        result = 0;
    } else {
        /* Validate magic number and version */
        if (header.magic != KHDATA_MAGIC || header.version < 1 || header.version > 2) {
            result = 0;
        }
        
        /* Check if variable count is reasonable */
        if (header.variable_count > 10000) {
            result = 0;
        }
        
        /* For version 2, validate hash table size */
        if (header.version >= 2) {
            if (header.hash_table_size > 0 && header.hash_table_size < KH_HASH_TABLE_MIN_SIZE) {
                result = 0;
            }
            if (header.data_section_offset >= (uint32_t)file_size) {
                result = 0;
            }
        }
    }
    
    fclose(file);
    return result;
}

/* Read binary file with hash table support - returns hash table info */
static int kh_read_binary_file_with_hash(const char* file_path, kh_variable_t** variables, int* count, kh_hash_table_info_t* hash_info) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_variable_t* temp_variables = NULL;
    int success = 0;
    long file_size = 0;
    long bytes_read = 0;
    uint32_t format_version = 1;
    
    *variables = NULL;
    *count = 0;
    if (hash_info) {
        hash_info->entries = NULL;
        hash_info->size = 0;
        hash_info->used_count = 0;
        hash_info->deleted_count = 0;
        hash_info->needs_rebuild = 0;
    }
    
    /* Get and validate file size */
    file_size = kh_get_file_size(file_path);
    if (file_size <= 0 || file_size > MAX_TOTAL_KHDATA_SIZE_BYTES) {
        return 0;
    }
    
    /* Validate file integrity */
    if (!kh_validate_file_integrity(file_path, file_size)) {
        return 0;
    }
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0;
    }
    
    /* Read and validate header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    bytes_read += sizeof(kh_file_header_t);
    
    if (header.magic != KHDATA_MAGIC || header.version < 1 || header.version > 2) {
        fclose(file);
        return 0;
    }
    
    format_version = header.version;
    
    /* Validate variable count */
    if (header.variable_count > 10000 || 
        (header.variable_count > 0 && bytes_read >= file_size)) {
        fclose(file);
        return 0;
    }
    
    if (header.variable_count == 0) {
        fclose(file);
        return 1; /* Empty file is valid */
    }
    
    /* For version 2, read hash table */
    if (format_version >= 2 && header.hash_table_size > 0 && hash_info) {
        hash_info->entries = (kh_hash_entry_t*)calloc(header.hash_table_size, sizeof(kh_hash_entry_t));
        if (!hash_info->entries) {
            fclose(file);
            return 0;
        }
        hash_info->size = header.hash_table_size;
        
        if (fread(hash_info->entries, sizeof(kh_hash_entry_t), header.hash_table_size, file) != header.hash_table_size) {
            kh_free_hash_table_info(hash_info);
            fclose(file);
            return 0;
        }
        bytes_read += header.hash_table_size * sizeof(kh_hash_entry_t);
        
        /* Count used and deleted entries */
        for (uint32_t i = 0; i < header.hash_table_size; i++) {
            if (hash_info->entries[i].name_hash != KH_HASH_EMPTY) {
                if (hash_info->entries[i].deleted) {
                    hash_info->deleted_count++;
                } else {
                    hash_info->used_count++;
                }
            }
        }
        
        /* Seek to data section */
        if (fseek(file, header.data_section_offset, SEEK_SET) != 0) {
            kh_free_hash_table_info(hash_info);
            fclose(file);
            return 0;
        }
        bytes_read = header.data_section_offset;
    }
    
    /* Allocate variables array */
    temp_variables = (kh_variable_t*)calloc(header.variable_count, sizeof(kh_variable_t));
    if (!temp_variables) {
        if (hash_info) kh_free_hash_table_info(hash_info);
        fclose(file);
        return 0;
    }
    
    /* Read variables with comprehensive bounds checking */
    for (unsigned int i = 0; i < header.variable_count; i++) {
        unsigned int name_len, value_len, type_len;
        char* type_str = NULL;
        
        /* Read name length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&name_len, sizeof(unsigned int), 1, file) != 1 || 
            name_len == 0 || name_len > 1024) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Check bounds for name data */
        if (bytes_read + name_len > file_size) {
            goto cleanup_failure;
        }
        
        /* Allocate and read name */
        temp_variables[i].name = (char*)malloc(name_len + 1);
        if (!temp_variables[i].name || 
            fread(temp_variables[i].name, name_len, 1, file) != 1) {
            goto cleanup_failure;
        }
        temp_variables[i].name[name_len] = '\0';
        bytes_read += name_len;
        
        /* Validate name contains only printable characters */
        for (unsigned int j = 0; j < name_len; j++) {
            if (temp_variables[i].name[j] < 32 || temp_variables[i].name[j] == 127) {
                goto cleanup_failure;
            }
        }
        
        /* Read type length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&type_len, sizeof(unsigned int), 1, file) != 1 || 
            type_len == 0 || type_len >= 64) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Check bounds for type data */
        if (bytes_read + type_len > file_size) {
            goto cleanup_failure;
        }
        
        /* Allocate and read type */
        type_str = (char*)malloc(type_len + 1);
        if (!type_str || fread(type_str, type_len, 1, file) != 1) {
            free(type_str);
            goto cleanup_failure;
        }
        type_str[type_len] = '\0';
        bytes_read += type_len;
        
        /* Validate and convert type */
        temp_variables[i].type = kh_get_type_from_string(type_str);
        free(type_str);
        type_str = NULL;
        
        if (temp_variables[i].type == KH_TYPE_UNKNOWN) {
            goto cleanup_failure;
        }
        
        /* Read value length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&value_len, sizeof(unsigned int), 1, file) != 1 || 
            value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Allocate value buffer */
        temp_variables[i].value = (char*)malloc(value_len + 1);
        if (!temp_variables[i].value) {
            goto cleanup_failure;
        }
        
        /* Read value data if present */
        if (value_len > 0) {
            if (bytes_read + value_len > file_size || 
                fread(temp_variables[i].value, value_len, 1, file) != 1) {
                goto cleanup_failure;
            }
            bytes_read += value_len;
        }
        
        temp_variables[i].value[value_len] = '\0';
        (*count)++;
    }
    
    /* Success */
    *variables = temp_variables;
    success = 1;
    temp_variables = NULL;
    
cleanup_failure:
    fclose(file);
    
    if (!success) {
        if (temp_variables) {
            kh_free_variables(temp_variables, *count);
        }
        if (hash_info) {
            kh_free_hash_table_info(hash_info);
        }
        *variables = NULL;
        *count = 0;
    }
    
    return success;
}

/* Write binary file with hash table indexing - enhanced for indexed format */
static int kh_write_binary_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0) || count < 0) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_hash_entry_t* hash_table = NULL;
    int success = 0;
    char* temp_file_path = NULL;
    uint32_t hash_table_size = 0;
    uint32_t data_section_offset = 0;
    
    /* Validate input data before writing */
    for (int i = 0; i < count; i++) {
        if (!variables[i].name || !variables[i].value) {
            return 0;
        }
        
        /* Validate lengths */
        size_t name_len = strlen(variables[i].name);
        size_t value_len = strlen(variables[i].value);
        
        if (name_len == 0 || name_len > 1024 || value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
            return 0;
        }
        
        /* Validate type */
        if (variables[i].type == KH_TYPE_UNKNOWN) {
            return 0;
        }
        
        /* Validate name contains only safe characters */
        for (size_t j = 0; j < name_len; j++) {
            if (variables[i].name[j] < 32 || variables[i].name[j] == 127) {
                return 0;
            }
        }
    }
    
    /* Create hash table for indexed access */
    if (count > 0) {
        hash_table_size = kh_calculate_hash_table_size(count);
        hash_table = (kh_hash_entry_t*)calloc(hash_table_size, sizeof(kh_hash_entry_t));
        if (!hash_table) return 0;
        
        /* Build hash table using Robin Hood hashing */
        for (int i = 0; i < count; i++) {
            uint32_t name_hash = kh_hash_variable_name(variables[i].name);
            if (!kh_hash_table_insert_robin_hood(hash_table, hash_table_size, name_hash, i)) {
                /* Hash table insertion failed - should not happen with proper sizing */
                free(hash_table);
                return 0;
            }
        }
    }
    
    /* Calculate data section offset with proper alignment */
    data_section_offset = sizeof(kh_file_header_t) + (hash_table_size * sizeof(kh_hash_entry_t));
    /* Align to 8-byte boundary for better performance */
    data_section_offset = (data_section_offset + 7) & ~7;
    
    /* Create temporary file path for atomic write */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10);
    if (!temp_file_path) {
        free(hash_table);
        return 0;
    }
    
    if (strcpy_s(temp_file_path, path_len + 10, file_path) != 0 ||
        strcat_s(temp_file_path, path_len + 10, ".tmp") != 0) {
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Open temporary file for writing */
    if (fopen_s(&file, temp_file_path, "wb") != 0) {
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Write header with version 2 (indexed format) */
    memset(&header, 0, sizeof(header)); /* Clear reserved fields */
    header.magic = KHDATA_MAGIC;
    header.version = 2; /* Use version 2 for indexed format */
    header.variable_count = (unsigned int)count;
    header.hash_table_size = hash_table_size;
    header.data_section_offset = data_section_offset;
    
    if (fwrite(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        DeleteFileA(temp_file_path);
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Write hash table */
    if (hash_table_size > 0) {
        if (fwrite(hash_table, sizeof(kh_hash_entry_t), hash_table_size, file) != hash_table_size) {
            goto write_failure;
        }
    }
    
    /* Pad to data section alignment */
    long current_pos = ftell(file);
    if (current_pos < 0) {
        goto write_failure;
    }
    
    while ((uint32_t)current_pos < data_section_offset) {
        if (fputc(0, file) == EOF) {
            goto write_failure;
        }
        current_pos++;
    }
    
    /* Write variables */
    for (int i = 0; i < count; i++) {
        unsigned int name_len = (unsigned int)strlen(variables[i].name);
        unsigned int value_len = (unsigned int)strlen(variables[i].value);
        const char* type_str = kh_get_string_from_type(variables[i].type);
        unsigned int type_len = (unsigned int)strlen(type_str);
        
        /* Write name */
        if (fwrite(&name_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(variables[i].name, name_len, 1, file) != 1) {
            goto write_failure;
        }
        
        /* Write type */
        if (fwrite(&type_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(type_str, type_len, 1, file) != 1) {
            goto write_failure;
        }
        
        /* Write value */
        if (fwrite(&value_len, sizeof(unsigned int), 1, file) != 1) {
            goto write_failure;
        }
        
        if (value_len > 0) {
            if (fwrite(variables[i].value, value_len, 1, file) != 1) {
                goto write_failure;
            }
        }
    }
    
    /* Flush and close file */
    if (fflush(file) != 0) {
        goto write_failure;
    }
    fclose(file);
    
    /* Atomic replace: move temp file to final location */
    if (DeleteFileA(file_path) || GetLastError() == ERROR_FILE_NOT_FOUND) {
        if (MoveFileA(temp_file_path, file_path)) {
            success = 1;
        }
    }
    
    /* Cleanup temp file if move failed */
    if (!success) {
        DeleteFileA(temp_file_path);
    }
    
    free(hash_table);
    free(temp_file_path);
    return success;
    
write_failure:
    fclose(file);
    DeleteFileA(temp_file_path);
    free(hash_table);
    free(temp_file_path);
    return 0;
}

/* Load a single file from disk into memory */
static int kh_load_file_into_memory(const char* filename) {
    if (!filename || !g_memory_manager.initialized) return 0;
    
    char full_path[MAX_FILE_PATH_LENGTH];
    khdata_file_t* file_slot = NULL;
    
    /* Get full path */
    if (!kh_get_khdata_file_path(filename, full_path, sizeof(full_path))) {
        return 0;
    }
    
    /* Check if file exists */
    if (GetFileAttributesA(full_path) == INVALID_FILE_ATTRIBUTES) {
        return 0; /* File doesn't exist */
    }
    
    /* Check if already loaded */
    file_slot = kh_find_file_in_memory(filename);
    if (file_slot) {
        return 1; /* Already loaded */
    }
    
    /* Find empty slot or resize array */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (!g_memory_manager.files[i].filename) {
            file_slot = &g_memory_manager.files[i];
            break;
        }
    }
    
    if (!file_slot) {
        /* Need to resize array */
        if (g_memory_manager.file_count >= g_memory_manager.file_capacity) {
            int new_capacity = g_memory_manager.file_capacity * 2;
            if (new_capacity < 16) new_capacity = 16;
            
            khdata_file_t* new_files = (khdata_file_t*)realloc(g_memory_manager.files, 
                                                               new_capacity * sizeof(khdata_file_t));
            if (!new_files) return 0;
            
            /* Initialize new slots */
            memset(&new_files[g_memory_manager.file_capacity], 0, 
                   (new_capacity - g_memory_manager.file_capacity) * sizeof(khdata_file_t));
            
            g_memory_manager.files = new_files;
            g_memory_manager.file_capacity = new_capacity;
        }
        
        file_slot = &g_memory_manager.files[g_memory_manager.file_count++];
    }
    
    /* Initialize file slot */
    memset(file_slot, 0, sizeof(khdata_file_t));
    
    /* Convert to binary format if needed */
    if (!kh_is_file_binary(full_path)) {
        /* Read as text and convert */
        kh_variable_t* temp_variables = NULL;
        int temp_count = 0;
        
        if (!kh_read_text_file(full_path, &temp_variables, &temp_count)) {
            return 0;
        }
        
        /* Write as binary */
        if (!kh_write_binary_file(full_path, temp_variables, temp_count)) {
            kh_free_variables(temp_variables, temp_count);
            return 0;
        }
        
        kh_free_variables(temp_variables, temp_count);
    }
    
    /* Read binary file */
    if (!kh_read_binary_file_with_hash(full_path, &file_slot->variables, 
                                       &file_slot->variable_count, &file_slot->hash_info)) {
        return 0;
    }
    
    /* Store filename and path */
    file_slot->filename = (char*)malloc(strlen(filename) + 1);
    file_slot->full_path = (char*)malloc(strlen(full_path) + 1);
    
    if (!file_slot->filename || !file_slot->full_path) {
        kh_free_khdata_file(file_slot);
        return 0;
    }
    
    strcpy_s(file_slot->filename, strlen(filename) + 1, filename);
    strcpy_s(file_slot->full_path, strlen(full_path) + 1, full_path);
    
    file_slot->capacity = file_slot->variable_count;
    file_slot->dirty = 0;
    
    return 1;
}

/* Save a file from memory to disk */
static int kh_save_file_from_memory(khdata_file_t* file) {
    if (!file || !file->filename || !file->full_path || !file->dirty) {
        return 1;
    }
    
    // NEW: Ensure directory exists before saving
    char dir_path[MAX_FILE_PATH_LENGTH];
    if (kh_get_arma3_documents_path(dir_path, sizeof(dir_path))) {
        CreateDirectoryA(dir_path, NULL); // Recreate if deleted
    }
    
    if (!kh_write_binary_file(file->full_path, file->variables, file->variable_count)) {
        return 0;
    }
    
    file->dirty = 0;
    return 1;
}

/* Add new file to memory */
static khdata_file_t* kh_add_file_to_memory(const char* filename) {
    if (!filename || !g_memory_manager.initialized) return NULL;
    
    char full_path[MAX_FILE_PATH_LENGTH];
    khdata_file_t* file_slot = NULL;
    
    /* Get full path */
    if (!kh_get_khdata_file_path(filename, full_path, sizeof(full_path))) {
        return NULL;
    }
    
    /* Check if already exists */
    file_slot = kh_find_file_in_memory(filename);
    if (file_slot) {
        return file_slot;
    }
    
    /* Find empty slot or resize array */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (!g_memory_manager.files[i].filename) {
            file_slot = &g_memory_manager.files[i];
            break;
        }
    }
    
    if (!file_slot) {
        /* Need to resize array */
        if (g_memory_manager.file_count >= g_memory_manager.file_capacity) {
            int new_capacity = g_memory_manager.file_capacity * 2;
            if (new_capacity < 16) new_capacity = 16;
            
            khdata_file_t* new_files = (khdata_file_t*)realloc(g_memory_manager.files, 
                                                               new_capacity * sizeof(khdata_file_t));
            if (!new_files) return NULL;
            
            /* Initialize new slots */
            memset(&new_files[g_memory_manager.file_capacity], 0, 
                   (new_capacity - g_memory_manager.file_capacity) * sizeof(khdata_file_t));
            
            g_memory_manager.files = new_files;
            g_memory_manager.file_capacity = new_capacity;
        }
        
        file_slot = &g_memory_manager.files[g_memory_manager.file_count++];
    }
    
    /* Initialize new file */
    memset(file_slot, 0, sizeof(khdata_file_t));
    
    file_slot->filename = (char*)malloc(strlen(filename) + 1);
    file_slot->full_path = (char*)malloc(strlen(full_path) + 1);
    
    if (!file_slot->filename || !file_slot->full_path) {
        kh_free_khdata_file(file_slot);
        return NULL;
    }
    
    strcpy_s(file_slot->filename, strlen(filename) + 1, filename);
    strcpy_s(file_slot->full_path, strlen(full_path) + 1, full_path);
    
    file_slot->capacity = 16; /* Initial capacity */
    file_slot->variables = (kh_variable_t*)calloc(file_slot->capacity, sizeof(kh_variable_t));
    if (!file_slot->variables) {
        kh_free_khdata_file(file_slot);
        return NULL;
    }
    
    file_slot->dirty = 1;
    
    return file_slot;
}

/* Remove file from memory */
static int kh_remove_file_from_memory(const char* filename) {
    if (!filename || !g_memory_manager.initialized) return 0;
    
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (g_memory_manager.files[i].filename && 
            kh_strcasecmp(g_memory_manager.files[i].filename, filename) == 0) {
            
            /* Save if dirty before removing */
            if (g_memory_manager.files[i].dirty) {
                kh_save_file_from_memory(&g_memory_manager.files[i]);
            }
            
            kh_free_khdata_file(&g_memory_manager.files[i]);
            return 1;
        }
    }
    
    return 0;
}

/* Initialize memory manager and load all existing files */
static int kh_init_memory_manager(void) {
    static volatile long initialization_in_progress = 0;
    
    /* Atomic check and set */
    if (InterlockedCompareExchange(&initialization_in_progress, 1, 0) != 0) {
        /* Another thread is initializing, wait for completion */
        while (!g_memory_manager.initialized && initialization_in_progress) {
            Sleep(1);
        }
        return g_memory_manager.initialized ? 1 : 0;
    }
    
    /* We got the lock, check if already initialized */
    if (g_memory_manager.initialized) {
        InterlockedExchange(&initialization_in_progress, 0);
        return 1;
    }
    
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_pattern[MAX_FILE_PATH_LENGTH];
    
    /* Get base path */
    if (!kh_get_arma3_documents_path(g_memory_manager.base_path, sizeof(g_memory_manager.base_path))) {
        InterlockedExchange(&initialization_in_progress, 0);
        return 0;
    }
    
    /* Initialize memory manager */
    g_memory_manager.file_capacity = 16;
    g_memory_manager.files = (khdata_file_t*)calloc(g_memory_manager.file_capacity, sizeof(khdata_file_t));
    if (!g_memory_manager.files) {
        InterlockedExchange(&initialization_in_progress, 0);
        return 0;
    }
    
    g_memory_manager.file_count = 0;
    g_memory_manager.initialized = 1;
    
    /* Find all .khdata files */
    _snprintf_s(search_pattern, sizeof(search_pattern), _TRUNCATE, "%s\\*.khdata", g_memory_manager.base_path);
    
    find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        InterlockedExchange(&initialization_in_progress, 0);
        return 1; /* No files found, but initialization successful */
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            /* Load this file into memory */
            char* filename = find_data.cFileName;
            
            /* Remove .khdata extension for internal storage */
            int filename_len = (int)strlen(filename);
            if (filename_len > 7 && strcmp(filename + filename_len - 7, ".khdata") == 0) {
                filename[filename_len - 7] = '\0';
            }
            
            kh_load_file_into_memory(filename);
            
            /* Restore extension */
            if (filename_len > 7) {
                filename[filename_len - 7] = '.';
            }
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    InterlockedExchange(&initialization_in_progress, 0);
    return 1;
}

/* Cleanup memory manager and save all dirty files */
static void kh_cleanup_memory_manager(void) {
    if (!g_memory_manager.initialized) return;
    
    /* Save all dirty files */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (g_memory_manager.files[i].dirty) {
            kh_save_file_from_memory(&g_memory_manager.files[i]);
        }
        kh_free_khdata_file(&g_memory_manager.files[i]);
    }
    
    free(g_memory_manager.files);
    memset(&g_memory_manager, 0, sizeof(g_memory_manager));
}

/* Convert text format file to binary format */
static int kh_convert_text_to_binary(const char* file_path) {
    if (!file_path) return 0;
    
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    char* temp_file_path = NULL;
    int result = 0;
    
    /* Read text format */
    if (!kh_read_text_file(file_path, &variables, &variable_count)) {
        return 0;
    }
    
    /* Create temporary file path */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10); /* +10 for ".tmp" + null */
    if (!temp_file_path) {
        goto cleanup;
    }
    
    strcpy_s(temp_file_path, path_len + 10, file_path);
    strcat_s(temp_file_path, path_len + 10, ".tmp");
    
    /* Write binary format to temporary file */
    if (!kh_write_binary_file(temp_file_path, variables, variable_count)) {
        goto cleanup;
    }
    
    /* Replace original file with temporary file */
    if (DeleteFileA(file_path) && MoveFileA(temp_file_path, file_path)) {
        result = 1;
    } else {
        DeleteFileA(temp_file_path); /* Clean up temp file if move failed */
    }

cleanup:
    kh_free_variables(variables, variable_count);
    free(temp_file_path);
    return result;
}

/* Convert binary format file to text format */
static int kh_convert_binary_to_text(const char* file_path) {
    if (!file_path) return 0;
    
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    kh_hash_table_info_t hash_info = {0};
    char* temp_file_path = NULL;
    int result = 0;
    
    /* Read binary format */
    if (!kh_read_binary_file_with_hash(file_path, &variables, &variable_count, &hash_info)) {
        return 0;
    }
    
    /* Create temporary file path */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10); /* +10 for ".tmp" + null */
    if (!temp_file_path) {
        goto cleanup;
    }
    
    strcpy_s(temp_file_path, path_len + 10, file_path);
    strcat_s(temp_file_path, path_len + 10, ".tmp");
    
    /* Write text format to temporary file */
    if (!kh_write_text_file(temp_file_path, variables, variable_count)) {
        goto cleanup;
    }
    
    /* Replace original file with temporary file */
    if (DeleteFileA(file_path) && MoveFileA(temp_file_path, file_path)) {
        result = 1;
    } else {
        DeleteFileA(temp_file_path); /* Clean up temp file if move failed */
    }

cleanup:
    kh_free_variables(variables, variable_count);
    kh_free_hash_table_info(&hash_info);
    free(temp_file_path);
    return result;
}

/* Main UnbinarizeKHData function - convert binary to text format */
static int kh_unbinarize_khdata(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Check if file exists */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Check if already in text format */
    if (!kh_is_file_binary(file_path)) {
        strcpy_s(output, (size_t)output_size, "ALREADY_TEXT_FORMAT");
        result = 0;
        goto cleanup;
    }
    
    /* Convert binary to text */
    if (!kh_convert_binary_to_text(file_path)) {
        kh_set_error(output, output_size, "CONVERSION_FAILED");
        goto cleanup;
    }
    
    /* Remove from memory if loaded (will be reloaded as text format next time) */
    kh_remove_file_from_memory(clean_filename);
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    return result;
}

/* Main BinarizeKHData function - convert text to binary format */
static int kh_binarize_khdata(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Check if file exists */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Check if already in binary format */
    if (kh_is_file_binary(file_path)) {
        strcpy_s(output, (size_t)output_size, "ALREADY_BINARY_FORMAT");
        result = 0;
        goto cleanup;
    }
    
    /* Convert text to binary */
    if (!kh_convert_text_to_binary(file_path)) {
        kh_set_error(output, output_size, "CONVERSION_FAILED");
        goto cleanup;
    }
    
    /* Remove from memory if loaded (will be reloaded as binary format next time) */
    kh_remove_file_from_memory(clean_filename);
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    return result;
}

/* Find variable by name (case-insensitive) - linear search fallback */
static inline int kh_find_variable(kh_variable_t* variables, int count, const char* name) {
    if (!variables || !name || count <= 0) return -1;
    
    int i;
    for (i = 0; i < count; i++) {
        if (variables[i].name && kh_strcasecmp(variables[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Enhanced find variable with hash table lookup - using Robin Hood hashing */
static inline int kh_find_variable_indexed(kh_variable_t* variables, int count, const char* name,
                                          kh_hash_entry_t* hash_table, uint32_t hash_table_size) {
    if (!variables || !name || count <= 0 || !hash_table || hash_table_size == 0) {
        return kh_find_variable(variables, count, name); /* Fallback to linear search */
    }
    
    uint32_t name_hash = kh_hash_variable_name(name);
    int found = 0;
    uint32_t index = kh_hash_table_find_robin_hood(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        /* Hash found, verify the actual name matches */
        uint32_t var_index = hash_table[index].data_offset;
        if (var_index < (uint32_t)count && variables[var_index].name &&
            kh_strcasecmp(variables[var_index].name, name) == 0) {
            return (int)var_index;
        }
    }
    
    return -1; /* Not found */
}

/* Helper function to find variable using hash table if available */
static inline int kh_find_variable_optimal(kh_variable_t* variables, int count, const char* name,
                                           kh_hash_table_info_t* hash_info) {
    if (hash_info && hash_info->entries && hash_info->size > 0) {
        return kh_find_variable_indexed(variables, count, name, hash_info->entries, hash_info->size);
    } else {
        return kh_find_variable(variables, count, name);
    }
}

/* Format variable for output as ["TYPE", value] */
static int kh_format_variable_output(kh_data_type_t type, const char* value, char* output, int output_size) {
    if (!value || !output || output_size <= 0) return 0;
    
    const char* type_str = kh_get_string_from_type(type);
    
    switch (type) {
        case KH_TYPE_STRING:
        case KH_TYPE_TEXT:
        case KH_TYPE_CODE:
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", \"%s\"]", type_str, value) >= 0);
        case KH_TYPE_BOOL: {
            /* Normalize boolean values to canonical string representation */
            const char* normalized_value;
            
            /* Parse the stored boolean value */
            int bool_val = kh_parse_boolean(value);
            normalized_value = bool_val ? "true" : "false";
            
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, normalized_value) >= 0);
        } 
        default:
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, value) >= 0);
    }
}

/* Parse Arma 3 hashmap entry in format [key, value] - improved memory management */
static int kh_parse_hashmap_entry(const char* input, char** key, char** value) {
    if (!input || !key || !value) return 0;
    
    *key = NULL;
    *value = NULL;
    
    const char* ptr = input;
    int bracket_count = 0;
    size_t input_len = strlen(input);
    
    /* Basic input validation */
    if (input_len == 0) return 0;
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != '[') return 0;
    
    const char* bracket_start = ptr + 1;
    ptr = bracket_start;
    
    /* Find comma while tracking bracket depth */
    const char* comma_pos = NULL;
    const char* bracket_end = NULL;
    int found_comma = 0;
    
    while (*ptr && (size_t)(ptr - input) < input_len) {
        if (*ptr == '[') {
            bracket_count++;
        } else if (*ptr == ']') {
            bracket_count--;
            if (bracket_count < 0) {
                bracket_end = ptr;
                break;
            }
        } else if (*ptr == ',' && bracket_count == 0 && !found_comma) {
            comma_pos = ptr;
            found_comma = 1;
        }
        ptr++;
    }
    
    /* Validation: Must have found comma and closing bracket */
    if (!found_comma || !bracket_end || bracket_count != -1) {
        return 0;
    }
    
    /* Extract key - dynamically sized */
    size_t key_len = (size_t)(comma_pos - bracket_start);
    if (key_len == 0) return 0;
    
    /* Trim whitespace from key */
    while (key_len > 0 && (bracket_start[key_len-1] == ' ' || bracket_start[key_len-1] == '\t')) {
        key_len--;
    }
    
    if (key_len == 0) return 0;
    
    *key = (char*)malloc(key_len + 1);
    if (!*key) return 0;
    
    memcpy(*key, bracket_start, key_len);
    (*key)[key_len] = '\0';
    
    /* Remove quotes from key if present */
    if (key_len >= 2 && (*key)[0] == '"' && (*key)[key_len-1] == '"') {
        memmove(*key, *key + 1, key_len - 2);
        (*key)[key_len - 2] = '\0';
        key_len -= 2;
    }
    
    /* Validate key contains only safe characters */
    for (size_t i = 0; i < key_len; i++) {
        char c = (*key)[i];
        if (c < 32 || c == 127) {
            free(*key);
            *key = NULL;
            return 0;
        }
    }
    
    /* Extract value - dynamically sized */
    const char* value_start = comma_pos + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) value_start++;
    
    size_t value_len = (size_t)(bracket_end - value_start);
    
    /* Trim whitespace from value */
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t')) {
        value_len--;
    }
    
    *value = (char*)malloc(value_len + 1);
    if (!*value) {
        free(*key);
        *key = NULL;
        return 0;
    }
    
    if (value_len > 0) {
        memcpy(*value, value_start, value_len);
    }
    (*value)[value_len] = '\0';
    
    return 1;
}

/* Enhanced full hashmap parsing with depth limits and validation (no size limits) */
static int kh_parse_full_hashmap(const char* input, char*** entries, int max_entries) {
    if (!input || !entries || max_entries <= 0 || max_entries > 10000) return 0;
    
    const char* ptr = input;
    int entry_count = 0;
    int bracket_depth = 0;
    int max_depth = 64; /* Reasonable nesting limit increased */
    const char* entry_start = NULL;
    size_t input_len = strlen(input);
    
    /* Input validation */
    if (input_len == 0) return 0;
    
    /* Allocate array of string pointers */
    *entries = (char**)calloc((size_t)max_entries, sizeof(char*));
    if (!*entries) return 0;
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != '[') {
        free(*entries);
        *entries = NULL;
        return 0;
    }
    ptr++; /* Skip opening bracket */
    
    while (*ptr && entry_count < max_entries && (size_t)(ptr - input) < input_len) {
        if (*ptr == '[') {
            if (bracket_depth == 0) {
                entry_start = ptr;
            }
            bracket_depth++;
            
            if (bracket_depth > max_depth) {
                /* Cleanup on excessive nesting */
                for (int i = 0; i < entry_count; i++) {
                    free((*entries)[i]);
                }
                free(*entries);
                *entries = NULL;
                return 0;
            }
        } else if (*ptr == ']') {
            bracket_depth--;
            if (bracket_depth == 0 && entry_start) {
                /* Found complete entry */
                size_t entry_len = (size_t)(ptr - entry_start + 1);
                
                /* Allocate based on actual size needed */
                (*entries)[entry_count] = (char*)malloc(entry_len + 1);
                if (!(*entries)[entry_count]) {
                    /* Cleanup on allocation failure */
                    for (int i = 0; i < entry_count; i++) {
                        free((*entries)[i]);
                    }
                    free(*entries);
                    *entries = NULL;
                    return 0;
                }
                
                memcpy((*entries)[entry_count], entry_start, entry_len);
                (*entries)[entry_count][entry_len] = '\0';
                entry_count++;
                
                entry_start = NULL;
            } else if (bracket_depth < 0) {
                /* Mismatched brackets - cleanup */
                for (int i = 0; i < entry_count; i++) {
                    free((*entries)[i]);
                }
                free(*entries);
                *entries = NULL;
                return 0;
            }
        }
        ptr++;
    }
    
    /* Final validation */
    if (bracket_depth != -1) {
        /* Unbalanced brackets - cleanup */
        for (int i = 0; i < entry_count; i++) {
            free((*entries)[i]);
        }
        free(*entries);
        *entries = NULL;
        return 0;
    }
    
    return entry_count;
}

/* Enhanced hashmap merging with comprehensive validation (no size limits) */
static int kh_merge_hashmap(const char* existing_hashmap, const char* new_entry, char* result, int result_size) {
    if (!existing_hashmap || !new_entry || !result || result_size <= 0) return 0;
    
    /* Size validation */
    size_t existing_len = strlen(existing_hashmap);
    size_t new_entry_len = strlen(new_entry);
    
    if (existing_len == 0 || new_entry_len == 0) {
        return 0;
    }
    
    /* Pre-validate existing hashmap format */
    if (!kh_validate_hashmap_format(existing_hashmap)) {
        return 0;
    }
    
    char** entries = NULL;
    int entry_count = 0;
    char* key = NULL;
    char* value = NULL;
    char* temp_result = NULL;
    int ret = 0;
    
    /* Allocate temporary result buffer dynamically based on content size */
    size_t temp_size = existing_len + new_entry_len + 1024; /* Small safety margin */
    
    /* Check for overflow */
    if (temp_size < existing_len || temp_size < new_entry_len) {
        return 0; /* Overflow detected */
    }
    
    temp_result = (char*)malloc(temp_size);
    if (!temp_result) return 0;
    
    /* Initialize with existing hashmap */
    if (strcpy_s(temp_result, temp_size, existing_hashmap) != 0) {
        free(temp_result);
        return 0;
    }
    
    /* Try to parse as full hashmap first */
    entry_count = kh_parse_full_hashmap(new_entry, &entries, 16);
    
    if (entry_count > 0) {
        /* Process each entry */
        for (int i = 0; i < entry_count; i++) {
            if (kh_parse_hashmap_entry(entries[i], &key, &value)) {
                /* Calculate required space for formatted entry */
                size_t key_len = key ? strlen(key) : 0;
                size_t value_len = value ? strlen(value) : 0;
                size_t formatted_len = key_len + value_len + 20; /* Extra space for formatting */
                
                char* formatted_entry = (char*)malloc(formatted_len);
                if (!formatted_entry) {
                    free(key);
                    free(value);
                    goto cleanup;
                }
                
                if (_snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", 
                               key ? key : "", value ? value : "") < 0) {
                    free(formatted_entry);
                    free(key);
                    free(value);
                    goto cleanup;
                }
                
                /* Check if we have enough space in temp_result */
                size_t current_len = strlen(temp_result);
                size_t required_len = current_len + strlen(formatted_entry) + 10;
                
                /* Dynamically resize if needed */
                if (required_len >= temp_size) {
                    size_t new_temp_size = required_len + 1024;
                    
                    char* new_temp_result = (char*)realloc(temp_result, new_temp_size);
                    if (!new_temp_result) {
                        free(formatted_entry);
                        free(key);
                        free(value);
                        goto cleanup;
                    }
                    temp_result = new_temp_result;
                    temp_size = new_temp_size;
                }
                
                /* Add to temp_result */
                const char* trimmed = temp_result;
                while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
                
                if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                    if (_snprintf_s(temp_result, temp_size, _TRUNCATE, "[%s]", formatted_entry) < 0) {
                        free(formatted_entry);
                        free(key);
                        free(value);
                        goto cleanup;
                    }
                } else {
                    if (current_len > 0 && temp_result[current_len-1] == ']') {
                        if (_snprintf_s(temp_result, temp_size, _TRUNCATE, "%.*s, %s]", 
                                       (int)(current_len - 1), temp_result, formatted_entry) < 0) {
                            free(formatted_entry);
                            free(key);
                            free(value);
                            goto cleanup;
                        }
                    }
                }
                
                free(formatted_entry);
                free(key);
                free(value);
                key = NULL;
                value = NULL;
            }
        }
        
        /* Copy result back with bounds checking */
        if (strlen(temp_result) < (size_t)result_size) {
            if (strcpy_s(result, (size_t)result_size, temp_result) == 0) {
                ret = 1;
            }
        }
    } else {
        /* Fall back to single entry parsing */
        if (kh_parse_hashmap_entry(new_entry, &key, &value)) {
            size_t key_len = key ? strlen(key) : 0;
            size_t value_len = value ? strlen(value) : 0;
            size_t formatted_len = key_len + value_len + 20;
            
            char* formatted_entry = (char*)malloc(formatted_len);
            if (formatted_entry) {
                if (_snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", 
                               key ? key : "", value ? value : "") >= 0) {
                    
                    const char* trimmed = existing_hashmap;
                    while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
                    
                    if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                        if (strlen(formatted_entry) + 2 < (size_t)result_size) {
                            if (_snprintf_s(result, (size_t)result_size, _TRUNCATE, "[%s]", formatted_entry) >= 0) {
                                ret = 1;
                            }
                        }
                    } else {
                        size_t existing_len_local = strlen(existing_hashmap);
                        if (existing_len_local + strlen(formatted_entry) + 10 < (size_t)result_size && 
                            existing_len_local > 0 && existing_hashmap[existing_len_local-1] == ']') {
                            if (_snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.*s, %s]", 
                                           (int)(existing_len_local - 1), existing_hashmap, formatted_entry) >= 0) {
                                ret = 1;
                            }
                        }
                    }
                }
                free(formatted_entry);
            }
        }
    }

cleanup:
    /* Clean up dynamic arrays */
    if (entries) {
        for (int i = 0; i < entry_count; i++) {
            free(entries[i]);
        }
        free(entries);
    }
    free(key);
    free(value);
    free(temp_result);
    return ret;
}

/* Enhanced value combination with better memory handling */
static int kh_combine_values(kh_data_type_t type, const char* existing_value, const char* new_value, 
                           int overwrite_flag, char* result, int result_size) {
    if (!existing_value || !new_value || !result || result_size <= 0) return 0;
    
    char* clean_existing = (char*)malloc(strlen(existing_value) + 1);
    char* clean_new = (char*)malloc(strlen(new_value) + 1);
    int ret = 0;
    
    if (!clean_existing || !clean_new) goto cleanup;
    
    kh_clean_string(existing_value, clean_existing, (int)strlen(existing_value) + 1);
    kh_clean_string(new_value, clean_new, (int)strlen(new_value) + 1);
    
    switch (type) {
        case KH_TYPE_BOOL:
            if (overwrite_flag) {
                strcpy_s(result, (size_t)result_size, clean_new);
            } else {
                /* Toggle boolean */
                int current_value = kh_parse_boolean(clean_existing);
                strcpy_s(result, (size_t)result_size, current_value ? "false" : "true");
            }
            ret = 1;
            break;
            
        case KH_TYPE_HASHMAP:
            if (overwrite_flag) {
                strcpy_s(result, (size_t)result_size, clean_new);
                ret = 1;
            } else {
                ret = kh_merge_hashmap(clean_existing, clean_new, result, result_size);
            }
            break;
            
        case KH_TYPE_SCALAR:
            if (overwrite_flag) {
                strcpy_s(result, (size_t)result_size, clean_new);
            } else {
                /* Use strtod with error checking */
                char* endptr1, *endptr2;
                double existing_num = strtod(clean_existing, &endptr1);
                double new_num = strtod(clean_new, &endptr2);
                
                /* Only add if both conversions were successful */
                if (endptr1 != clean_existing && *endptr1 == '\0' &&
                    endptr2 != clean_new && *endptr2 == '\0') {
                    _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.6g", existing_num + new_num);
                } else {
                    /* If conversion failed, treat as string concatenation */
                    if (strlen(clean_existing) + strlen(clean_new) < (size_t)result_size) {
                        _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                    }
                }
            }
            ret = 1;
            break;
            
        case KH_TYPE_ARRAY:
            if (overwrite_flag) {
                strcpy_s(result, (size_t)result_size, clean_new);
            } else {
                size_t existing_len = strlen(clean_existing);
                size_t new_len = strlen(clean_new);
                
                if (existing_len >= 2 && new_len >= 2) {
                    if (existing_len == 2 || (existing_len == 3 && clean_existing[1] == ' ')) {
                        strcpy_s(result, (size_t)result_size, clean_new);
                    } else if (existing_len + new_len < (size_t)result_size) {
                        _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.*s,%s", (int)(existing_len - 1), clean_existing, clean_new + 1);
                    }
                }
            }
            ret = 1;
            break;
            
        default:
            if (overwrite_flag) {
                strcpy_s(result, (size_t)result_size, clean_new);
            } else {
                if (strlen(clean_existing) + strlen(clean_new) < (size_t)result_size) {
                    _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                }
            }
            ret = 1;
            break;
    }

cleanup:
    free(clean_existing);
    free(clean_new);
    return ret;
}

/* Read from memory instead of disk */
static int kh_read_khdata_variable_slice(const char* filename, const char* variable_name, const char* slice_index_str, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    khdata_file_t* file = NULL;
    int var_index;
    int slice_index = -1;
    char* formatted_output = NULL;
    int result = 1;
    
    /* Allocate memory for cleaned strings */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_var_name || !clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    if (slice_index_str && strlen(slice_index_str) > 0) {
        char* endptr;
        long temp_slice_index = strtol(slice_index_str, &endptr, 10);
        
        /* Validate conversion and range */
        if (endptr == slice_index_str || *endptr != '\0' || temp_slice_index < 0 || temp_slice_index > INT_MAX) {
            kh_set_error(output, output_size, "INVALID SLICE INDEX");
            goto cleanup;
        }
        
        slice_index = (int)temp_slice_index;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        kh_set_error(output, output_size, "FILE NOT FOUND OR LOAD FAILED");
        goto cleanup;
    }
    
    /* Find file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        kh_set_error(output, output_size, "FILE NOT FOUND IN MEMORY");
        goto cleanup;
    }
    
    /* Special case: return all variable names if var_name == filename */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        size_t result_size = (size_t)file->variable_count * 256 + 100;
        char* result_str = (char*)malloc(result_size);
        if (!result_str) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        
        strcpy_s(result_str, result_size, "[");
        for (int i = 0; i < file->variable_count; i++) {
            if (i > 0) strcat_s(result_str, result_size, ", ");
            strcat_s(result_str, result_size, "\"");
            strcat_s(result_str, result_size, file->variables[i].name);
            strcat_s(result_str, result_size, "\"");
        }
        strcat_s(result_str, result_size, "]");
        
        /* Handle UTF-8 aware slicing */
        if (slice_index >= 0) {
            int data_len = (int)strlen(result_str);
            int slice_start = slice_index * SLICE_SIZE;
            int slice_end = slice_start + SLICE_SIZE;
            
            if (slice_start >= data_len) {
                kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
            } else {
                if (slice_end > data_len) slice_end = data_len;
                
                /* Ensure we don't break UTF-8 characters */
                slice_end = kh_utf8_safe_slice_end(result_str, data_len, slice_end);
                
                int slice_length = slice_end - slice_start;
                if (slice_length >= output_size) slice_length = output_size - 1;
                
                memcpy(output, result_str + slice_start, (size_t)slice_length);
                output[slice_length] = '\0';
                result = 0;
            }
        } else {
            if (strlen(result_str) >= SLICE_SIZE) {
                kh_set_error(output, output_size, "DATA TOO LARGE WITHOUT SLICING");
            } else {
                strcpy_s(output, (size_t)output_size, result_str);
                result = 0;
            }
        }
        
        free(result_str);
        goto cleanup;
    }
    
    /* Find specific variable */
    var_index = kh_find_variable_optimal(file->variables, file->variable_count, clean_var_name, &file->hash_info);
    if (var_index == -1) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        goto cleanup;
    }
    
    /* Format output */
    size_t formatted_len = strlen(file->variables[var_index].value) + 50;
    formatted_output = (char*)malloc(formatted_len);
    if (!formatted_output) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    if (!kh_format_variable_output(file->variables[var_index].type, file->variables[var_index].value, formatted_output, (int)formatted_len)) {
        kh_set_error(output, output_size, "OUTPUT FORMATTING FAILED");
        goto cleanup;
    }
    
    /* Handle UTF-8 aware slicing */
    if (slice_index >= 0) {
        int data_len = (int)strlen(formatted_output);
        int slice_start = slice_index * SLICE_SIZE;
        int slice_end = slice_start + SLICE_SIZE;
        
        if (slice_start >= data_len) {
            kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
        } else {
            if (slice_end > data_len) slice_end = data_len;
            
            /* Ensure we don't break UTF-8 characters */
            slice_end = kh_utf8_safe_slice_end(formatted_output, data_len, slice_end);
            
            int slice_length = slice_end - slice_start;
            if (slice_length >= output_size) slice_length = output_size - 1;
            
            memcpy(output, formatted_output + slice_start, (size_t)slice_length);
            output[slice_length] = '\0';
            result = 0;
        }
    } else {
        if (strlen(formatted_output) >= SLICE_SIZE) {
            kh_set_error(output, output_size, "DATA TOO LARGE WITHOUT SLICING");
        } else {
            strcpy_s(output, (size_t)output_size, formatted_output);
            result = 0;
        }
    }

cleanup:
    free(clean_var_name);
    free(clean_filename);
    free(formatted_output);
    return result;
}

/* Delete entire KHData file from memory and disk */
static int kh_delete_khdata_file(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Remove from memory first */
    kh_remove_file_from_memory(clean_filename);
    
    /* Check if file exists on disk */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Attempt to delete the file from disk */
    if (DeleteFileA(file_path)) {
        strcpy_s(output, (size_t)output_size, "SUCCESS");
        result = 0;
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_ACCESS_DENIED:
                kh_set_error(output, output_size, "ACCESS DENIED - FILE MAY BE IN USE");
                break;
            case ERROR_SHARING_VIOLATION:
                kh_set_error(output, output_size, "FILE IS IN USE BY ANOTHER PROCESS");
                break;
            default:
                kh_set_error(output, output_size, "FAILED TO DELETE FILE");
                break;
        }
    }

cleanup:
    free(clean_filename);
    return result;
}

/* Delete specific variable from KHData file in memory and sync to disk */
static int kh_delete_khdata_variable(const char* filename, const char* variable_name, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char* clean_filename = NULL;
    char* clean_var_name = NULL;
    khdata_file_t* file = NULL;
    int var_index;
    int result = 1;
    
    /* Allocate memory for cleaned strings */
    clean_filename = (char*)malloc(strlen(filename) + 1);
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    
    if (!clean_filename || !clean_var_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        kh_set_error(output, output_size, "FILE NOT FOUND OR LOAD FAILED");
        goto cleanup;
    }
    
    /* Find file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        kh_set_error(output, output_size, "FILE NOT FOUND IN MEMORY");
        goto cleanup;
    }
    
    /* Handle empty file */
    if (file->variable_count == 0) {
        kh_set_error(output, output_size, "FILE IS EMPTY");
        goto cleanup;
    }
    
    /* Find variable to delete */
    var_index = kh_find_variable_optimal(file->variables, file->variable_count, clean_var_name, &file->hash_info);
    if (var_index == -1) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        goto cleanup;
    }
    
    /* If this is the only variable, delete the entire file */
    if (file->variable_count == 1) {
        /* Remove from memory and delete from disk */
        kh_remove_file_from_memory(clean_filename);
        if (DeleteFileA(file->full_path)) {
            strcpy_s(output, (size_t)output_size, "SUCCESS");
            result = 0;
        } else {
            kh_set_error(output, output_size, "FAILED TO DELETE FILE");
        }
        goto cleanup;
    }
    
    /* Remove from hash table first */
    kh_hash_table_remove_entry(&file->hash_info, clean_var_name);
    
    /* Free the variable to be deleted */
    free(file->variables[var_index].name);
    free(file->variables[var_index].value);
    
    /* Shift remaining variables down */
    for (int i = var_index; i < file->variable_count - 1; i++) {
        file->variables[i] = file->variables[i + 1];
    }
    
    /* Clear the last variable */
    memset(&file->variables[file->variable_count - 1], 0, sizeof(kh_variable_t));
    file->variable_count--;
    
    /* Update hash table indices for shifted variables */
    for (int i = 0; i < file->hash_info.size; i++) {
        if (file->hash_info.entries[i].name_hash != KH_HASH_EMPTY && 
            !file->hash_info.entries[i].deleted) {
            if (file->hash_info.entries[i].data_offset > (uint32_t)var_index) {
                file->hash_info.entries[i].data_offset--;
            }
        }
    }
    
    /* Only rebuild hash table if necessary */
    if (kh_should_rebuild_hash_table(&file->hash_info, file->variable_count)) {
        if (!kh_rebuild_hash_table(file)) {
            kh_set_error(output, output_size, "FAILED TO REBUILD HASH TABLE");
            goto cleanup;
        }
    }
    
    /* Mark as dirty and save */
    file->dirty = 1;
    if (!kh_save_file_from_memory(file)) {
        kh_set_error(output, output_size, "FAILED TO SAVE FILE");
        goto cleanup;
    }
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    free(clean_var_name);
    return result;
}

/* Calculate the formatted size of a variable's output from memory */
static long kh_get_variable_formatted_size(const char* filename, const char* variable_name) {
    if (!filename || !variable_name) return -1;
    
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    khdata_file_t* file = NULL;
    int var_index;
    long total_size = 0;
    
    /* Allocate memory for cleaned strings */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_filename = (char*)malloc(strlen(filename) + 1);
    
    if (!clean_var_name || !clean_filename) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Find file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Special case: variable name equals filename (return size of all variable names array) */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        total_size = 2; /* Opening and closing brackets */
        
        for (int i = 0; i < file->variable_count; i++) {
            if (i > 0) {
                total_size += 2; /* ", " separator */
            }
            if (file->variables[i].name) {
                total_size += (long)strlen(file->variables[i].name) + 2; /* Name with quotes */
            }
        }
        
        free(clean_var_name);
        free(clean_filename);
        return total_size;
    }
    
    /* Find specific variable */
    var_index = kh_find_variable_optimal(file->variables, file->variable_count, clean_var_name, &file->hash_info);
    if (var_index == -1) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Calculate size of formatted output ["TYPE", value] */
    const char* type_str = kh_get_string_from_type(file->variables[var_index].type);
    if (type_str && file->variables[var_index].value) {
        total_size = (long)strlen(type_str) + (long)strlen(file->variables[var_index].value) + 6; /* ["", ] formatting */
    }
    
    free(clean_var_name);
    free(clean_filename);
    return total_size;
}

/* Calculate how many slices a variable would need - optimized */
static int kh_calculate_slice_count(const char* filename, const char* variable_name, char* output, int output_size) {
    /* Validate inputs */
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    long data_size = kh_get_variable_formatted_size(filename, variable_name);
    
    if (data_size < 0) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND OR CONVERSION FAILED");
        return 1;
    }
    
    if (data_size == 0) {
        strcpy_s(output, (size_t)output_size, "1"); /* Empty data still needs 1 slice */
        return 0;
    }
    
    /* Calculate number of slices needed (ceiling division) */
    int slice_count = (int)((data_size + SLICE_SIZE - 1) / SLICE_SIZE);
    
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%d", slice_count);
    return 0;
}

/* Write to memory and sync to disk with optimized hash table management */
static int kh_write_khdata_variable(const char* filename, const char* variable_name, const char* value, 
                                  const char* type_str, const char* overwrite_str, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char* clean_var_name = NULL;
    char* clean_value = NULL;
    char* combined_value = NULL;
    char* clean_filename = NULL;
    kh_data_type_t data_type;
    khdata_file_t* file = NULL;
    int var_index;
    int overwrite_flag;
    int result = 1;
    
    /* Validate type */
    data_type = kh_get_type_from_string(type_str);
    if (data_type == KH_TYPE_UNKNOWN) {
        kh_set_error(output, output_size, "UNKNOWN TYPE");
        return 1;
    }
    
    /* Parse overwrite flag */
    overwrite_flag = (overwrite_str && strlen(overwrite_str) > 0) ? kh_parse_boolean(overwrite_str) : 1;
    
    /* Check file limit before proceeding */
    if (!kh_check_file_limit(filename, output, output_size)) {
        return 1;
    }
    
    /* Allocate memory for cleaned strings - account for UTF-8 expansion */
    size_t var_name_len = strlen(variable_name) + 1;
    size_t value_len = strlen(value) + 1;
    size_t filename_len = strlen(filename) + 1;
    
    clean_var_name = (char*)malloc(var_name_len);
    clean_value = (char*)malloc(value_len);
    clean_filename = (char*)malloc(filename_len);
    combined_value = (char*)malloc(value_len * 2 + 1024); /* Extra space for UTF-8 and combination */
    
    if (!clean_var_name || !clean_value || !clean_filename || !combined_value) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)var_name_len);
    kh_clean_string(value, clean_value, (int)value_len);
    kh_clean_string(filename, clean_filename, (int)filename_len);

    /* Validate UTF-8 in cleaned strings */
    if (!kh_validate_utf8_string(clean_var_name, (int)strlen(clean_var_name))) {
        kh_set_error(output, output_size, "INVALID UTF-8 IN VARIABLE NAME");
        goto cleanup;
    }
    
    if (!kh_validate_utf8_string(clean_value, (int)strlen(clean_value))) {
        kh_set_error(output, output_size, "INVALID UTF-8 IN VALUE");
        goto cleanup;
    }

    /* Variable name security check */
    if (!kh_validate_variable_name(clean_var_name)) {
        kh_set_error(output, output_size, "INVALID VARIABLE NAME - ONLY ALPHANUMERIC AND UNDERSCORE ALLOWED");
        goto cleanup;
    }
    
    /* Value format validation based on data type */
    if (!kh_validate_value_format((int)data_type, clean_value)) {
        switch (data_type) {
            case KH_TYPE_SCALAR:
                kh_set_error(output, output_size, "INVALID SCALAR VALUE - MUST BE A VALID NUMBER");
                break;
            case KH_TYPE_ARRAY:
                kh_set_error(output, output_size, "INVALID ARRAY FORMAT - CHECK BRACKET MATCHING");
                break;
            case KH_TYPE_HASHMAP:
                kh_set_error(output, output_size, "INVALID HASHMAP FORMAT - CHECK BRACKET MATCHING AND STRUCTURE");
                break;
            case KH_TYPE_BOOL:
                kh_set_error(output, output_size, "INVALID BOOLEAN VALUE - MUST BE true/false/1/0");
                break;
            default:
                kh_set_error(output, output_size, "INVALID VALUE FORMAT FOR TYPE");
                break;
        }
        goto cleanup;
    }

    /* Allow empty strings for certain data types */
    if (strlen(clean_value) == 0 && data_type != KH_TYPE_STRING && data_type != KH_TYPE_TEXT && data_type != KH_TYPE_CODE) {
        kh_set_error(output, output_size, "EMPTY VALUE");
        goto cleanup;
    }
    
    /* Load file into memory if not already loaded, or create new one */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        /* Try to load from disk */
        if (!kh_load_file_into_memory(clean_filename)) {
            /* File doesn't exist, create new one */
            file = kh_add_file_to_memory(clean_filename);
            if (!file) {
                kh_set_error(output, output_size, "FAILED TO CREATE FILE IN MEMORY");
                goto cleanup;
            }
        } else {
            file = kh_find_file_in_memory(clean_filename);
        }
    }
    
    if (!file) {
        kh_set_error(output, output_size, "FILE NOT FOUND IN MEMORY");
        goto cleanup;
    }
    
    /* Find existing variable */
    var_index = kh_find_variable_optimal(file->variables, file->variable_count, clean_var_name, &file->hash_info);
    
    if (var_index != -1 && !overwrite_flag) {
        /* Check for type mismatch */
        if (file->variables[var_index].type != data_type) {
            kh_set_error(output, output_size, "TYPE MISMATCH - USE OVERWRITE TO CHANGE TYPE");
            goto cleanup;
        }

        /* Combine with existing value */
        if (!kh_combine_values(data_type, file->variables[var_index].value, clean_value, 
                             overwrite_flag, combined_value, (int)(value_len * 2 + 1024))) {
            kh_set_error(output, output_size, "VALUE COMBINATION FAILED");
            goto cleanup;
        }
        
        /* Update existing variable */
        free(file->variables[var_index].value);
        size_t combined_len = strlen(combined_value) + 1;
        file->variables[var_index].value = (char*)malloc(combined_len);
        if (!file->variables[var_index].value) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(file->variables[var_index].value, combined_len, combined_value);
        file->variables[var_index].type = data_type;
    } else {
        /* Add new variable or replace existing */
        if (var_index == -1) {
            /* Need to add new variable */
            if (file->variable_count >= file->capacity) {
                /* Resize variables array */
                int new_capacity = file->capacity * 2;
                if (new_capacity < 16) new_capacity = 16;
                
                kh_variable_t* new_variables = (kh_variable_t*)realloc(file->variables, 
                                                                      new_capacity * sizeof(kh_variable_t));
                if (!new_variables) {
                    kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                    goto cleanup;
                }
                
                /* Initialize new slots */
                memset(&new_variables[file->capacity], 0, 
                       (new_capacity - file->capacity) * sizeof(kh_variable_t));
                
                file->variables = new_variables;
                file->capacity = new_capacity;
            }
            
            var_index = file->variable_count++;
            
            size_t name_len = strlen(clean_var_name) + 1;
            file->variables[var_index].name = (char*)malloc(name_len);
            if (!file->variables[var_index].name) {
                file->variable_count--; /* Rollback */
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            strcpy_s(file->variables[var_index].name, name_len, clean_var_name);
            
            /* Try to add to hash table without rebuilding */
            if (!kh_hash_table_add_entry(&file->hash_info, clean_var_name, var_index)) {
                /* Hash table needs rebuilding - will be done later if needed */
                file->hash_info.needs_rebuild = 1;
            }
        } else {
            /* Replace existing */
            free(file->variables[var_index].value);
        }
        
        size_t clean_value_len = strlen(clean_value) + 1;
        file->variables[var_index].value = (char*)malloc(clean_value_len);
        if (!file->variables[var_index].value) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(file->variables[var_index].value, clean_value_len, clean_value);
        file->variables[var_index].type = data_type;
    }
    
    /* Only rebuild hash table if necessary */
    if (kh_should_rebuild_hash_table(&file->hash_info, file->variable_count)) {
        if (!kh_rebuild_hash_table(file)) {
            kh_set_error(output, output_size, "FAILED TO REBUILD HASH TABLE");
            goto cleanup;
        }
    }
    
    /* Mark as dirty and save */
    file->dirty = 1;
    if (!kh_save_file_from_memory(file)) {
        kh_set_error(output, output_size, "FAILED TO SAVE FILE");
        goto cleanup;
    }
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_var_name);
    free(clean_value);
    free(clean_filename);
    free(combined_value);
    return result;
}

#endif /* PROCESS_KH_DATA_HPP */