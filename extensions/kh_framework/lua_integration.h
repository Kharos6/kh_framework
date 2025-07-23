#ifndef LUA_INTEGRATION_H
#define LUA_INTEGRATION_H

#include "common_defines.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

/* LuaJIT includes */
#include "luajit/include/lua.h"
#include "luajit/include/lualib.h"
#include "luajit/include/lauxlib.h"

/* Compatibility fixes for different LuaJIT versions */
#ifndef lua_rawlen
#define lua_rawlen(L,i) lua_objlen(L,i)
#endif

#ifndef LUA_OK
#define LUA_OK 0
#endif

/* High-performance FNV-1a hash implementation - consistent with crypto_operations.h */
static inline uint64_t lua_hash_fnv1a(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = KH_FNV1A_64_OFFSET_BASIS;
    
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= KH_FNV1A_64_PRIME;
    }
    
    return hash;
}

/* Hash function for variable names - case insensitive (same as process_kh_data.h) */
static inline uint32_t lua_hash_variable_name(const char* name) {
    if (!name) return LUA_VAR_HASH_EMPTY;
    
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
    return (hash == LUA_VAR_HASH_EMPTY) ? 1 : hash;
}

/* Calculate optimal hash table size */
static inline uint32_t lua_calculate_hash_table_size(uint32_t variable_count) {
    /* Check for potential overflow in load factor calculation */
    if (variable_count > UINT32_MAX / 2) {
        return LUA_VAR_HASH_TABLE_MIN_SIZE; /* Fallback to minimum size */
    }
    
    /* Use next power of 2 that gives load factor <= 0.75 */
    uint32_t min_size = (uint32_t)((double)variable_count / LUA_VAR_HASH_TABLE_LOAD_FACTOR);
    uint32_t size = LUA_VAR_HASH_TABLE_MIN_SIZE;
    
    /* Ensure we don't overflow when doubling */
    while (size < min_size && size <= UINT32_MAX / 2) {
        size <<= 1;
    }
    
    /* Final bounds check */
    if (size < LUA_VAR_HASH_TABLE_MIN_SIZE) {
        size = LUA_VAR_HASH_TABLE_MIN_SIZE;
    }
    
    return size;
}

/* Hash table entry for Lua variables */
typedef struct {
    uint32_t name_hash;      /* Hash of variable name */
    uint32_t var_index;      /* Index into variables array */
    uint32_t reserved;       /* Reserved for future use */
    uint8_t deleted;         /* Tombstone for Robin Hood hashing */
    uint8_t distance;        /* Distance from ideal position (Robin Hood) */
    uint16_t padding;        /* Alignment padding */
} lua_var_hash_entry_t;

/* Hash table info structure */
typedef struct {
    lua_var_hash_entry_t* entries;
    uint32_t size;
    uint32_t used_count;     /* Number of used entries */
    uint32_t deleted_count;  /* Number of deleted entries */
    int needs_rebuild;       /* Flag to indicate if rebuild is needed */
} lua_var_hash_table_t;

/* Enhanced persistent Lua state with better error tracking and initial state management */
typedef struct {
    lua_State* L;                        
    int in_use;                          
    int initialized;                     
    int jit_enabled;                     
    uint64_t* code_hashes;              /* Track executed code for JIT affinity */
    int code_hash_count;
    int code_hash_capacity;
    int execution_count;                /* Track how much code has run in this state */
    DWORD last_used_time;               /* Keep states alive longer for JIT benefit */
    DWORD creation_time;
    int error_count;                     
    DWORD last_error_time;               
    DWORD last_cleanup_time; 
} lua_persistent_state_t;

/* Structure to hold persistent Lua variables accessible across all states */
typedef struct lua_variable_s {
    char* name;
    char* original_value;  /* Original Arma format value */
    char* type;           /* Variable type string */
    int lua_type;         /* Converted Lua type for fast reference */
} lua_variable_t;

/* Hash table-based variable storage */
typedef struct {
    lua_variable_t* variables;    /* Array of variables */
    uint32_t variable_count;      /* Current number of variables */
    uint32_t variable_capacity;   /* Allocated capacity */
    lua_var_hash_table_t hash_table; /* Hash table for fast lookup */
    int initialized;              /* Whether storage is initialized */
} lua_variable_storage_t;

/* Staged call structure for ArmaStageCall functionality */
typedef struct {
    char* arguments;  /* Arma format argument array string */
    char* function;   /* Function name string */
} staged_call_t;

typedef struct {
    staged_call_t* calls;
    int count;
    int capacity;
    int initialized;
} staged_calls_storage_t;

/* Global staged calls storage */
static staged_calls_storage_t g_staged_calls = {0};

/* Global optimization caches - single-threaded, so no locking needed */
static lua_persistent_state_t g_lua_states[LUA_STATE_POOL_SIZE] = {0};
static int g_lua_pool_initialized = 0;

/* Global persistent Lua variables storage with hash table */
static lua_variable_storage_t g_lua_variable_storage = {0};
static int(*g_arma_callback)(char const *name, char const *function, char const *data) = NULL;
static int g_callback_initialized = 0;
static char g_extension_name[64] = "kh_framework"; /* Default extension name for callbacks */

/* Robin Hood hashing with quadratic probing fallback (same as process_kh_data.h) */
static inline uint32_t lua_hash_table_find_robin_hood(lua_var_hash_entry_t* hash_table, uint32_t hash_table_size, 
                                                      uint32_t name_hash, int* found) {
    if (!hash_table || hash_table_size == 0 || name_hash == LUA_VAR_HASH_EMPTY) {
        if (found) *found = 0;
        return 0;
    }
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    
    do {
        lua_var_hash_entry_t* entry = &hash_table[index];
        
        if (entry->name_hash == LUA_VAR_HASH_EMPTY && !entry->deleted) {
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
        
        /* Prevent integer overflow in quadratic probing */
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

/* Robin Hood hash table insertion (same as process_kh_data.h) */
static inline int lua_hash_table_insert_robin_hood(lua_var_hash_entry_t* hash_table, uint32_t hash_table_size,
                                                   uint32_t name_hash, uint32_t var_index) {
    if (!hash_table || hash_table_size == 0 || name_hash == LUA_VAR_HASH_EMPTY) return 0;
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t distance = 0;
    uint32_t original_index = index;
    
    /* Current entry to insert */
    lua_var_hash_entry_t new_entry = {name_hash, var_index, 0, 0, 0, 0};
    
    while (distance < hash_table_size) {
        lua_var_hash_entry_t* entry = &hash_table[index];
        
        /* Empty slot or deleted slot */
        if (entry->name_hash == LUA_VAR_HASH_EMPTY || entry->deleted) {
            *entry = new_entry;
            entry->distance = (uint8_t)distance;
            return 1;
        }
        
        /* Update existing entry */
        if (entry->name_hash == name_hash) {
            entry->var_index = var_index;
            return 1;
        }
        
        /* Robin Hood: if new entry is further from home, swap */
        if (distance > entry->distance) {
            lua_var_hash_entry_t temp = *entry;
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
static inline int lua_hash_table_delete(lua_var_hash_entry_t* hash_table, uint32_t hash_table_size, uint32_t name_hash) {
    if (!hash_table || hash_table_size == 0 || name_hash == LUA_VAR_HASH_EMPTY) return 0;
    
    int found = 0;
    uint32_t index = lua_hash_table_find_robin_hood(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        hash_table[index].deleted = 1;
        hash_table[index].name_hash = LUA_VAR_HASH_EMPTY;
        hash_table[index].var_index = 0;
        return 1;
    }
    
    return 0;
}

/* Initialize variable storage with hash table */
static int lua_init_variable_storage(void) {
    if (g_lua_variable_storage.initialized) return 1;
    
    g_lua_variable_storage.variable_capacity = 16;
    g_lua_variable_storage.variables = (lua_variable_t*)calloc(g_lua_variable_storage.variable_capacity, 
                                                               sizeof(lua_variable_t));
    if (!g_lua_variable_storage.variables) return 0;
    
    g_lua_variable_storage.hash_table.size = LUA_VAR_HASH_TABLE_MIN_SIZE;
    g_lua_variable_storage.hash_table.entries = (lua_var_hash_entry_t*)calloc(g_lua_variable_storage.hash_table.size, 
                                                                              sizeof(lua_var_hash_entry_t));
    if (!g_lua_variable_storage.hash_table.entries) {
        free(g_lua_variable_storage.variables);
        return 0;
    }
    
    g_lua_variable_storage.variable_count = 0;
    g_lua_variable_storage.hash_table.used_count = 0;
    g_lua_variable_storage.hash_table.deleted_count = 0;
    g_lua_variable_storage.hash_table.needs_rebuild = 0;
    g_lua_variable_storage.initialized = 1;
    
    return 1;
}

/* Rebuild hash table when needed */
static int lua_rebuild_variable_hash_table(void) {
    if (!g_lua_variable_storage.initialized) return 0;
    
    /* Calculate new size */
    uint32_t new_size = lua_calculate_hash_table_size(g_lua_variable_storage.variable_count);
    
    /* Allocate new hash table */
    lua_var_hash_entry_t* new_entries = (lua_var_hash_entry_t*)calloc(new_size, sizeof(lua_var_hash_entry_t));
    if (!new_entries) return 0;
    
    /* Free old hash table */
    free(g_lua_variable_storage.hash_table.entries);
    
    /* Set new hash table */
    g_lua_variable_storage.hash_table.entries = new_entries;
    g_lua_variable_storage.hash_table.size = new_size;
    g_lua_variable_storage.hash_table.used_count = 0;
    g_lua_variable_storage.hash_table.deleted_count = 0;
    g_lua_variable_storage.hash_table.needs_rebuild = 0;
    
    /* Rebuild hash table */
    for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
        if (g_lua_variable_storage.variables[i].name) {
            uint32_t name_hash = lua_hash_variable_name(g_lua_variable_storage.variables[i].name);
            if (!lua_hash_table_insert_robin_hood(g_lua_variable_storage.hash_table.entries, 
                                                 g_lua_variable_storage.hash_table.size, name_hash, i)) {
                return 0;
            }
            g_lua_variable_storage.hash_table.used_count++;
        }
    }
    
    return 1;
}

/* Check if hash table needs rebuilding */
static int lua_hash_table_needs_rebuild(void) {
    if (!g_lua_variable_storage.initialized) return 1;
    
    lua_var_hash_table_t* hash_table = &g_lua_variable_storage.hash_table;
    
    /* Rebuild if load factor too high */
    double load_factor = (double)(hash_table->used_count + hash_table->deleted_count) / hash_table->size;
    if (load_factor > 0.8) return 1;
    
    /* Rebuild if too many deletions */
    if (hash_table->deleted_count > hash_table->used_count / 2) return 1;
    
    /* Rebuild if size mismatch */
    uint32_t optimal_size = lua_calculate_hash_table_size(g_lua_variable_storage.variable_count);
    if (hash_table->size < optimal_size / 2 || hash_table->size > optimal_size * 2) return 1;
    
    /* Rebuild if explicitly flagged */
    if (hash_table->needs_rebuild) return 1;
    
    return 0;
}

/* Find a stored Lua variable by name using hash table */
static lua_variable_t* lua_find_variable(const char* name) {
    if (!name || !g_lua_variable_storage.initialized) return NULL;
    
    uint32_t name_hash = lua_hash_variable_name(name);
    int found = 0;
    uint32_t index = lua_hash_table_find_robin_hood(g_lua_variable_storage.hash_table.entries,
                                                   g_lua_variable_storage.hash_table.size, 
                                                   name_hash, &found);
    
    if (found && !g_lua_variable_storage.hash_table.entries[index].deleted) {
        uint32_t var_index = g_lua_variable_storage.hash_table.entries[index].var_index;
        if (var_index < g_lua_variable_storage.variable_count) {
            lua_variable_t* var = &g_lua_variable_storage.variables[var_index];
            if (var->name && strcmp(var->name, name) == 0) {
                return var;
            }
        }
    }
    
    return NULL;
}

/* Enhanced value combination for Lua variables (same logic as kh_combine_values) */
static int lua_combine_variable_values(int var_type, const char* existing_value, const char* new_value, 
                                      int overwrite_flag, char** result_value) {
    if (!existing_value || !new_value || !result_value) return 0;
    
    size_t existing_len = strlen(existing_value);
    size_t new_len = strlen(new_value);
    size_t result_size = existing_len + new_len + 1024; /* Extra space for formatting */
    
    char* clean_existing = (char*)malloc(existing_len + 1);
    char* clean_new = (char*)malloc(new_len + 1);
    char* temp_result = (char*)malloc(result_size);
    
    if (!clean_existing || !clean_new || !temp_result) {
        free(clean_existing);
        free(clean_new);
        free(temp_result);
        return 0;
    }
    
    kh_clean_string(existing_value, clean_existing, (int)existing_len + 1);
    kh_clean_string(new_value, clean_new, (int)new_len + 1);
    
    int success = 0;
    
    switch (var_type) {
        case 4: /* BOOL */
            if (overwrite_flag) {
                strcpy_s(temp_result, result_size, clean_new);
            } else {
                /* Toggle boolean */
                int current_value = kh_parse_boolean(clean_existing);
                strcpy_s(temp_result, result_size, current_value ? "false" : "true");
            }
            success = 1;
            break;
            
        case 3: /* HASHMAP */
            if (overwrite_flag) {
                strcpy_s(temp_result, result_size, clean_new);
                success = 1;
            } else {
                /* For simplicity, use string concatenation for hashmaps in Lua context */
                if (strlen(clean_existing) + strlen(clean_new) + 10 < result_size) {
                    _snprintf_s(temp_result, result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                    success = 1;
                }
            }
            break;
            
        case 2: /* SCALAR */
            if (overwrite_flag) {
                strcpy_s(temp_result, result_size, clean_new);
            } else {
                /* Add numbers together */
                char* endptr1, *endptr2;
                double existing_num = strtod(clean_existing, &endptr1);
                double new_num = strtod(clean_new, &endptr2);
                
                if (endptr1 != clean_existing && *endptr1 == '\0' &&
                    endptr2 != clean_new && *endptr2 == '\0') {
                    _snprintf_s(temp_result, result_size, _TRUNCATE, "%.6g", existing_num + new_num);
                } else {
                    /* String concatenation fallback */
                    if (strlen(clean_existing) + strlen(clean_new) < result_size) {
                        _snprintf_s(temp_result, result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                    }
                }
            }
            success = 1;
            break;
            
        case 0: /* ARRAY */
            if (overwrite_flag) {
                strcpy_s(temp_result, result_size, clean_new);
                success = 1;
            } else {
                /* FIXED: Enhanced array concatenation with proper empty array handling */
                size_t existing_clean_len = strlen(clean_existing);
                size_t new_clean_len = strlen(clean_new);
                
                /* Validate both arrays have minimum structure */
                if (existing_clean_len >= 2 && new_clean_len >= 2 &&
                    clean_existing[0] == '[' && clean_existing[existing_clean_len-1] == ']' &&
                    clean_new[0] == '[' && clean_new[new_clean_len-1] == ']') {
                    
                    /* Check if existing array is empty [] or [ ] */
                    int existing_is_empty = (existing_clean_len == 2) || 
                                          (existing_clean_len == 3 && clean_existing[1] == ' ');
                    
                    /* Check if new array is empty [] or [ ] */
                    int new_is_empty = (new_clean_len == 2) || 
                                     (new_clean_len == 3 && clean_new[1] == ' ');
                    
                    if (existing_is_empty && new_is_empty) {
                        /* Both empty, result is empty array */
                        strcpy_s(temp_result, result_size, "[]");
                        success = 1;
                    } else if (existing_is_empty) {
                        /* Existing is empty, use new array */
                        strcpy_s(temp_result, result_size, clean_new);
                        success = 1;
                    } else if (new_is_empty) {
                        /* New is empty, use existing array */
                        strcpy_s(temp_result, result_size, clean_existing);
                        success = 1;
                    } else {
                        /* Both have content, concatenate properly */
                        if (existing_clean_len + new_clean_len < result_size) {
                            _snprintf_s(temp_result, result_size, _TRUNCATE, "%.*s,%s", 
                                       (int)(existing_clean_len - 1), clean_existing, 
                                       clean_new + 1);
                            success = 1;
                        }
                    }
                } else {
                    /* Invalid array format, fallback to string concatenation */
                    if (strlen(clean_existing) + strlen(clean_new) < result_size) {
                        _snprintf_s(temp_result, result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                        success = 1;
                    }
                }
            }
            break;
            
        default: /* STRING, TEXT, CODE */
            if (overwrite_flag) {
                strcpy_s(temp_result, result_size, clean_new);
            } else {
                /* String concatenation */
                if (strlen(clean_existing) + strlen(clean_new) < result_size) {
                    _snprintf_s(temp_result, result_size, _TRUNCATE, "%s%s", clean_existing, clean_new);
                }
            }
            success = 1;
            break;
    }
    
    if (success) {
        size_t final_len = strlen(temp_result);
        *result_value = (char*)malloc(final_len + 1);
        if (*result_value) {
            strcpy_s(*result_value, final_len + 1, temp_result);
        } else {
            success = 0;
        }
    }
    
    free(clean_existing);
    free(clean_new);
    free(temp_result);
    
    return success;
}

/* FIXED: Set a persistent Lua variable with proper memory leak prevention, hash table and overwrite support */
static int lua_set_variable(const char* name, const char* value, const char* type, int overwrite_flag) {
    if (!name || !value || !type) return 0;
    
    /* Initialize storage if needed */
    if (!lua_init_variable_storage()) return 0;
    
    /* Validate variable name */
    if (!kh_validate_variable_name(name)) return 0;
    
    /* Validate type */
    int var_type = -1;
    if (strcmp(type, "ARRAY") == 0) var_type = 0;
    else if (strcmp(type, "STRING") == 0) var_type = 1;
    else if (strcmp(type, "SCALAR") == 0) var_type = 2;
    else if (strcmp(type, "HASHMAP") == 0) var_type = 3;
    else if (strcmp(type, "BOOL") == 0) var_type = 4;
    else return 0; /* Invalid type */
    
    /* Find existing variable */
    lua_variable_t* existing_var = lua_find_variable(name);
    if (existing_var) {
        /* Handle overwrite vs combine */
        char* final_value = NULL;
        
        if (overwrite_flag) {
            /* Simple overwrite */
            size_t value_len = strlen(value);
            final_value = (char*)malloc(value_len + 1);
            if (final_value) {
                strcpy_s(final_value, value_len + 1, value);
            }
        } else {
            /* Type must match for combining */
            if (existing_var->lua_type != var_type) {
                return 0; /* Type mismatch */
            }
            
            /* Combine values */
            if (!lua_combine_variable_values(var_type, existing_var->original_value, value, 
                                           overwrite_flag, &final_value)) {
                return 0;
            }
        }
        
        if (!final_value) return 0;
        
        /* Update type if overwriting */
        char* new_type = NULL;
        if (overwrite_flag) {
            size_t type_len = strlen(type);
            new_type = (char*)malloc(type_len + 1);
            if (!new_type) {
                free(final_value);
                return 0;
            }
            strcpy_s(new_type, type_len + 1, type);
        }
        
        /* Replace old values */
        free(existing_var->original_value);
        existing_var->original_value = final_value;
        
        if (overwrite_flag) {
            free(existing_var->type);
            existing_var->type = new_type;
            existing_var->lua_type = var_type;
        }
        
        return 1;
    }
    
    /* Add new variable */
    
    /* Expand variables array if needed */
    if (g_lua_variable_storage.variable_count >= g_lua_variable_storage.variable_capacity) {
        uint32_t new_capacity = g_lua_variable_storage.variable_capacity * 2;
        lua_variable_t* new_variables = (lua_variable_t*)realloc(g_lua_variable_storage.variables,
                                                                new_capacity * sizeof(lua_variable_t));
        if (!new_variables) return 0;
        
        /* Initialize new memory */
        memset(&new_variables[g_lua_variable_storage.variable_capacity], 0,
               (new_capacity - g_lua_variable_storage.variable_capacity) * sizeof(lua_variable_t));
        
        g_lua_variable_storage.variables = new_variables;
        g_lua_variable_storage.variable_capacity = new_capacity;
    }
    
    /* FIXED: Allocate all memory first with comprehensive error handling */
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t type_len = strlen(type);
    
    char* name_copy = (char*)malloc(name_len + 1);
    char* value_copy = (char*)malloc(value_len + 1);
    char* type_copy = (char*)malloc(type_len + 1);
    
    /* Check all allocations before proceeding */
    if (!name_copy || !value_copy || !type_copy) {
        /* Free any successful allocations */
        free(name_copy);
        free(value_copy);
        free(type_copy);
        return 0;
    }
    
    /* Copy strings */
    strcpy_s(name_copy, name_len + 1, name);
    strcpy_s(value_copy, value_len + 1, value);
    strcpy_s(type_copy, type_len + 1, type);
    
    /* Rebuild hash table if needed BEFORE adding the variable */
    if (lua_hash_table_needs_rebuild()) {
        if (!lua_rebuild_variable_hash_table()) {
            /* Free allocated memory on hash table failure */
            free(name_copy);
            free(value_copy);
            free(type_copy);
            return 0;
        }
    }
    
    /* Add to variables array */
    uint32_t var_index = g_lua_variable_storage.variable_count;
    
    /* FIXED: Add to hash table BEFORE updating the array to maintain consistency */
    uint32_t name_hash = lua_hash_variable_name(name);
    if (!lua_hash_table_insert_robin_hood(g_lua_variable_storage.hash_table.entries,
                                         g_lua_variable_storage.hash_table.size, 
                                         name_hash, var_index)) {
        /* Hash table insertion failed - free allocated memory */
        free(name_copy);
        free(value_copy);
        free(type_copy);
        return 0;
    }
    
    /* Now it's safe to update the array since hash table succeeded */
    g_lua_variable_storage.variables[var_index].name = name_copy;
    g_lua_variable_storage.variables[var_index].original_value = value_copy;
    g_lua_variable_storage.variables[var_index].type = type_copy;
    g_lua_variable_storage.variables[var_index].lua_type = var_type;
    g_lua_variable_storage.variable_count++;
    g_lua_variable_storage.hash_table.used_count++;
    
    return 1;
}

/* Forward declarations */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth);
static void lua_release_optimized_state(lua_persistent_state_t* state);
static lua_persistent_state_t* lua_get_optimized_state_with_affinity(uint64_t code_hash);
static int lua_format_result_optimized(lua_State* L, char* output, int output_size);
static int lua_c_arma_call(lua_State* L);
static int arma_callback_safe_call(const char* data, const char* function, char* error_output, int error_size);
static int arma_convert_lua_data_to_string(lua_State* L, int stack_index, char* output, int output_size);

static void lua_set_arma_callback(int(*callback)(char const*, char const*, char const*)) {
    g_arma_callback = callback;
    g_callback_initialized = (callback != NULL) ? 1 : 0;
}

/* Delete a persistent Lua variable using hash table */
static int lua_delete_variable(const char* name) {
    if (!name || !g_lua_variable_storage.initialized) return 0;
    
    uint32_t name_hash = lua_hash_variable_name(name);
    int found = 0;
    uint32_t hash_index = lua_hash_table_find_robin_hood(g_lua_variable_storage.hash_table.entries,
                                                        g_lua_variable_storage.hash_table.size, 
                                                        name_hash, &found);
    
    if (!found || g_lua_variable_storage.hash_table.entries[hash_index].deleted) {
        return 0; /* Not found */
    }
    
    uint32_t var_index = g_lua_variable_storage.hash_table.entries[hash_index].var_index;
    if (var_index >= g_lua_variable_storage.variable_count) {
        return 0; /* Invalid index */
    }
    
    lua_variable_t* var = &g_lua_variable_storage.variables[var_index];
    if (!var->name || strcmp(var->name, name) != 0) {
        return 0; /* Name mismatch */
    }
    
    /* Free memory */
    free(var->name);
    free(var->original_value);
    free(var->type);
    
    /* Remove from hash table first */
    lua_hash_table_delete(g_lua_variable_storage.hash_table.entries,
                         g_lua_variable_storage.hash_table.size, name_hash);
    g_lua_variable_storage.hash_table.used_count--;
    g_lua_variable_storage.hash_table.deleted_count++;
    
    /* FIXED: Use swap-with-last instead of shifting to avoid O(n) operations */
    uint32_t last_index = g_lua_variable_storage.variable_count - 1;
    
    if (var_index != last_index) {
        /* Move last element to deleted position */
        g_lua_variable_storage.variables[var_index] = g_lua_variable_storage.variables[last_index];
        
        /* Update hash table entry for the moved variable */
        if (g_lua_variable_storage.variables[var_index].name) {
            uint32_t moved_name_hash = lua_hash_variable_name(g_lua_variable_storage.variables[var_index].name);
            int moved_found = 0;
            uint32_t moved_hash_index = lua_hash_table_find_robin_hood(g_lua_variable_storage.hash_table.entries,
                                                                      g_lua_variable_storage.hash_table.size, 
                                                                      moved_name_hash, &moved_found);
            if (moved_found && !g_lua_variable_storage.hash_table.entries[moved_hash_index].deleted) {
                g_lua_variable_storage.hash_table.entries[moved_hash_index].var_index = var_index;
            }
        }
    }
    
    /* Clear last element */
    memset(&g_lua_variable_storage.variables[last_index], 0, sizeof(lua_variable_t));
    g_lua_variable_storage.variable_count--;
    
    /* Check if rebuild needed */
    if (lua_hash_table_needs_rebuild()) {
        g_lua_variable_storage.hash_table.needs_rebuild = 1;
    }
    
    return 1;
}

/* Convert and inject a variable into a Lua state */
static int lua_inject_variable(lua_State* L, lua_variable_t* var) {
    if (!L || !var || !var->name || !var->original_value) return 0;
    
    if (!lua_checkstack(L, 5)) return 0;
    
    /* Track initial stack position for cleanup */
    int initial_stack_top = lua_gettop(L);
    
    switch (var->lua_type) {
        case 0: /* ARRAY */
        case 3: /* HASHMAP */
            {
                /* Parse as array/hashmap using existing parser */
                const char* end = var->original_value + strlen(var->original_value);
                const char* result = lua_parse_value_recursive(L, var->original_value, end, 0);
                if (result) {
                    lua_setglobal(L, var->name);
                    return 1;
                } else {
                    /* Ensure stack is restored on parsing failure */
                    lua_settop(L, initial_stack_top);
                    return 0;
                }
            }
            break;
            
        case 1: /* STRING */
            {
                /* Clean the string value */
                size_t val_len = strlen(var->original_value);
                char* clean_value = (char*)malloc(val_len + 1);
                if (clean_value) {
                    kh_clean_string(var->original_value, clean_value, (int)val_len + 1);
                    lua_pushstring(L, clean_value);
                    lua_setglobal(L, var->name);
                    free(clean_value);
                    return 1;
                } else {
                    return 0;
                }
            }
            break;
            
        case 2: /* SCALAR */
            {
                char* endptr;
                double num = strtod(var->original_value, &endptr);
                if (*endptr == '\0' && !isnan(num) && !isinf(num)) {
                    lua_pushnumber(L, num);
                    lua_setglobal(L, var->name);
                    return 1;
                } else {
                    return 0;
                }
            }
            break;
            
        case 4: /* BOOL */
            {
                int bool_val = kh_parse_boolean(var->original_value);
                lua_pushboolean(L, bool_val);
                lua_setglobal(L, var->name);
                return 1;
            }
            break;
    }
    
    return 0;
}

/* FIXED: Get variable value and type in Arma format [type, value] */
static int lua_get_variable_value_and_type(const char* name, char* output, int output_size) {
    if (!name || !output || output_size <= 0) return 0;
    
    lua_variable_t* var = lua_find_variable(name);
    if (!var) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        return 0;
    }
    
    /* Get a Lua state to retrieve the current value */
    lua_persistent_state_t* state = lua_get_optimized_state_with_affinity(0);
    if (!state) {
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 0;
    }
    
    lua_State* L = state->L;
    int result = 0;
    int initial_stack_top = lua_gettop(L);
    
    /* Get the variable from Lua */
    lua_getglobal(L, name);
    
    char* temp_buffer = (char*)malloc(output_size);
    if (!temp_buffer) {
        lua_settop(L, initial_stack_top);
        lua_release_optimized_state(state);
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 0;
    }
    
    if (!lua_isnil(L, -1)) {
        /* Format the current value from Lua state */
        if (lua_format_result_optimized(L, temp_buffer, output_size - 20)) { /* Reserve space for type */
            /* Format as [type, value] */
            if (_snprintf_s(output, output_size, _TRUNCATE, "[\"%s\",%s]", 
                          var->type, temp_buffer) >= 0) {
                result = 1;
            } else {
                kh_set_error(output, output_size, "OUTPUT BUFFER TOO SMALL");
            }
        } else {
            kh_set_error(output, output_size, "RESULT FORMAT FAILED");
        }
    } else {
        /* Variable not found in Lua state, return original value with type */
        if (_snprintf_s(output, output_size, _TRUNCATE, "[\"%s\",\"%s\"]", 
                      var->type, var->original_value) >= 0) {
            result = 1;
        } else {
            kh_set_error(output, output_size, "OUTPUT BUFFER TOO SMALL");
        }
    }
    
    /* Clean up */
    free(temp_buffer);
    lua_settop(L, initial_stack_top);
    lua_release_optimized_state(state);
    
    return result;
}

/* Get variable value for Lua-callable function (returns just the value, not type) */
static int lua_get_variable_value_for_lua(const char* name, lua_State* L) {
    if (!name || !L) return 0;
    
    lua_variable_t* var = lua_find_variable(name);
    if (!var) {
        lua_pushnil(L);
        return 1;
    }
    
    /* Get the current value from the calling Lua state */
    lua_getglobal(L, name);
    
    /* If not found in current state, inject the stored value */
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1); /* Remove nil */
        if (!lua_inject_variable(L, var)) {
            lua_pushnil(L);
        } else {
            lua_getglobal(L, name);
        }
    }
    
    return 1;
}

/* Lua-callable function: GetPersistentVariable(name) - returns just the value */
static int lua_c_get_persistent_variable(lua_State* L) {
    /* Check argument count */
    int argc = lua_gettop(L);
    if (argc != 1) {
        lua_pushnil(L);
        return 1;
    }
    
    /* Get argument */
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushnil(L);
        return 1;
    }
    
    /* Clean the name */
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        lua_pushnil(L);
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Validate and get */
    int success = 0;
    if (strlen(clean_name) > 0 && kh_validate_variable_name(clean_name)) {
        success = lua_get_variable_value_for_lua(clean_name, L);
    }
    
    free(clean_name);
    
    if (!success) {
        lua_pushnil(L);
    }
    
    return 1;
}

/* Lua-callable function: SetPersistentVariable(name, value, type, overwrite) */
static int lua_c_set_persistent_variable(lua_State* L) {
    if (!lua_checkstack(L, 1)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Check argument count */
    int argc = lua_gettop(L);
    if (argc < 3 || argc > 4) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Get arguments */
    const char* name = lua_tostring(L, 1);
    const char* value = lua_tostring(L, 2);
    const char* type = lua_tostring(L, 3);
    
    /* Get overwrite flag (defaults to true) */
    int overwrite_flag = 1; /* Default to true */
    if (argc >= 4 && !lua_isnil(L, 4)) {
        if (lua_isboolean(L, 4)) {
            overwrite_flag = lua_toboolean(L, 4);
        } else if (lua_isstring(L, 4)) {
            const char* overwrite_str = lua_tostring(L, 4);
            if (overwrite_str) {
                overwrite_flag = kh_parse_boolean(overwrite_str);
            }
        }
    }
    
    if (!name || !value || !type) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Validate and clean inputs */
    size_t name_len = strlen(name);
    size_t type_len = strlen(type);
    
    char* clean_name = (char*)malloc(name_len + 1);
    char* clean_type = (char*)malloc(type_len + 1);
    
    if (!clean_name || !clean_type) {
        free(clean_name);
        free(clean_type);
        lua_pushboolean(L, 0);
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    kh_clean_string(type, clean_type, (int)type_len + 1);
    
    /* Convert type to uppercase for consistency */
    for (size_t i = 0; i < strlen(clean_type); i++) {
        if (clean_type[i] >= 'a' && clean_type[i] <= 'z') {
            clean_type[i] = clean_type[i] - 32;
        }
    }
    
    /* Validate inputs */
    int success = 0;
    if (strlen(clean_name) > 0 && kh_validate_variable_name(clean_name)) {
        /* Validate type */
        int type_code = -1;
        if (strcmp(clean_type, "ARRAY") == 0) type_code = 0;
        else if (strcmp(clean_type, "STRING") == 0) type_code = 1;
        else if (strcmp(clean_type, "SCALAR") == 0) type_code = 2;
        else if (strcmp(clean_type, "HASHMAP") == 0) type_code = 3;
        else if (strcmp(clean_type, "BOOL") == 0) type_code = 4;
        
        if (type_code >= 0 && kh_validate_value_format(type_code, value)) {
            /* Set the variable */
            success = lua_set_variable(clean_name, value, clean_type, overwrite_flag);
        }
    }
    
    free(clean_name);
    free(clean_type);
    
    lua_pushboolean(L, success);
    return 1;
}

/* Lua-callable function: DeletePersistentVariable(name) */
static int lua_c_delete_persistent_variable(lua_State* L) {
    if (!lua_checkstack(L, 1)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Check argument count */
    int argc = lua_gettop(L);
    if (argc != 1) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Get argument */
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Clean the name */
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Validate and delete */
    int success = 0;
    if (strlen(clean_name) > 0 && kh_validate_variable_name(clean_name)) {
        success = lua_delete_variable(clean_name);
    }
    
    free(clean_name);
    
    lua_pushboolean(L, success);
    return 1;
}

/* NEW: LuaClearVariables operation - Clear all persistent Lua variables */
static int kh_process_lua_clear_variables_operation(char* output, int output_size, 
                                                   const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc != 0) {
        kh_set_error(output, output_size, "REQUIRES 0 ARGUMENTS");
        return 1;
    }
    
    uint32_t cleared_count = 0;
    size_t memory_freed = 0;
    
    /* FIXED: Clear all persistent variables with proper validation */
    if (g_lua_variable_storage.initialized && g_lua_variable_storage.variables) {
        cleared_count = g_lua_variable_storage.variable_count;
        
        /* Free all variable data and calculate freed memory */
        for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
            lua_variable_t* var = &g_lua_variable_storage.variables[i];
            if (var->name) {
                memory_freed += strlen(var->name) + 1;
                free(var->name);
                var->name = NULL;
            }
            if (var->original_value) {
                memory_freed += strlen(var->original_value) + 1;
                free(var->original_value);
                var->original_value = NULL;
            }
            if (var->type) {
                memory_freed += strlen(var->type) + 1;
                free(var->type);
                var->type = NULL;
            }
            var->lua_type = 0;
        }
        
        /* Reset counters */
        g_lua_variable_storage.variable_count = 0;
        
        /* FIXED: Clear and reset hash table safely */
        if (g_lua_variable_storage.hash_table.entries && g_lua_variable_storage.hash_table.size > 0) {
            /* Zero out all hash table entries */
            for (uint32_t i = 0; i < g_lua_variable_storage.hash_table.size; i++) {
                g_lua_variable_storage.hash_table.entries[i].name_hash = LUA_VAR_HASH_EMPTY;
                g_lua_variable_storage.hash_table.entries[i].var_index = 0;
                g_lua_variable_storage.hash_table.entries[i].deleted = 0;
                g_lua_variable_storage.hash_table.entries[i].distance = 0;
                g_lua_variable_storage.hash_table.entries[i].reserved = 0;
                g_lua_variable_storage.hash_table.entries[i].padding = 0;
            }
        }
        g_lua_variable_storage.hash_table.used_count = 0;
        g_lua_variable_storage.hash_table.deleted_count = 0;
        g_lua_variable_storage.hash_table.needs_rebuild = 0;
    }
    
    _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\",%u,%zu]", cleared_count, memory_freed);
    return 0;
}

/* FIXED: Enhanced LuaSetVariable operation - Set persistent Lua variable with overwrite support */
static int kh_process_lua_set_variable_operation(char* output, int output_size, 
                                                const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 3 || argc > 4 || !argv || !argv[0] || !argv[1] || !argv[2]) {
        kh_set_error(output, output_size, "REQUIRES 3-4 ARGUMENTS: [NAME, VALUE, TYPE, OVERWRITE]");
        return 1;
    }
    
    const char* name = argv[0];
    const char* value = argv[1];
    const char* type = argv[2];
    
    /* Parse overwrite flag (defaults to true) */
    int overwrite_flag = 1; /* Default to true */
    if (argc >= 4 && argv[3]) {
        overwrite_flag = kh_parse_boolean(argv[3]);
    }
    
    /* Clean the name */
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Clean the type */
    size_t type_len = strlen(type);
    char* clean_type = (char*)malloc(type_len + 1);
    if (!clean_type) {
        free(clean_name);
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(type, clean_type, (int)type_len + 1);
    
    /* Convert type to uppercase for consistency */
    for (size_t i = 0; i < strlen(clean_type); i++) {
        if (clean_type[i] >= 'a' && clean_type[i] <= 'z') {
            clean_type[i] = clean_type[i] - 32;
        }
    }
    
    /* Validate inputs */
    if (strlen(clean_name) == 0) {
        free(clean_name);
        free(clean_type);
        kh_set_error(output, output_size, "VARIABLE NAME CANNOT BE EMPTY");
        return 1;
    }
    
    if (!kh_validate_variable_name(clean_name)) {
        free(clean_name);
        free(clean_type);
        kh_set_error(output, output_size, "INVALID VARIABLE NAME");
        return 1;
    }
    
    /* Validate type and value format */
    int type_code = -1;
    if (strcmp(clean_type, "ARRAY") == 0) type_code = 0;
    else if (strcmp(clean_type, "STRING") == 0) type_code = 1;
    else if (strcmp(clean_type, "SCALAR") == 0) type_code = 2;
    else if (strcmp(clean_type, "HASHMAP") == 0) type_code = 3;
    else if (strcmp(clean_type, "BOOL") == 0) type_code = 4;
    
    if (type_code == -1) {
        free(clean_name);
        free(clean_type);
        kh_set_error(output, output_size, "INVALID TYPE: MUST BE ARRAY, STRING, SCALAR, HASHMAP, OR BOOL");
        return 1;
    }
    
    /* Validate value format matches type */
    if (!kh_validate_value_format(type_code, value)) {
        free(clean_name);
        free(clean_type);
        kh_set_error(output, output_size, "VALUE FORMAT DOES NOT MATCH SPECIFIED TYPE");
        return 1;
    }
    
    /* Set the variable */
    if (lua_set_variable(clean_name, value, clean_type, overwrite_flag)) {
        _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\"]");
        free(clean_name);
        free(clean_type);
        return 0;
    } else {
        free(clean_name);
        free(clean_type);
        kh_set_error(output, output_size, "FAILED TO SET VARIABLE");
        return 1;
    }
}

/* FIXED: Enhanced LuaGetVariable operation - Get persistent Lua variable with type */
static int kh_process_lua_get_variable_operation(char* output, int output_size, 
                                                const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 1 || !argv || !argv[0]) {
        kh_set_error(output, output_size, "REQUIRES 1 ARGUMENT: [NAME]");
        return 1;
    }
    
    const char* name = argv[0];
    
    /* Clean the name */
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Validate name */
    if (strlen(clean_name) == 0) {
        free(clean_name);
        kh_set_error(output, output_size, "VARIABLE NAME CANNOT BE EMPTY");
        return 1;
    }
    
    if (!kh_validate_variable_name(clean_name)) {
        free(clean_name);
        kh_set_error(output, output_size, "INVALID VARIABLE NAME");
        return 1;
    }
    
    /* Get the variable value and type */
    int result = lua_get_variable_value_and_type(clean_name, output, output_size);
    
    free(clean_name);
    return result ? 0 : 1;
}

/* Enhanced LuaDeleteVariable operation - Delete persistent Lua variable */
static int kh_process_lua_delete_variable_operation(char* output, int output_size, 
                                                   const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 1 || !argv || !argv[0]) {
        kh_set_error(output, output_size, "REQUIRES 1 ARGUMENT: [NAME]");
        return 1;
    }
    
    const char* name = argv[0];
    
    /* Clean the name */
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Validate name */
    if (strlen(clean_name) == 0) {
        free(clean_name);
        kh_set_error(output, output_size, "VARIABLE NAME CANNOT BE EMPTY");
        return 1;
    }
    
    if (!kh_validate_variable_name(clean_name)) {
        free(clean_name);
        kh_set_error(output, output_size, "INVALID VARIABLE NAME");
        return 1;
    }
    
    /* Delete the variable */
    if (lua_delete_variable(clean_name)) {
        _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\"]");
        free(clean_name);
        return 0;
    } else {
        free(clean_name);
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        return 1;
    }
}

/* Enhanced print function for security */
static int lua_secure_print_fast(lua_State* L) {
    /* Consume all arguments but produce no output for security */
    int n = lua_gettop(L);
    lua_pop(L, n);
    return 0;
}

/* Called when Lua tries to access a non-existent global variable */
static int lua_lazy_variable_handler(lua_State* L) {
    if (!lua_checkstack(L, 2)) {
        lua_pushnil(L);
        return 1;
    }
    
    /* Get the variable name being requested */
    const char* var_name = lua_tostring(L, 2);
    if (!var_name) {
        lua_pushnil(L);
        return 1;
    }
    
    /* Check if this variable exists in persistent storage */
    lua_variable_t* var = lua_find_variable(var_name);
    if (var) {
        /* Variable exists - inject it and return its value */
        if (lua_inject_variable(L, var)) {
            /* Get the newly injected variable */
            lua_getglobal(L, var_name);
            return 1;
        }
    }
    
    /* Variable doesn't exist or injection failed */
    lua_pushnil(L);
    return 1;
}

/* Called when Lua assigns to a global variable */
static int lua_variable_change_handler(lua_State* L) {
    if (!lua_checkstack(L, 3)) return 0;
    
    /* Get the variable name being assigned */
    const char* var_name = lua_tostring(L, 2);
    
    /* Perform the actual assignment first */
    lua_rawset(L, 1);    
    return 0;
}

/* NEW: Setup lazy variable injection instead of eager injection */
static int lua_setup_lazy_variable_injection(lua_State* L) {
    if (!L || !lua_checkstack(L, 5)) return 0;
    
    /* Get the global table */
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    
    /* Create metatable for globals */
    lua_newtable(L);
    
    /* Set __index metamethod */
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, lua_lazy_variable_handler);
    lua_settable(L, -3);
    
    /* Set __newindex metamethod to track variable changes */
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lua_variable_change_handler);
    lua_settable(L, -3);
    
    /* Apply metatable to globals */
    lua_setmetatable(L, -2);
    lua_pop(L, 1); /* Remove globals table */
    
    return 1;
}

/* Initialize staged calls storage */
static int lua_init_staged_calls_storage(void) {
    if (g_staged_calls.initialized) return 1;
    
    g_staged_calls.capacity = 16;
    g_staged_calls.calls = (staged_call_t*)calloc(g_staged_calls.capacity, sizeof(staged_call_t));
    if (!g_staged_calls.calls) return 0;
    
    g_staged_calls.count = 0;
    g_staged_calls.initialized = 1;
    return 1;
}

/* Add a staged call with comprehensive error handling */
static int lua_add_staged_call(const char* arguments, const char* function) {
    if (!arguments || !function) return 0;
    
    /* Validate input lengths to prevent excessive memory usage */
    size_t args_len = strlen(arguments);
    size_t func_len = strlen(function);
    
    if (args_len == 0 || func_len == 0 || args_len > 32767 || func_len > 1023) {
        return 0; /* Invalid input lengths */
    }
    
    if (!lua_init_staged_calls_storage()) return 0;
    
    /* Prevent excessive staging (DoS protection) */
    if (g_staged_calls.count >= 1000) return 0;
    
    /* Expand capacity if needed */
    if (g_staged_calls.count >= g_staged_calls.capacity) {
        /* Check for potential overflow */
        if (g_staged_calls.capacity > INT_MAX / 2) return 0;
        
        int new_capacity = g_staged_calls.capacity * 2;
        staged_call_t* new_calls = (staged_call_t*)realloc(g_staged_calls.calls, 
                                                           new_capacity * sizeof(staged_call_t));
        if (!new_calls) return 0;
        
        /* Initialize new memory */
        memset(&new_calls[g_staged_calls.capacity], 0, 
               (new_capacity - g_staged_calls.capacity) * sizeof(staged_call_t));
        
        g_staged_calls.calls = new_calls;
        g_staged_calls.capacity = new_capacity;
    }
    
    /* Allocate and copy strings - check both allocations before proceeding */
    char* args_copy = (char*)malloc(args_len + 1);
    char* func_copy = (char*)malloc(func_len + 1);
    
    if (!args_copy || !func_copy) {
        free(args_copy);
        free(func_copy);
        return 0;
    }
    
    strcpy_s(args_copy, args_len + 1, arguments);
    strcpy_s(func_copy, func_len + 1, function);
    
    /* Store the staged call */
    g_staged_calls.calls[g_staged_calls.count].arguments = args_copy;
    g_staged_calls.calls[g_staged_calls.count].function = func_copy;
    g_staged_calls.count++;
    
    return 1;
}

/* Clear all staged calls and free memory */
static void lua_clear_staged_calls(void) {
    if (!g_staged_calls.calls) return;
    
    for (int i = 0; i < g_staged_calls.count; i++) {
        free(g_staged_calls.calls[i].arguments);
        free(g_staged_calls.calls[i].function);
        g_staged_calls.calls[i].arguments = NULL;
        g_staged_calls.calls[i].function = NULL;
    }
    
    g_staged_calls.count = 0;
}

/* Clean up staged calls storage completely */
static void lua_cleanup_staged_calls_storage(void) {
    lua_clear_staged_calls();
    
    if (g_staged_calls.calls) {
        free(g_staged_calls.calls);
        g_staged_calls.calls = NULL;
    }
    
    g_staged_calls.capacity = 0;
    g_staged_calls.initialized = 0;
}

/* Lua C function: ArmaStageCall(call_data) - Stage a call for later execution */
static int lua_c_arma_stage_call(lua_State* L) {
    /* Ensure adequate stack space */
    if (!lua_checkstack(L, 6)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "STACK OVERFLOW");
        return 2;
    }
    
    /* Validate argument count */
    int argc = lua_gettop(L);
    if (argc != 1) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "REQUIRES 1 ARGUMENT: {arguments, function}");
        return 2;
    }
    
    /* Validate argument is a table */
    if (!lua_istable(L, 1)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "ARGUMENT MUST BE AN ARRAY");
        return 2;
    }
    
    /* Track initial stack for cleanup */
    int initial_stack = lua_gettop(L);
    
    /* Check that table has exactly 2 elements */
    lua_rawgeti(L, 1, 1); /* arguments */
    lua_rawgeti(L, 1, 2); /* function */
    lua_rawgeti(L, 1, 3); /* should be nil */
    
    /* Validate structure: must have exactly 2 elements */
    if (lua_isnil(L, -3) || lua_isnil(L, -2) || !lua_isnil(L, -1)) {
        lua_settop(L, initial_stack);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "ARRAY MUST HAVE EXACTLY 2 ELEMENTS: {arguments, function}");
        return 2;
    }
    
    lua_pop(L, 1); /* Remove the nil check result */
    
    /* Function must be string */
    if (!lua_isstring(L, -1)) {
        lua_settop(L, initial_stack);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION PARAMETER MUST BE A STRING");
        return 2;
    }
    
    const char* function = lua_tostring(L, -1);
    if (!function || strlen(function) == 0) {
        lua_settop(L, initial_stack);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION PARAMETER CANNOT BE EMPTY");
        return 2;
    }
    
    /* Clean the function string - use standard allocation */
    size_t function_len = strlen(function);
    char* clean_function = (char*)malloc(function_len + 1);
    
    if (!clean_function) {
        lua_settop(L, initial_stack);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "MEMORY ALLOCATION FAILED FOR FUNCTION STRING");
        return 2;
    }
    
    kh_clean_string(function, clean_function, (int)function_len + 1);
    
    /* Validate cleaned function */
    if (strlen(clean_function) == 0) {
        lua_settop(L, initial_stack);
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION PARAMETER CANNOT BE EMPTY AFTER CLEANING");
        return 2;
    }
    
    /* Convert arguments to Arma string format - use standard allocation */
    char* args_buffer = (char*)malloc(KH_MAX_OUTPUT_SIZE);
    
    if (!args_buffer) {
        lua_settop(L, initial_stack);
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "MEMORY ALLOCATION FAILED FOR ARGUMENTS BUFFER");
        return 2;
    }
    
    /* Convert arguments (at stack position -2) to string format */
    if (!arma_convert_lua_data_to_string(L, -2, args_buffer, KH_MAX_OUTPUT_SIZE)) {
        lua_settop(L, initial_stack);
        free(clean_function);
        free(args_buffer);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FAILED TO CONVERT ARGUMENTS TO ARMA FORMAT");
        return 2;
    }
    
    /* Validate converted arguments */
    if (strlen(args_buffer) == 0) {
        lua_settop(L, initial_stack);
        free(clean_function);
        free(args_buffer);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "CONVERTED ARGUMENTS CANNOT BE EMPTY");
        return 2;
    }
    
    /* Add to staged calls */
    if (!lua_add_staged_call(args_buffer, clean_function)) {
        lua_settop(L, initial_stack);
        free(clean_function);
        free(args_buffer);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FAILED TO STAGE CALL - MEMORY EXHAUSTION OR TOO MANY STAGED CALLS");
        return 2;
    }
    
    /* Successful staging - restore stack and cleanup */
    lua_settop(L, initial_stack);
    free(clean_function);
    free(args_buffer);
    
    /* Return success without values to stack (ArmaStageCall returns nothing) */
    return 0;
}

/* Enhanced Lua state initialization with Lua-callable functions registration */
static int lua_init_turbo_state(lua_State* L) {
    if (!L) return 0;
    
    /* Ensure we have enough stack space */
    if (!lua_checkstack(L, LUA_STACK_SAFETY_MARGIN + 20)) return 0;
    
    /* Load essential libraries */
    luaopen_base(L);
    luaopen_math(L);     
    luaopen_string(L);   
    luaopen_table(L);
    luaopen_jit(L);
    
    /* Configure LuaJIT for maximum performance and stability */
    lua_getglobal(L, "jit");
    if (!lua_isnil(L, -1)) {
        /* Enable JIT compilation */
        lua_getfield(L, -1, "on");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                lua_pop(L, 2);
                return 0;
            }
        } else {
            lua_pop(L, 1);
        }
        
        /* Aggressive JIT optimization for repeated code execution */
        lua_getfield(L, -1, "opt");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "start");
            /* Very aggressive compilation thresholds for repeated execution */
            lua_pushstring(L, "hotloop=10");       /* Compile loops after 10 iterations */
            lua_pushstring(L, "hotexit=5");        /* Compile side exits after 5 */
            lua_pushstring(L, "tryside=2");        /* Try side trace recording after 2 */
            lua_pushstring(L, "instunroll=4");     /* Instruction unrolling */
            lua_pushstring(L, "loopunroll=20");    /* Aggressive loop unrolling */
            lua_pushstring(L, "recunroll=4");      /* Recursive unrolling */
            lua_pushstring(L, "callunroll=5");     /* Call unrolling */
            
            /* Increase memory limits for complex traces */
            lua_pushstring(L, "maxmcode=8192");    /* More machine code memory */
            lua_pushstring(L, "maxtrace=6000");    /* More traces */
            lua_pushstring(L, "maxrecord=12000");   /* Longer traces */
            lua_pushstring(L, "maxirconst=3000");  /* More constants */
            lua_pushstring(L, "maxside=300");      /* More side exits */
            lua_pushstring(L, "maxsnap=1500");     /* More snapshots */
            
            if (lua_pcall(L, 13, 0, 0) != LUA_OK) {
                lua_pop(L, 2);
                return 0;
            }
        } else {
            lua_pop(L, 1);
        }
        
        /* Enable all JIT optimizations explicitly */
        lua_getfield(L, -1, "opt");
        if (lua_isfunction(L, -1)) {
            /* Enable comprehensive optimizations */
            lua_pushstring(L, "+cse");      /* Common subexpression elimination */
            lua_pushstring(L, "+dce");      /* Dead code elimination */
            lua_pushstring(L, "+fwd");      /* Load/store forwarding */
            lua_pushstring(L, "+dse");      /* Dead store elimination */
            lua_pushstring(L, "+narrow");   /* Narrowing of number operations */
            lua_pushstring(L, "+loop");     /* Loop optimizations */
            lua_pushstring(L, "+abc");      /* Array bounds check elimination */
            lua_pushstring(L, "+sink");     /* Allocation/store sinking */
            lua_pushstring(L, "+fuse");     /* Fusion of operands into instructions */
            lua_pushstring(L, "+fold");     /* Constant folding */
            lua_pushstring(L, "+phielim");  /* PHI elimination */
            lua_pushstring(L, "+split");    /* Split optimizations */
            
            if (lua_pcall(L, 10, 0, 0) != LUA_OK) {
                lua_pop(L, 1); /* Continue on optimization failure */
            }
        } else {
            lua_pop(L, 1);
        }
        
        lua_pop(L, 1); /* Pop jit table */
    } else {
        lua_pop(L, 1);
    }
    
    /* Remove dangerous globals for security */
    const char* dangerous[] = {
        "dofile", "loadfile", "load", "require", "module", 
        "getfenv", "setfenv", "io", "os", "debug", "package", 
        "rawget", "rawset", "rawequal", "getmetatable", "setmetatable",
        "pcall", "xpcall", NULL
    };
    
    for (int i = 0; dangerous[i]; i++) {
        lua_pushnil(L);
        lua_setglobal(L, dangerous[i]);
    }
    
    /* Secure print function */
    lua_pushcfunction(L, lua_secure_print_fast);
    lua_setglobal(L, "print");
    
    /* Register custom functions */
    lua_pushcfunction(L, lua_c_get_persistent_variable);
    lua_setglobal(L, "GetPersistentVariable");
    
    lua_pushcfunction(L, lua_c_set_persistent_variable);
    lua_setglobal(L, "SetPersistentVariable");
    
    lua_pushcfunction(L, lua_c_delete_persistent_variable);
    lua_setglobal(L, "DeletePersistentVariable");

    lua_pushcfunction(L, lua_c_arma_call);
    lua_setglobal(L, "ArmaCall");
    
    lua_pushcfunction(L, lua_c_arma_stage_call);
    lua_setglobal(L, "ArmaStageCall");
    
    return 1;
}

/* Enhanced state management with health monitoring and initial state capture */
static lua_persistent_state_t* lua_get_optimized_state_with_affinity(uint64_t code_hash) {
    if (!g_lua_pool_initialized) {
        memset(g_lua_states, 0, sizeof(g_lua_states));
        g_lua_pool_initialized = 1;
    }
    
    DWORD current_time = GetTickCount();
    lua_persistent_state_t* best_state = NULL;
    lua_persistent_state_t* affinity_state = NULL;
    int best_score = -1;
    
    /* First pass: look for states with code affinity and available states */
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        lua_persistent_state_t* state = &g_lua_states[i];
        
        if (state->in_use) continue;
        
        /* Check if state has run this code before (JIT affinity) */
        if (state->initialized && state->code_hashes && code_hash != 0) {
            for (int j = 0; j < state->code_hash_count; j++) {
                if (state->code_hashes[j] == code_hash) {
                    /* Perfect match - this state has JIT compiled this code */
                    affinity_state = state;
                    break;
                }
            }
        }
        
        if (affinity_state) break; /* Found perfect match */
        
        /* Calculate state quality score */
        int score = 0;
        if (state->initialized && state->L) {
            score += 100; /* Prefer initialized states */
            score += state->execution_count; /* Prefer warmed up states */
            score -= state->error_count * 10; /* Avoid error-prone states */
            
            /* Prefer recently used states (JIT data still hot) */
            DWORD age = current_time - state->last_used_time;
            if (age < 30000) score += 50; /* Used in last 30 seconds */
            else if (age < 120000) score += 25; /* Used in last 2 minutes */
        }
        
        if (score > best_score) {
            best_score = score;
            best_state = state;
        }
    }
    
    /* Use affinity state if found, otherwise use best available */
    lua_persistent_state_t* selected_state = affinity_state ? affinity_state : best_state;
    
    if (selected_state) {
        if (!selected_state->initialized) {
            /* Initialize new state using standard Lua state creation */
            selected_state->L = luaL_newstate();
            if (!selected_state->L) return NULL;
            
            selected_state->jit_enabled = lua_init_turbo_state(selected_state->L);
            if (!selected_state->jit_enabled) {
                lua_close(selected_state->L);
                selected_state->L = NULL;
                return NULL;
            }
            
            /* Setup lazy variable injection */
            if (!lua_setup_lazy_variable_injection(selected_state->L)) {
                lua_close(selected_state->L);
                selected_state->L = NULL;
                return NULL;
            }
            
            /* Initialize code hash tracking */
            selected_state->code_hash_capacity = LUA_MAX_CODE_HASHES_PER_STATE;
            selected_state->code_hashes = (uint64_t*)calloc(selected_state->code_hash_capacity, sizeof(uint64_t));
            if (!selected_state->code_hashes) {
                lua_close(selected_state->L);
                selected_state->L = NULL;
                return NULL;
            }
            
            selected_state->initialized = 1;
            selected_state->error_count = 0;
            selected_state->execution_count = 0;
            selected_state->code_hash_count = 0;
            selected_state->creation_time = current_time;
            selected_state->last_cleanup_time = current_time;
        }
        
        selected_state->in_use = 1;
        selected_state->last_used_time = current_time;
        
        /* Track this code hash for future affinity */
        if (code_hash != 0 && selected_state->code_hashes) {
            int found = 0;
            for (int i = 0; i < selected_state->code_hash_count; i++) {
                if (selected_state->code_hashes[i] == code_hash) {
                    found = 1;
                    break;
                }
            }
            
            if (!found && selected_state->code_hash_count < selected_state->code_hash_capacity) {
                selected_state->code_hashes[selected_state->code_hash_count++] = code_hash;
            }
        }
        
        return selected_state;
    }

    int available_states = 0;
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        if (!g_lua_states[i].in_use) available_states++;
    }

    /* If pool pressure is high, aggressively clean old states */
    if (available_states < LUA_STATE_POOL_SIZE / 4) { /* Less than 25% available */
        DWORD aggressive_timeout = 1000; /* 1 second under pressure */
        for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
            lua_persistent_state_t* state = &g_lua_states[i];
            if (!state->in_use && state->L && 
                (current_time - state->last_used_time) > aggressive_timeout) {
                lua_close(state->L);
                free(state->code_hashes);
                memset(state, 0, sizeof(lua_persistent_state_t));
            }
        }
    }

    return NULL;
}

/* Enhanced argument clearing with better error handling */
static void lua_clear_argument_globals(lua_State* L) {
    if (!L || !lua_checkstack(L, 2)) return;
    
    /* Always clear the maximum number of arguments to ensure no leftovers */
    for (int i = 1; i <= LUA_MAX_SIMPLE_ARGS; i++) {
        char arg_name[16];
        if (_snprintf_s(arg_name, sizeof(arg_name), _TRUNCATE, "arg%d", i) > 0) {
            lua_pushnil(L);
            lua_setglobal(L, arg_name);
        }
    }
    
    /* Clear argc */
    lua_pushnil(L);
    lua_setglobal(L, "argc");
}

/* Enhanced state release with proper global restoration */
static void lua_release_optimized_state(lua_persistent_state_t* state) {
    if (!state || !state->L) return;
    
    DWORD current_time = GetTickCount();
    DWORD time_since_last_use = current_time - state->last_used_time;
    DWORD state_age = current_time - state->creation_time;
    
    /* Clear stack but preserve global state and JIT compilations */
    int stack_top = lua_gettop(state->L);
    if (stack_top > 0) {
        lua_settop(state->L, 0);
    }
    
    /* Clear argument globals but preserve other globals */
    lua_clear_argument_globals(state->L);
    
    /* Increment execution count for tracking JIT warmup */
    state->execution_count++;
    
    /* Adaptive cleanup based on usage patterns */
    int should_reset = 0;
    
    /* Always reset on excessive errors */
    if (state->error_count > 20 || state_age > 600000 || (state->execution_count > 1 && time_since_last_use > 120000) || (state->execution_count == 1 && time_since_last_use > 30000) || (state->execution_count < 5 && time_since_last_use > 60000)) {
        should_reset = 1;
    }
    
    if (should_reset) {
        lua_close(state->L);
        free(state->code_hashes);
        memset(state, 0, sizeof(lua_persistent_state_t));
        return;
    }
    
    /* Periodic garbage collection for active states */
    DWORD time_since_cleanup = current_time - state->last_cleanup_time;
    if (time_since_cleanup > 60000) { /* GC every minute for active states */
        lua_gc(state->L, LUA_GCCOLLECT, 0);
        state->last_cleanup_time = current_time;
    }
    
    state->in_use = 0;
}

/* Enhanced depth-limited array validation */
static int lua_is_key_value_pair(lua_State* L, int stack_index) {
    if (!lua_istable(L, stack_index)) return 0;
    
    /* Ensure we have stack space for operations */
    if (!lua_checkstack(L, 3)) return 0;
    
    lua_rawgeti(L, stack_index, 1);
    int has_first = !lua_isnil(L, -1);
    lua_pop(L, 1);
    
    if (!has_first) return 0;
    
    lua_rawgeti(L, stack_index, 2);
    int has_second = !lua_isnil(L, -1);
    lua_pop(L, 1);
    
    if (!has_second) return 0;
    
    lua_rawgeti(L, stack_index, 3);
    int has_third = !lua_isnil(L, -1);
    lua_pop(L, 1);
    
    return !has_third;
}

/* Enhanced hashmap conversion with error checking */
static int lua_convert_to_hashmap(lua_State* L, int table_stack_index, int element_count) {
    if (element_count == 0 || !lua_checkstack(L, 5)) return 0;
    
    /* Create new hash table */
    lua_newtable(L);
    int hash_table_index = lua_gettop(L);
    
    /* Convert each key-value pair */
    for (int i = 1; i <= element_count; i++) {
        lua_rawgeti(L, table_stack_index, i);
        
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); /* Get key */
            lua_rawgeti(L, -2, 2); /* Get value */
            
            /* Set hash_table[key] = value */
            if (!lua_isnil(L, -2)) { /* Ensure key is not nil */
                lua_pushvalue(L, -2);  /* Duplicate key */
                lua_pushvalue(L, -2);  /* Duplicate value */
                lua_settable(L, hash_table_index);
            }
            
            lua_pop(L, 2); /* Remove key and value */
        }
        
        lua_pop(L, 1); /* Remove pair table */
    }
    
    /* Replace the original table */
    lua_remove(L, table_stack_index);
    lua_insert(L, table_stack_index);
    
    return 1;
}

/* FIXED: Enhanced Arma array parsing with optimized stack reservations */
static const char* lua_parse_arma_array(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr || depth > LUA_MAX_RECURSION_DEPTH) return NULL;
    
    /* FIXED: More precise stack space calculation */
    int required_stack = 6 + (depth < 10 ? 2 : 1); /* Less aggressive stack reservation */
    if (!lua_checkstack(L, required_stack)) return NULL;
    
    /* Skip opening bracket */
    if (*ptr != '[') return NULL;
    ptr++;
    
    /* Create new Lua table */
    lua_newtable(L);
    int table_index = lua_gettop(L);
    int element_count = 0;
    
    /* Skip whitespace */
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    
    /* Handle empty array */
    if (ptr < end && *ptr == ']') {
        return ptr + 1;
    }
    
    /* Parse array elements with limits */
    int max_elements = (depth > 10) ? 100 : 1000; /* Limit elements at deeper levels */
    
    while (ptr < end && *ptr != ']' && element_count < max_elements) {
        /* Skip whitespace and commas */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
        if (ptr >= end || *ptr == ']') break;
        
        /* Parse the value recursively */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, depth + 1);
        if (!next_ptr) {
            lua_pop(L, 1); /* Remove the table */
            return NULL;
        }
        
        /* Set table[index] = value */
        element_count++;
        lua_rawseti(L, table_index, element_count);
        ptr = next_ptr;
    }
    
    /* Skip closing bracket */
    if (ptr < end && *ptr == ']') {
        ptr++;
    }
    
    /* Check if this should be converted to a hashmap */
    if (element_count > 0 && element_count <= 100) { /* Limit hashmap conversion */
        int is_hashmap = 1;
        
        for (int i = 1; i <= element_count && is_hashmap; i++) {
            lua_rawgeti(L, table_index, i);
            if (!lua_is_key_value_pair(L, -1)) {
                is_hashmap = 0;
            }
            lua_pop(L, 1);
        }
        
        if (is_hashmap) {
            if (!lua_convert_to_hashmap(L, table_index, element_count)) {
                /* Conversion failed, continue as array */
            }
        }
    }
    
    return ptr;
}

/* FIXED: Enhanced recursive value parsing with optimized memory usage for small strings */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr || ptr >= end || depth > LUA_MAX_RECURSION_DEPTH) return NULL;
    
    /* FIXED: More precise stack space calculation */
    int required_stack = 4 + (depth < 10 ? 2 : 1); /* Less aggressive stack reservation */
    if (!lua_checkstack(L, required_stack)) return NULL;
    
    /* Skip leading whitespace */
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (ptr >= end) return NULL;
    
    /* Check what type of value we're parsing */
    if (*ptr == '[') {
        return lua_parse_arma_array(L, ptr, end, depth);
    }
    
    /* Find the end of this value with bounds checking */
    const char* value_start = ptr;
    const char* value_end = ptr;
    int in_quotes = 0;
    int bracket_depth = 0;
    
    while (value_end < end) {
        char c = *value_end;
        
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes) {
            if (c == '[') {
                bracket_depth++;
            } else if (c == ']') {
                if (bracket_depth > 0) {
                    bracket_depth--;
                } else {
                    break;
                }
            } else if (c == ',' && bracket_depth == 0) {
                break;
            }
        }
        
        value_end++;
    }
    
    /* Calculate actual value length */
    size_t value_len = value_end - value_start;
    if (value_len == 0) return NULL;
    
    /* FIXED: Use stack buffer for small strings, heap for large ones */
    char stack_buffer[1024];
    char* value_buffer = NULL;
    
    if (value_len < sizeof(stack_buffer)) {
        value_buffer = stack_buffer;
    } else {
        value_buffer = (char*)malloc(value_len + 1);
        if (!value_buffer) return NULL;
    }
    
    memcpy(value_buffer, value_start, value_len);
    value_buffer[value_len] = '\0';
    
    /* Trim whitespace safely */
    char* trimmed = value_buffer;
    while (*trimmed && (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n')) trimmed++;
    
    size_t trimmed_len = strlen(trimmed);
    if (trimmed_len > 0) {
        char* trim_end = trimmed + trimmed_len - 1;
        while (trim_end > trimmed && (*trim_end == ' ' || *trim_end == '\t' || *trim_end == '\n')) {
            *trim_end-- = '\0';
        }
    }
    
    const char* result_ptr = value_end;
    
    /* Parse the value based on its content */
    if (strcmp(trimmed, "true") == 0) {
        lua_pushboolean(L, 1);
    } else if (strcmp(trimmed, "false") == 0) {
        lua_pushboolean(L, 0);
    } else if (strcmp(trimmed, "nil") == 0) {
        lua_pushnil(L);
    } else if (trimmed_len >= 2 && trimmed[0] == '"' && trimmed[trimmed_len-1] == '"') {
        /* String literal - remove quotes */
        trimmed[trimmed_len-1] = '\0';
        lua_pushstring(L, trimmed + 1);
    } else {
        /* Try as number first, fallback to string */
        char* endptr;
        double num = strtod(trimmed, &endptr);
        if (*endptr == '\0' && endptr != trimmed && !isnan(num) && !isinf(num)) {
            lua_pushnumber(L, num);
        } else {
            lua_pushstring(L, trimmed);
        }
    }
    
    /* Free heap buffer if used */
    if (value_buffer != stack_buffer) {
        free(value_buffer);
    }
    
    return result_ptr;
}

/* Enhanced argument parsing with comprehensive error handling and proper return values */
static int lua_parse_args_optimized(lua_State* L, const char* args_str) {
    if (!args_str || !L || !lua_checkstack(L, LUA_STACK_SAFETY_MARGIN)) return -1;
    
    /* Clear all previous argument globals first */
    lua_clear_argument_globals(L);
    
    /* Skip to opening bracket */
    const char* ptr = args_str;
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr != '[') return -1;
    ptr++;
    
    /* Handle empty array */
    const char* end = args_str + strlen(args_str);
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (ptr < end && *ptr == ']') {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
        return 0;
    }
    
    /* Ultra-fast single number detection */
    if (ptr < end && (isdigit(*ptr) || *ptr == '-' || *ptr == '+')) {
        const char* num_start = ptr;
        const char* num_end = ptr;
        
        if (*num_end == '-' || *num_end == '+') num_end++;
        while (num_end < end && (isdigit(*num_end) || *num_end == '.')) num_end++;
        
        const char* check = num_end;
        while (check < end && (*check == ' ' || *check == '\t' || *check == '\n')) check++;
        if (check < end && *check == ']' && num_end > num_start) {
            char num_buffer[32];
            int len = (int)(num_end - num_start);
            if (len < sizeof(num_buffer)) {
                memcpy(num_buffer, num_start, len);
                num_buffer[len] = '\0';
                
                char* endptr;
                double num = strtod(num_buffer, &endptr);
                if (*endptr == '\0' && !isnan(num) && !isinf(num)) {
                    if (!lua_checkstack(L, 2)) {
                        lua_pushnil(L);
                        lua_setglobal(L, "argc");
                        return -1;
                    }
                    
                    lua_pushnumber(L, num);
                    lua_setglobal(L, "arg1");
                    
                    lua_pushinteger(L, 1);
                    lua_setglobal(L, "argc");
                    
                    return 1;
                }
            }
        }
    }
    
    /* General parsing with recursive array support - Better error handling */
    int arg_count = 0;
    int initial_stack_top = lua_gettop(L);
    
    while (ptr < end && *ptr != ']' && arg_count < LUA_MAX_SIMPLE_ARGS) {
        /* Skip separators and whitespace */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
        if (ptr >= end || *ptr == ']') break;
        
        /* Ensure stack space before parsing */
        if (!lua_checkstack(L, 5)) {
            /* Clean up any globals we set before the failure */
            for (int cleanup_i = 1; cleanup_i <= arg_count; cleanup_i++) {
                char cleanup_name[16];
                if (_snprintf_s(cleanup_name, sizeof(cleanup_name), _TRUNCATE, "arg%d", cleanup_i) > 0) {
                    lua_pushnil(L);
                    lua_setglobal(L, cleanup_name);
                }
            }
            lua_pushnil(L);
            lua_setglobal(L, "argc");
            lua_settop(L, initial_stack_top);
            return -1;
        }
        
        /* Parse this argument recursively */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, 0);
        if (!next_ptr) {
            /* Clean up any globals we set before the failure */
            for (int cleanup_i = 1; cleanup_i <= arg_count; cleanup_i++) {
                char cleanup_name[16];
                if (_snprintf_s(cleanup_name, sizeof(cleanup_name), _TRUNCATE, "arg%d", cleanup_i) > 0) {
                    lua_pushnil(L);
                    lua_setglobal(L, cleanup_name);
                }
            }
            lua_pushnil(L);
            lua_setglobal(L, "argc");
            
            /* Clean up stack on parsing failure */
            lua_settop(L, initial_stack_top);
            return -1;
        }
        
        arg_count++;
        
        /* Set as global variable argN */
        char arg_name[16];
        if (_snprintf_s(arg_name, sizeof(arg_name), _TRUNCATE, "arg%d", arg_count) > 0) {
            lua_setglobal(L, arg_name);
        } else {
            /* If naming failed, clean up what we've set so far */
            lua_pop(L, 1); /* Remove the value from stack */
            arg_count--; /* Don't count this failed argument */
            
            /* Clean up any globals we set before this failure */
            for (int cleanup_i = 1; cleanup_i <= arg_count; cleanup_i++) {
                char cleanup_name[16];
                if (_snprintf_s(cleanup_name, sizeof(cleanup_name), _TRUNCATE, "arg%d", cleanup_i) > 0) {
                    lua_pushnil(L);
                    lua_setglobal(L, cleanup_name);
                }
            }
            lua_pushnil(L);
            lua_setglobal(L, "argc");
            
            lua_settop(L, initial_stack_top);
            return -1;
        }
        
        ptr = next_ptr;
    }
    
    /* Set argc global variable */
    if (!lua_checkstack(L, 2)) {
        /* Clean up on final failure */
        for (int cleanup_i = 1; cleanup_i <= arg_count; cleanup_i++) {
            char cleanup_name[16];
            if (_snprintf_s(cleanup_name, sizeof(cleanup_name), _TRUNCATE, "arg%d", cleanup_i) > 0) {
                lua_pushnil(L);
                lua_setglobal(L, cleanup_name);
            }
        }
        lua_settop(L, initial_stack_top);
        return -1;
    }
    
    lua_pushinteger(L, arg_count);
    lua_setglobal(L, "argc");
    
    return arg_count;
}

/* Forward declarations for table conversion */
static int lua_table_to_arma_recursive(lua_State* L, int stack_index, char* buffer, 
                                      size_t buffer_size, size_t* pos, int depth);

/* Enhanced array detection with error handling */
static int lua_is_array(lua_State* L, int stack_index) {
    if (!lua_istable(L, stack_index) || !lua_checkstack(L, 2)) return 0;
    
    lua_pushnil(L);
    int max_index = 0;
    int key_count = 0;
    int has_non_integer_key = 0;
    
    while (lua_next(L, stack_index) != 0) {
        if (lua_type(L, -2) == LUA_TNUMBER) {
            double key = lua_tonumber(L, -2);
            if (key == floor(key) && key >= 1 && key <= 10000) { /* Reasonable limits */
                int int_key = (int)key;
                if (int_key > max_index) max_index = int_key;
            } else {
                has_non_integer_key = 1;
            }
        } else {
            has_non_integer_key = 1;
        }
        key_count++;
        
        /* Prevent infinite loops */
        if (key_count > 10000) {
            lua_pop(L, 2); /* Clean up stack */
            return 0;
        }
        
        lua_pop(L, 1);
    }
    
    return (!has_non_integer_key && max_index == key_count && key_count > 0);
}

/* Enhanced value to string conversion with bounds checking */
static int lua_value_to_arma_string(lua_State* L, int stack_index, char* buffer, 
                                   size_t buffer_size, size_t* pos, int depth) {
    if (*pos >= buffer_size - 1 || depth > LUA_MAX_RECURSION_DEPTH) return 0;
    
    if (!lua_checkstack(L, 3)) return 0;
    
    /* Normalize to absolute index */
    int abs_stack_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    int type = lua_type(L, abs_stack_index);
    
    switch (type) {
        case LUA_TNIL:
            if (*pos + 3 < buffer_size) {
                memcpy(buffer + *pos, "nil", 3);
                *pos += 3;
                return 1;
            }
            break;
            
        case LUA_TBOOLEAN: {
            const char* bool_str = lua_toboolean(L, abs_stack_index) ? "true" : "false";
            size_t bool_len = strlen(bool_str);
            if (*pos + bool_len < buffer_size) {
                memcpy(buffer + *pos, bool_str, bool_len);
                *pos += bool_len;
                return 1;
            }
            break;
        }
        
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, abs_stack_index);
            
            /* Check for NaN and infinity */
            if (isnan(num) || isinf(num)) {
                const char* special = "nil";
                size_t special_len = 3;
                if (*pos + special_len < buffer_size) {
                    memcpy(buffer + *pos, special, special_len);
                    *pos += special_len;
                    return 1;
                }
                return 0;
            }
            
            char num_buffer[64];
            int num_len;
            
            if (num == floor(num) && num >= -2147483648.0 && num <= 2147483647.0) {
                num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.0f", num);
            } else {
                num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.15g", num);
            }
            
            if (num_len > 0 && *pos + (size_t)num_len < buffer_size) {
                memcpy(buffer + *pos, num_buffer, num_len);
                *pos += num_len;
                return 1;
            }
            break;
        }
        
        case LUA_TSTRING: {
            const char* str = lua_tostring(L, abs_stack_index);
            if (!str) return 0;
            
            size_t str_len = strlen(str);
            
            /* Enhanced bounds checking for string escaping */
            size_t worst_case_len = str_len * 2 + 2; /* Worst case: all quotes doubled plus outer quotes */
            if (*pos + worst_case_len >= buffer_size) {
                /* Try to fit truncated string safely */
                size_t remaining = buffer_size - *pos;
                if (remaining < 3) return 0; /* Not enough space for even "" */
                
                buffer[(*pos)++] = '"';
                remaining--;
                
                size_t i = 0;
                while (i < str_len && remaining > 1) { /* Keep 1 byte for closing quote */
                    if (str[i] == '"' && remaining > 2) { /* Need 2 bytes for "" */
                        buffer[(*pos)++] = '"';
                        buffer[(*pos)++] = '"';
                        remaining -= 2;
                    } else if (str[i] != '"' && remaining > 1) {
                        buffer[(*pos)++] = str[i];
                        remaining--;
                    } else {
                        break; /* Not enough space */
                    }
                    i++;
                }
                
                if (remaining > 0) {
                    buffer[(*pos)++] = '"';
                    return 1;
                }
                return 0;
            } else {
                /* Normal case with full string */
                buffer[(*pos)++] = '"';
                for (size_t i = 0; i < str_len; i++) {
                    if (str[i] == '"') {
                        buffer[(*pos)++] = '"';
                        buffer[(*pos)++] = '"';
                    } else {
                        buffer[(*pos)++] = str[i];
                    }
                }
                buffer[(*pos)++] = '"';
                return 1;
            }
            break;
        }
        
        case LUA_TTABLE:
            return lua_table_to_arma_recursive(L, abs_stack_index, buffer, buffer_size, pos, depth + 1);
            
        default: {
            const char* type_str = "[unknown]";
            size_t type_len = strlen(type_str);
            if (*pos + type_len < buffer_size) {
                memcpy(buffer + *pos, type_str, type_len);
                *pos += type_len;
                return 1;
            }
            break;
        }
    }
    
    return 0;
}

/* Enhanced table to Arma conversion with comprehensive error handling */
/* Enhanced table to Arma conversion with comprehensive error handling and proper stack management */
static int lua_table_to_arma_recursive(lua_State* L, int stack_index, char* buffer, 
                                      size_t buffer_size, size_t* pos, int depth) {
    if (*pos >= buffer_size - 2 || depth > LUA_MAX_RECURSION_DEPTH) return 0;
    
    if (!lua_checkstack(L, 5)) return 0; /* Ensure enough stack space */
    
    /* Normalize stack index to absolute index to handle stack changes */
    int abs_stack_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    
    buffer[(*pos)++] = '[';
    
    if (lua_is_array(L, abs_stack_index)) {
        /* Convert as array */
        int array_length = (int)lua_rawlen(L, abs_stack_index);
        
        /* Limit array size for performance and safety */
        if (array_length > 1000) array_length = 1000;
        
        for (int i = 1; i <= array_length; i++) {
            if (i > 1) {
                if (*pos + 1 < buffer_size) {
                    buffer[(*pos)++] = ',';
                } else {
                    return 0;
                }
            }
            
            int stack_before = lua_gettop(L);
            lua_rawgeti(L, abs_stack_index, i);
            int success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            lua_settop(L, stack_before); /* Ensure stack is restored */
            
            if (!success) return 0;
        }
    } else {
        /* Convert as hashmap with proper stack management */
        int initial_stack_top = lua_gettop(L);
        lua_pushnil(L);
        int first_pair = 1;
        int pair_count = 0;
        const int max_pairs = 100; /* Limit hashmap size */
        
        while (lua_next(L, abs_stack_index) != 0 && pair_count < max_pairs) {
            /* Stack now has: [initial_stack] [key] [value] */
            int iteration_stack_top = lua_gettop(L);
            
            if (!first_pair) {
                if (*pos + 1 < buffer_size) {
                    buffer[(*pos)++] = ',';
                } else {
                    /* Clean up iteration state properly */
                    lua_settop(L, initial_stack_top);
                    return 0;
                }
            }
            first_pair = 0;
            pair_count++;
            
            /* Start key-value pair array */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = '[';
            } else {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* Add key - key is at index -2 */
            int key_success = lua_value_to_arma_string(L, -2, buffer, buffer_size, pos, depth);
            if (!key_success) {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* Add comma */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = ',';
            } else {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* Add value - value is at index -1 */
            int value_success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            if (!value_success) {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* End key-value pair array */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = ']';
            } else {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* Remove value, keep key for next iteration */
            lua_pop(L, 1);
            /* Stack now has: [initial_stack] [key] */
        }
        
        /* Restore stack to initial state */
        lua_settop(L, initial_stack_top);
    }
    
    /* Close the array */
    if (*pos < buffer_size) {
        buffer[(*pos)++] = ']';
        return 1;
    }
    
    return 0;
}

/* Enhanced result formatting with comprehensive error handling */
static int lua_format_result_optimized(lua_State* L, char* output, int output_size) {
    if (!L || !output || output_size <= 10) return 0; /* Need space for [[], []] minimum */
    
    if (!lua_checkstack(L, 3)) return 0;
    
    /* New format: [[result], [staged_calls]] */
    output[0] = '[';
    size_t pos = 1;
    
    /* First element: [result] */
    if (pos + 1 >= (size_t)output_size) {
        strcpy_s(output, output_size, "[[nil], []]");
        lua_clear_staged_calls();
        return 1;
    }
    output[pos++] = '[';
    
    int top = lua_gettop(L);
    
    /* Calculate space needed for staged calls structure with overflow protection */
    size_t staged_calls_space_needed = 2; /* [] minimum for empty staged calls */
    
    /* Pre-calculate space needed for all staged calls with overflow checking */
    for (int i = 0; i < g_staged_calls.count; i++) {
        size_t current_call_space = 0;
        
        if (i > 0) current_call_space += 1; /* comma separator */
        current_call_space += 1; /* opening [ */
        
        size_t args_len = strlen(g_staged_calls.calls[i].arguments);
        size_t func_len = strlen(g_staged_calls.calls[i].function);
        
        /* Check for overflow in individual components */
        if (args_len > SIZE_MAX - current_call_space - 10) {
            /* Space calculation would overflow, use fallback */
            strcpy_s(output, output_size, "[[nil], []]");
            lua_clear_staged_calls();
            return 1;
        }
        
        current_call_space += args_len;
        current_call_space += 1; /* comma separator */
        current_call_space += 2; /* opening and closing quotes for function */
        
        /* Calculate space needed for escaped function name with overflow protection */
        size_t escaped_func_space = 0;
        for (size_t j = 0; j < func_len; j++) {
            if (g_staged_calls.calls[i].function[j] == '"') {
                escaped_func_space += 2; /* escaped quote */
            } else {
                escaped_func_space += 1; /* regular char */
            }
            
            /* Check for overflow in escaping calculation */
            if (escaped_func_space > SIZE_MAX - current_call_space - 10) {
                strcpy_s(output, output_size, "[[nil], []]");
                lua_clear_staged_calls();
                return 1;
            }
        }
        
        current_call_space += escaped_func_space;
        current_call_space += 1; /* closing ] */
        
        /* Check for overflow in total space calculation */
        if (staged_calls_space_needed > SIZE_MAX - current_call_space) {
            strcpy_s(output, output_size, "[[nil], []]");
            lua_clear_staged_calls();
            return 1;
        }
        
        staged_calls_space_needed += current_call_space;
    }
    
    /* Reserve space for structure: ],[ and final ] */
    size_t structure_overhead = 4;
    size_t total_suffix_space = staged_calls_space_needed + structure_overhead;
    
    /* Check for overflow in total calculation */
    if (staged_calls_space_needed > SIZE_MAX - structure_overhead) {
        strcpy_s(output, output_size, "[[nil], []]");
        lua_clear_staged_calls();
        return 1;
    }
    
    /* Calculate space available for result */
    size_t max_result_space = (output_size > total_suffix_space + pos) ? 
                             (output_size - total_suffix_space) : pos + 10; /* minimal fallback */
    
    if (top == 0) {
        /* No results, output nil */
        const char* nil_result = "nil";
        size_t nil_len = strlen(nil_result);
        if (pos + nil_len < max_result_space) {
            memcpy(output + pos, nil_result, nil_len);
            pos += nil_len;
        } else {
            /* Can't fit nil, use fallback */
            strcpy_s(output, output_size, "[[nil], []]");
            lua_clear_staged_calls();
            return 1;
        }
    } else {
        /* Handle the top result with calculated space */
        size_t pos_before_result = pos;
        if (!lua_value_to_arma_string(L, -1, output, (int)max_result_space, &pos, 0)) {
            /* Result conversion failed or didn't fit, use fallback */
            pos = pos_before_result;
            const char* fallback = "nil";
            size_t fallback_len = strlen(fallback);
            if (pos + fallback_len < max_result_space) {
                memcpy(output + pos, fallback, fallback_len);
                pos += fallback_len;
            } else {
                /* Even fallback doesn't fit */
                strcpy_s(output, output_size, "[[nil], []]");
                lua_clear_staged_calls();
                return 1;
            }
        }
    }
    
    /* Close first element: ] */
    if (pos + 1 >= (size_t)output_size) {
        strcpy_s(output, output_size, "[[nil], []]");
        lua_clear_staged_calls();
        return 1;
    }
    output[pos++] = ']';
    
    /* Add comma separator */
    if (pos + 1 >= (size_t)output_size) {
        strcpy_s(output, output_size, "[[nil], []]");
        lua_clear_staged_calls();
        return 1;
    }
    output[pos++] = ',';
    
    /* Second element: [staged_calls] */
    if (pos + 1 >= (size_t)output_size) {
        strcpy_s(output, output_size, "[[nil], []]");
        lua_clear_staged_calls();
        return 1;
    }
    output[pos++] = '[';
    
    /* Check if we have enough space for all staged calls */
    if (pos + staged_calls_space_needed + 1 > (size_t)output_size) {
        /* Not enough space for staged calls, just close empty */
        if (pos + 1 < (size_t)output_size) {
            output[pos++] = ']';
            if (pos + 1 < (size_t)output_size) {
                output[pos++] = ']';
                output[pos] = '\0';
            } else {
                output[output_size - 1] = '\0';
            }
        } else {
            output[output_size - 1] = '\0';
        }
        lua_clear_staged_calls();
        return 1;
    }
    
    /* Add staged calls - now we know we have enough space */
    for (int i = 0; i < g_staged_calls.count; i++) {
        if (i > 0) {
            output[pos++] = ',';
        }
        
        /* Start staged call array */
        output[pos++] = '[';
        
        /* Add arguments */
        size_t args_len = strlen(g_staged_calls.calls[i].arguments);
        memcpy(output + pos, g_staged_calls.calls[i].arguments, args_len);
        pos += args_len;
        
        /* Add comma separator */
        output[pos++] = ',';
        
        /* Add function as quoted string */
        output[pos++] = '"';
        
        /* Add function with quote escaping - we know we have space */
        const char* func_str = g_staged_calls.calls[i].function;
        size_t func_len = strlen(func_str);
        
        for (size_t j = 0; j < func_len; j++) {
            if (func_str[j] == '"') {
                output[pos++] = '"';
                output[pos++] = '"';
            } else {
                output[pos++] = func_str[j];
            }
        }
        
        /* Close function quote */
        output[pos++] = '"';
        
        /* Close staged call array */
        output[pos++] = ']';
    }
    
    /* Close staged calls array */
    output[pos++] = ']';
    
    /* Close main array */
    output[pos++] = ']';
    output[pos] = '\0';
    
    /* Clear staged calls after successful formatting */
    lua_clear_staged_calls();
    
    return 1;
}

/* Enhanced execution function with proper error handling for argument parsing */
static int kh_execute_lua_optimized(const char* arguments, const char* code, 
                                   char* output, int output_size) {
    if (!arguments || !code || !output || output_size <= 0) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    size_t code_len = strlen(code);
    size_t args_len = strlen(arguments);
    
    if (code_len == 0) {
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }
    
    /* Clean the code string */
    char* clean_code = (char*)malloc(code_len + 1);
    if (!clean_code) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    if (strlen(clean_code) == 0) {
        free(clean_code);
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY AFTER CLEANING");
        return 1;
    }
    
    /* Hash the code for state affinity */
    uint64_t code_hash = lua_hash_fnv1a(clean_code, strlen(clean_code));
    
    /* Get state with JIT affinity */
    lua_persistent_state_t* state = lua_get_optimized_state_with_affinity(code_hash);
    if (!state) {
        free(clean_code);
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    int initial_stack_top = lua_gettop(L);
    
    /* Fast argument parsing check */
    int has_args = 0;
    const char* args_check = arguments;
    while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
    if (*args_check == '[') {
        args_check++;
        while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
        if (*args_check != ']') has_args = 1;
    } else {
        has_args = 1;
    }
    
    /* Parse arguments only if needed */
    if (has_args) {
        int arg_parse_result = lua_parse_args_optimized(L, arguments);
        if (arg_parse_result < 0) {
            lua_settop(L, initial_stack_top);
            lua_release_optimized_state(state);
            free(clean_code);
            kh_set_error(output, output_size, "ARGUMENT PARSING FAILED");
            return 1;
        }
    } else {
        /* Set argc to 0 for consistency */
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
    }
    
    /* Direct compilation and execution - let LuaJIT handle caching internally */
    int compile_result = luaL_loadstring(L, clean_code);
    
    if (compile_result == LUA_OK) {
        /* Execute with error handling */
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            if (lua_format_result_optimized(L, output, output_size)) {
                result = 0; /* Success */
            } else {
                kh_set_error(output, output_size, "RESULT FORMAT FAILED");
                state->error_count++;
            }
        } else {
            const char* error_msg = lua_tostring(L, -1);
            _snprintf_s(output, output_size, _TRUNCATE, 
                       KH_ERROR_PREFIX "EXECUTION: %s", 
                       error_msg ? error_msg : "unknown error");
            state->error_count++;
            state->last_error_time = GetTickCount();
        }
    } else {
        const char* error_msg = lua_tostring(L, -1);
        _snprintf_s(output, output_size, _TRUNCATE, 
                   KH_ERROR_PREFIX "COMPILATION: %s", 
                   error_msg ? error_msg : "syntax error");
        state->error_count++;
        state->last_error_time = GetTickCount();
    }
    
    /* Ensure stack is properly cleaned up */
    lua_settop(L, initial_stack_top);
    
    /* Release state */
    lua_release_optimized_state(state);
    free(clean_code);
    
    return result;
}

/* Main entry point - streamlined for maximum performance */
static int kh_process_lua_operation(char* output, int output_size, 
                                   const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 2 || !argv || !argv[0] || !argv[1]) {
        kh_set_error(output, output_size, "REQUIRES 2 ARGUMENTS: [ARGUMENTS, CODE]");
        return 1;
    }
    
    return kh_execute_lua_optimized(argv[0], argv[1], output, output_size);
}

/* High-performance Lua data to Arma string conversion with enhanced error handling */
static int arma_convert_lua_data_to_string(lua_State* L, int stack_index, char* output, int output_size) {
    if (!L || !output || output_size <= 3) return 0;
    
    if (!lua_checkstack(L, 3)) return 0;
    
    /* Normalize to absolute stack index */
    int abs_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    
    /* Validate that we have a table/array */
    if (!lua_istable(L, abs_index)) {
        /* Single value - wrap in array format */
        output[0] = '[';
        size_t pos = 1;
        
        if (lua_value_to_arma_string(L, abs_index, output, output_size, &pos, 0)) {
            if (pos < (size_t)output_size) {
                output[pos] = ']';
                output[pos + 1] = '\0';
                return 1;
            }
        }
        return 0;
    }
    
    /* Convert table using existing optimized function */
    size_t pos = 0;
    return lua_table_to_arma_recursive(L, abs_index, output, output_size, &pos, 0);
}

/* Safe callback wrapper with retry logic and comprehensive error handling */
static int arma_callback_safe_call(const char* data, const char* function, char* error_output, int error_size) {
    if (!g_callback_initialized || !g_arma_callback) {
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "CALLBACK NOT INITIALIZED");
        }
        return 0;
    }
    
    if (!data || !function) {
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "NULL DATA OR FUNCTION");
        }
        return 0;
    }
    
    /* Validate data and function lengths to prevent buffer overflows */
    size_t data_len = strlen(data);
    size_t function_len = strlen(function);
    
    if (data_len == 0) {
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "EMPTY DATA");
        }
        return 0;
    }
    
    if (function_len == 0) {
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "EMPTY FUNCTION");
        }
        return 0;
    }
    
    if (data_len > 32767 || function_len > 1023) { /* Reasonable limits for Arma */
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "DATA OR FUNCTION TOO LARGE");
        }
        return 0;
    }
    
    int last_result = g_arma_callback(g_extension_name, function, data);

    if (last_result >= 0) {
        /* Success */
        return 1;
    } else if (last_result == -1) {
        /* Buffer full - fail immediately, let Lua handle retry logic */
        if (error_output && error_size > 0) {
            kh_set_error(error_output, error_size, "CALLBACK BUFFER FULL");
        }
    }
    
    return 0;
}

/* Enhanced LuaCompile operation - Validate Lua code syntax without execution */
static int kh_process_lua_compile_operation(char* output, int output_size, 
                                           const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc != 1 || !argv || !argv[0]) {
        kh_set_error(output, output_size, "REQUIRES 1 ARGUMENT: [CODE]");
        return 1;
    }
    
    const char* code = argv[0];
    size_t code_len = strlen(code);
    
    if (code_len == 0) {
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }
    
    /* Prevent excessively large code strings to avoid memory issues */
    if (code_len > KH_MAX_OUTPUT_SIZE * 4) { /* 32KB limit */
        kh_set_error(output, output_size, "CODE TOO LARGE");
        return 1;
    }
    
    /* Clean the code string (remove surrounding quotes) */
    char* clean_code = (char*)malloc(code_len + 1);
    if (!clean_code) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    if (strlen(clean_code) == 0) {
        free(clean_code);
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY AFTER CLEANING");
        return 1;
    }
    
    /* Get a Lua state for compilation - no code hash since we don't execute */
    lua_persistent_state_t* state = lua_get_optimized_state_with_affinity(0);
    if (!state) {
        free(clean_code);
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1; /* Default to error */
    int initial_stack_top = lua_gettop(L);
    
    /* Attempt to compile the code without executing it */
    int compile_result = luaL_loadstring(L, clean_code);
    
    if (compile_result == LUA_OK) {
        /* Compilation successful - code has valid syntax */
        _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\"]");
        result = 0;
    } else {
        /* Compilation failed - extract and format error message */
        const char* error_msg = lua_tostring(L, -1);
        if (error_msg && strlen(error_msg) > 0) {
            /* Truncate error message if too long to prevent buffer overflow */
            char truncated_error[512];
            size_t error_len = strlen(error_msg);
            if (error_len >= sizeof(truncated_error)) {
                memcpy(truncated_error, error_msg, sizeof(truncated_error) - 4);
                strcpy_s(truncated_error + sizeof(truncated_error) - 4, 4, "...");
            } else {
                strcpy_s(truncated_error, sizeof(truncated_error), error_msg);
            }
            
            _snprintf_s(output, output_size, _TRUNCATE, 
                       KH_ERROR_PREFIX "COMPILATION: %s", truncated_error);
        } else {
            kh_set_error(output, output_size, "COMPILATION: SYNTAX ERROR");
        }
        
        /* Update state error tracking */
        state->error_count++;
        state->last_error_time = GetTickCount();
    }
    
    /* Ensure stack is properly cleaned up */
    lua_settop(L, initial_stack_top);
    
    /* Release state and cleanup memory */
    lua_release_optimized_state(state);
    free(clean_code);
    
    return result;
}

/* Lua C function: ArmaCall(data, function) - High-performance implementation */
static int lua_c_arma_call(lua_State* L) {
    /* Ensure adequate stack space for error handling */
    if (!lua_checkstack(L, 5)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "STACK OVERFLOW");
        return 2;
    }
    
    /* Validate argument count */
    int argc = lua_gettop(L);
    if (argc != 2) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "REQUIRES 2 ARGUMENTS: ArmaCall(data, function)");
        return 2;
    }
    
    /* Validate callback initialization */
    if (!g_callback_initialized || !g_arma_callback) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "ARMA CALLBACK NOT AVAILABLE");
        return 2;
    }
    
    /* Get function argument (must be string) */
    if (!lua_isstring(L, 2)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION MUST BE A STRING");
        return 2;
    }
    
    const char* function = lua_tostring(L, 2);
    if (!function) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION CANNOT BE EMPTY");
        return 2;
    }
    
    /* Cache string length to avoid multiple strlen calls */
    size_t function_len = strlen(function);
    if (function_len == 0) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION CANNOT BE EMPTY");
        return 2;
    }
    
    /* Clean the function string - use standard allocation */
    char* clean_function = (char*)malloc(function_len + 1);
    
    if (!clean_function) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "MEMORY ALLOCATION FAILED FOR FUNCTION");
        return 2;
    }
    
    kh_clean_string(function, clean_function, (int)function_len + 1);
    
    /* Cache cleaned string length to avoid strlen call */
    size_t clean_function_len = strlen(clean_function);
    if (clean_function_len == 0) {
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "FUNCTION CANNOT BE EMPTY AFTER CLEANING");
        return 2;
    }
    
    /* Convert data to Arma format - use standard allocation */
    char* data_buffer = (char*)malloc(KH_MAX_OUTPUT_SIZE);
    
    if (!data_buffer) {
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "DATA BUFFER ALLOCATION FAILED");
        return 2;
    }
    
    /* Convert the first argument (data) to Arma array format */
    if (!arma_convert_lua_data_to_string(L, 1, data_buffer, KH_MAX_OUTPUT_SIZE)) {
        free(data_buffer);
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "DATA CONVERSION FAILED");
        return 2;
    }
    
    /* Validate converted data length */
    size_t data_len = strlen(data_buffer);
    if (data_len == 0) {
        free(data_buffer);
        free(clean_function);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "CONVERTED DATA IS EMPTY");
        return 2;
    }
    
    /* Perform the callback with comprehensive error handling */
    char error_buffer[512];
    int callback_success = arma_callback_safe_call(data_buffer, clean_function, 
                                                  error_buffer, sizeof(error_buffer));
    
    /* Cleanup all allocations */
    free(data_buffer);
    free(clean_function);
    
    /* Return results to Lua */
    if (callback_success) {
        lua_pushboolean(L, 1);
        lua_pushstring(L, "SUCCESS");
        return 2;
    } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, error_buffer[0] ? error_buffer : "UNKNOWN CALLBACK ERROR");
        return 2;
    }
}

/* FIXED: Cleanup function with proper memory management */
static void kh_cleanup_lua_states(void) {
    /* Clean up persistent Lua variables */
    if (g_lua_variable_storage.initialized) {
        for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
            free(g_lua_variable_storage.variables[i].name);
            free(g_lua_variable_storage.variables[i].original_value);
            free(g_lua_variable_storage.variables[i].type);
        }
        free(g_lua_variable_storage.variables);
        free(g_lua_variable_storage.hash_table.entries);
        memset(&g_lua_variable_storage, 0, sizeof(g_lua_variable_storage));
    }
    
    /* Clean up staged calls */
    lua_cleanup_staged_calls_storage();
    
    /* Clean up Lua states */
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        if (g_lua_states[i].L) {
            /* Clean up code hash tracking */
            free(g_lua_states[i].code_hashes);
            
            /* Close Lua state */
            lua_close(g_lua_states[i].L);
        }
        
        /* Clear the entire state structure */
        memset(&g_lua_states[i], 0, sizeof(lua_persistent_state_t));
    }
    
    g_lua_pool_initialized = 0;
}

#endif /* LUA_INTEGRATION_H */