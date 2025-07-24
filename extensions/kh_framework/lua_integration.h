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

/* Simplified single Lua state structure */
typedef struct {
    lua_State* L;
    int initialized;
    int jit_enabled;
    int error_count;
    DWORD creation_time;
} lua_single_state_t;

/* Single global Lua state - replaces the pool */
static lua_single_state_t g_lua_state = {NULL, 0, 0, 0, 0};
static int g_lua_initialized = 0;

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

/* Global persistent Lua variables storage with hash table */
static lua_variable_storage_t g_lua_variable_storage = {0};
static int(*g_arma_callback)(char const *name, char const *function, char const *data) = NULL;
static int g_callback_initialized = 0;
static char g_extension_name[64] = EXTENSION_NAME; /* Default extension name for callbacks */

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
        
        /* Move to next position with consistent quadratic probing */
        distance++;
        
        /* Prevent integer overflow in quadratic probing - match find function */
        if (distance > 65535) break; /* Reasonable upper limit */
        
        uint64_t next_offset = (uint64_t)distance * distance;
        if (next_offset > hash_table_size) {
            /* Linear probing fallback when quadratic would overflow */
            index = (original_index + distance) % hash_table_size;
        } else {
            index = (original_index + (uint32_t)next_offset) % hash_table_size;
        }
        
        if (distance >= hash_table_size) break; /* Prevent infinite loops */
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
    
    /* Rebuild hash table entries in the new table first */
    uint32_t new_used_count = 0;
    for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
        if (g_lua_variable_storage.variables[i].name) {
            uint32_t name_hash = lua_hash_variable_name(g_lua_variable_storage.variables[i].name);
            if (!lua_hash_table_insert_robin_hood(new_entries, new_size, name_hash, i)) {
                /* Rebuild failed - free the new table and keep the old one */
                free(new_entries);
                return 0;
            }
            new_used_count++;
        }
    }
    
    /* Only now that we're sure rebuild succeeded, replace the old table */
    free(g_lua_variable_storage.hash_table.entries);
    g_lua_variable_storage.hash_table.entries = new_entries;
    g_lua_variable_storage.hash_table.size = new_size;
    g_lua_variable_storage.hash_table.used_count = new_used_count;
    g_lua_variable_storage.hash_table.deleted_count = 0;
    g_lua_variable_storage.hash_table.needs_rebuild = 0;
    
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

/* Forward declarations */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth);
static int lua_format_result_optimized(lua_State* L, char* output, int output_size);
static int lua_c_arma_callback(lua_State* L);
static int arma_callback_safe_call(const char* data, const char* function, char* error_output, int error_size);
static int arma_convert_lua_data_to_string(lua_State* L, int stack_index, char* output, int output_size);
static lua_single_state_t* lua_get_single_state(void);
static int lua_inject_variable(lua_State* L, lua_variable_t* var);

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
        
        /* IMMEDIATE INJECTION: Inject into current Lua state */
        lua_single_state_t* state = lua_get_single_state();
        if (state && state->L && state->initialized) {
            lua_inject_variable(state->L, existing_var);
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
    
    /* Allocate all memory first with comprehensive error handling */
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
    
    /* Add to hash table BEFORE updating the array to maintain consistency */
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
    
    /* IMMEDIATE INJECTION: Inject new variable into current Lua state */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized) {
        lua_inject_variable(state->L, &g_lua_variable_storage.variables[var_index]);
    }
    
    return 1;
}

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
    
    /* IMMEDIATE REMOVAL: Remove from current Lua state first */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized) {
        if (lua_checkstack(state->L, 2)) {
            lua_pushnil(state->L);
            lua_setglobal(state->L, name);
        }
    }
    
    /* Free memory */
    free(var->name);
    free(var->original_value);
    free(var->type);
    
    /* Remove from hash table */
    lua_hash_table_delete(g_lua_variable_storage.hash_table.entries,
                         g_lua_variable_storage.hash_table.size, name_hash);
    g_lua_variable_storage.hash_table.used_count--;
    g_lua_variable_storage.hash_table.deleted_count++;
    
    /* Use swap-with-last instead of shifting to avoid O(n) operations */
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

/* Enhanced immediate variable injection - replaces lazy loading */
static int lua_inject_all_variables(lua_State* L) {
    if (!L || !g_lua_variable_storage.initialized) return 1;
    
    if (!lua_checkstack(L, 5)) return 0;
    
    /* Inject all stored variables immediately */
    for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
        lua_variable_t* var = &g_lua_variable_storage.variables[i];
        if (var && var->name) {
            lua_inject_variable(L, var);
        }
    }
    
    return 1;
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
    lua_single_state_t* state = lua_get_single_state();
    if (!state || !state->L || !state->initialized) {
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 0;
    }
    
    lua_State* L = state->L;
    int result = 0;
    int initial_stack_top = lua_gettop(L);
    
    if (!lua_checkstack(L, 3)) {
        kh_set_error(output, output_size, "LUA STACK OVERFLOW");
        return 0;
    }
    
    /* Get the variable from Lua state */
    lua_getglobal(L, name);
    
    char* temp_buffer = (char*)malloc(output_size);
    if (!temp_buffer) {
        lua_settop(L, initial_stack_top);
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
        /* Variable not in Lua state, return stored value with type */
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
    /* Check argument count first before any stack operations */
    int argc = lua_gettop(L);
    if (argc < 3 || argc > 4) {
        /* Only push if we have space, otherwise just return */
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
        }
        return 1;
    }
    
    /* Now check if we have adequate stack space for all operations */
    if (!lua_checkstack(L, 2)) {
        /* Can't safely push anything, just return */
        return 0;
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
    /* Check argument count first */
    int argc = lua_gettop(L);
    if (argc != 1) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
        }
        return 1;
    }
    
    /* Check stack space for operations */
    if (!lua_checkstack(L, 1)) {
        return 0;
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
    
    /* IMMEDIATE CLEANUP: Clear variables from Lua state first */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized && lua_checkstack(state->L, 2)) {
        /* Clear each variable from Lua state */
        for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
            lua_variable_t* var = &g_lua_variable_storage.variables[i];
            if (var && var->name) {
                lua_pushnil(state->L);
                lua_setglobal(state->L, var->name);
            }
        }
    }
    
    /* Clear all persistent variables with proper memory management */
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
        
        /* Clear and reset hash table safely */
        if (g_lua_variable_storage.hash_table.entries && g_lua_variable_storage.hash_table.size > 0) {
            memset(g_lua_variable_storage.hash_table.entries, 0, 
                   g_lua_variable_storage.hash_table.size * sizeof(lua_var_hash_entry_t));
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
    
    /* Cache string lengths once */
    size_t name_len = strlen(name);
    size_t type_len = strlen(type);
    
    /* Parse overwrite flag (defaults to true) */
    int overwrite_flag = 1;
    if (argc >= 4 && argv[3]) {
        overwrite_flag = kh_parse_boolean(argv[3]);
    }
    
    /* Clean the name */
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    /* Cache cleaned name length */
    size_t clean_name_len = strlen(clean_name);
    
    /* Clean the type */
    char* clean_type = (char*)malloc(type_len + 1);
    if (!clean_type) {
        free(clean_name);
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(type, clean_type, (int)type_len + 1);
    
    /* Convert type to uppercase for consistency */
    for (size_t i = 0; clean_type[i]; i++) {
        if (clean_type[i] >= 'a' && clean_type[i] <= 'z') {
            clean_type[i] = clean_type[i] - 32;
        }
    }
    
    /* Validate inputs */
    if (clean_name_len == 0) {
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

/* Enhanced Lua state initialization with Lua-callable functions registration */
static int lua_init_turbo_state(lua_State* L) {
    if (!L) return 0;
    
    /* Only reserve what we actually need for this function */
    if (!lua_checkstack(L, 8)) return 0;
    
    /* Load essential libraries */
    luaopen_base(L);
    luaopen_math(L);     
    luaopen_string(L);   
    luaopen_table(L);
    luaopen_jit(L);
    luaopen_bit(L);
    
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
        
        /* Aggressive JIT optimization - more compact setup */
        lua_getfield(L, -1, "opt");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "start");
            
            /* Increase memory limits for complex traces */
            lua_pushstring(L, "maxmcode=65536");    /* More machine code memory */
            lua_pushstring(L, "maxtrace=2000");    /* More traces */
            lua_pushstring(L, "maxrecord=6000");   /* Longer traces */
            lua_pushstring(L, "maxirconst=1000");  /* More constants */
            lua_pushstring(L, "maxside=200");      /* More side exits */
            lua_pushstring(L, "maxsnap=1000");     /* More snapshots */
            
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
            
            if (lua_pcall(L, 12, 0, 0) != LUA_OK) {
                lua_pop(L, 1); /* Continue on optimization failure */
            }
        } else {
            lua_pop(L, 1);
        }
        
        lua_pop(L, 1); /* Pop jit table */
    } else {
        lua_pop(L, 1);
    }
    
    /* Remove dangerous globals for security - batch operations */
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

    lua_pushcfunction(L, lua_c_arma_callback);
    lua_setglobal(L, "ArmaCallback");
    
    /* IMMEDIATE INJECTION: Inject all stored variables into new Lua state */
    lua_inject_all_variables(L);
    
    return 1;
}

/* Simplified state getter - creates or returns the single state */
static lua_single_state_t* lua_get_single_state(void) {
    if (!g_lua_initialized) {
        memset(&g_lua_state, 0, sizeof(g_lua_state));
        g_lua_initialized = 1;
    }
    
    /* If not initialized or state is NULL, create new state */
    if (!g_lua_state.initialized || !g_lua_state.L) {
        /* Clean up any existing state first */
        if (g_lua_state.L) {
            lua_close(g_lua_state.L);
        }
        
        g_lua_state.L = luaL_newstate();
        if (!g_lua_state.L) {
            g_lua_state.initialized = 0;
            return NULL;
        }
        
        g_lua_state.jit_enabled = lua_init_turbo_state(g_lua_state.L);
        if (!g_lua_state.jit_enabled) {
            lua_close(g_lua_state.L);
            g_lua_state.L = NULL;
            g_lua_state.initialized = 0;
            return NULL;
        }
        
        g_lua_state.initialized = 1;
        g_lua_state.error_count = 0;
        g_lua_state.creation_time = GetTickCount();
    }
    
    return &g_lua_state;
}

/* Reset the Lua state completely */
static void lua_reset_single_state(void) {
    if (g_lua_state.L) {
        lua_close(g_lua_state.L);
    }
    memset(&g_lua_state, 0, sizeof(g_lua_state));
    /* g_lua_initialized remains 1 so we don't re-initialize the system */
}

/* LuaResetState operation - Reset the single Lua state */
static int kh_process_lua_reset_state_operation(char* output, int output_size, 
                                               const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc != 0) {
        kh_set_error(output, output_size, "REQUIRES 0 ARGUMENTS");
        return 1;
    }
    
    /* Reset the state */
    lua_reset_single_state();
    
    /* Get the new state and inject all variables immediately */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized) {
        /* Variables are automatically injected in lua_init_turbo_state */
    }
    
    _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\"]");
    return 0;
}

/* Cleanup function with single state management */
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
    
    /* Clean up single Lua state */
    if (g_lua_state.L) {
        lua_close(g_lua_state.L);
    }
    memset(&g_lua_state, 0, sizeof(g_lua_state));
    g_lua_initialized = 0;
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
    
    /* Reserve stack space based on expected usage */
    if (!lua_checkstack(L, 5)) return NULL;
    
    /* Skip opening bracket */
    if (*ptr != '[') return NULL;
    ptr++;
    
    /* Create new Lua table */
    lua_newtable(L);
    int table_index = lua_gettop(L);
    int element_count = 0;
    int parse_operations = 0;
    
    /* Skip whitespace */
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    
    /* Handle empty array */
    if (ptr < end && *ptr == ']') {
        return ptr + 1;
    }
    
    /* Parse array elements - removed arbitrary element limit, use parse operations limit instead */
    while (ptr < end && *ptr != ']' && parse_operations < KH_ARRAY_PARSE_OPERATIONS_LIMIT) {
        /* Skip whitespace and commas */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) {
            ptr++;
            parse_operations++;
            if (parse_operations >= KH_ARRAY_PARSE_OPERATIONS_LIMIT) break;
        }
        if (ptr >= end || *ptr == ']' || parse_operations >= KH_ARRAY_PARSE_OPERATIONS_LIMIT) break;
        
        /* Parse the value recursively */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, depth + 1);
        if (!next_ptr) {
            /* Ensure stack is restored on parsing failure */
            lua_settop(L, table_index - 1);
            return NULL;
        }
        
        /* Set table[index] = value */
        element_count++;
        lua_rawseti(L, table_index, element_count);
        ptr = next_ptr;
        parse_operations++;
    }
    
    /* Skip closing bracket */
    if (ptr < end && *ptr == ']') {
        ptr++;
    } else {
        /* Missing closing bracket or hit parse operations limit */
        lua_settop(L, table_index - 1);
        return NULL;
    }
    
    /* Check if this should be converted to a hashmap - optimize for common cases */
    if (element_count > 0) {
        int is_hashmap = 1;
        
        /* Quick check first few elements */
        int check_limit = (element_count < 5) ? element_count : 5;
        for (int i = 1; i <= check_limit && is_hashmap; i++) {
            lua_rawgeti(L, table_index, i);
            if (!lua_is_key_value_pair(L, -1)) {
                is_hashmap = 0;
            }
            lua_pop(L, 1);
        }
        
        if (is_hashmap && element_count > 5) {
            /* Check remaining elements if needed */
            for (int i = 6; i <= element_count && is_hashmap; i++) {
                lua_rawgeti(L, table_index, i);
                if (!lua_is_key_value_pair(L, -1)) {
                    is_hashmap = 0;
                }
                lua_pop(L, 1);
            }
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
    
    /* Reserve minimal stack space based on operation type */
    if (!lua_checkstack(L, 3)) return NULL;
    
    /* Skip leading whitespace */
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (ptr >= end) return NULL;
    
    /* Check what type of value we're parsing */
    if (*ptr == '[') {
        return lua_parse_arma_array(L, ptr, end, depth);
    }
    
    /* Find the end of this value with optimized bounds checking */
    const char* value_start = ptr;
    const char* value_end = ptr;
    int in_quotes = 0;
    int bracket_depth = 0;
    
    /* Pre-calculate end position to avoid repeated bounds checks */
    const char* max_end = end;
    
    while (value_end < max_end) {
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
    size_t value_len = (size_t)(value_end - value_start);
    if (value_len == 0) return NULL;
    
    /* FIXED: Consistent buffer management with standard 8KB threshold */
    char stack_buffer[8192]; /* Consistent with other functions */
    char* value_buffer = NULL;
    int allocated = 0;
    
    if (value_len < sizeof(stack_buffer)) {
        value_buffer = stack_buffer;
    } else {
        value_buffer = (char*)malloc(value_len + 1);
        if (!value_buffer) return NULL;
        allocated = 1;
    }
    
    memcpy(value_buffer, value_start, value_len);
    value_buffer[value_len] = '\0';
    
    /* Optimized trimming - work on the buffer directly */
    char* trimmed = value_buffer;
    char* trim_end = value_buffer + value_len - 1;
    
    /* Trim leading whitespace */
    while (trimmed <= trim_end && (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n')) trimmed++;
    
    /* Trim trailing whitespace */
    while (trim_end >= trimmed && (*trim_end == ' ' || *trim_end == '\t' || *trim_end == '\n')) {
        *trim_end-- = '\0';
    }
    
    if (trimmed > trim_end) {
        /* Empty after trimming */
        if (allocated) free(value_buffer);
        return NULL;
    }
    
    const char* result_ptr = value_end;
    
    /* Optimized value parsing with fewer string comparisons */
    size_t trimmed_len = (size_t)(trim_end - trimmed + 1);
    
    if (trimmed_len == 4 && memcmp(trimmed, "true", 4) == 0) {
        lua_pushboolean(L, 1);
    } else if (trimmed_len == 5 && memcmp(trimmed, "false", 5) == 0) {
        lua_pushboolean(L, 0);
    } else if (trimmed_len == 3 && memcmp(trimmed, "nil", 3) == 0) {
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
    
    /* Cleanup */
    if (allocated) free(value_buffer);
    
    return result_ptr;
}

/* Enhanced argument parsing with comprehensive error handling and proper return values */
static int lua_parse_args_optimized(lua_State* L, const char* args_str) {
    if (!args_str || !L) return -1;
    
    /* Cache string length to avoid multiple strlen calls */
    size_t args_len = strlen(args_str);
    if (args_len == 0) {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
        return 0;
    }
    
    /* Fast empty array check */
    const char* ptr = args_str;
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (!*ptr) {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
        return 0;
    }
    
    /* Find array start */
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr != '[') {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
        return 0;
    }
    ptr++;
    
    /* Skip whitespace after opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (*ptr == ']') {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
        return 0;
    }
    
    /* Ultra-fast single number path with improved bounds checking */
    if (isdigit(*ptr) || *ptr == '-' || *ptr == '+' || *ptr == '.') {
        char num_buf[32];
        int buf_pos = 0;
        const char* num_start = ptr;
        
        /* Copy number characters with bounds checking */
        while (*ptr && buf_pos < 31 && 
               (isdigit(*ptr) || *ptr == '.' || *ptr == '-' || *ptr == '+' || 
                *ptr == 'e' || *ptr == 'E')) {
            num_buf[buf_pos++] = *ptr++;
        }
        num_buf[buf_pos] = '\0';
        
        /* Check if this is the only element */
        while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
        if (*ptr == ']') {
            char* endptr;
            double num = strtod(num_buf, &endptr);
            if (*endptr == '\0' && !isnan(num) && !isinf(num)) {
                /* Reserve minimal stack for single number */
                if (!lua_checkstack(L, 2)) return -1;
                
                lua_pushnumber(L, num);
                lua_setglobal(L, "arg1");
                lua_pushinteger(L, 1);
                lua_setglobal(L, "argc");
                return 1;
            }
        }
        ptr = num_start; /* Reset for general parsing */
    }
    
    /* General parsing - reserve stack for maximum expected arguments */
    if (!lua_checkstack(L, LUA_MAX_SIMPLE_ARGS + 5)) return -1;
    
    const char* end = args_str + args_len;
    int arg_count = 0;
    
    while (ptr < end && *ptr != ']' && arg_count < LUA_MAX_SIMPLE_ARGS) {
        /* Skip separators */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
        if (ptr >= end || *ptr == ']') break;
        
        /* Parse argument */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, 0);
        if (!next_ptr) break;
        
        arg_count++;
        ptr = next_ptr;
    }
    
    /* Set globals in reverse order (stack is in reverse) */
    for (int i = arg_count; i >= 1; i--) {
        char arg_name[8];
        _snprintf_s(arg_name, sizeof(arg_name), _TRUNCATE, "arg%d", i);
        lua_setglobal(L, arg_name);
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
    if (!L || !output || output_size <= 3) return 0;
    
    if (!lua_checkstack(L, 2)) return 0;
    
    /* Simple format: just return the result value */
    size_t pos = 0;
    size_t output_limit = (size_t)output_size;
    
    /* Handle result */
    int top = lua_gettop(L);
    if (top == 0) {
        if (pos + 3 >= output_limit) return 0;
        memcpy(output + pos, "nil", 3);
        pos += 3;
    } else {
        /* Convert the top stack value to string */
        if (!lua_value_to_arma_string(L, -1, output, output_limit, &pos, 0)) {
            /* Fallback to nil on conversion failure */
            pos = 0;
            if (pos + 3 >= output_limit) return 0;
            memcpy(output + pos, "nil", 3);
            pos += 3;
        }
    }
    
    /* Null terminate */
    if (pos < output_limit) {
        output[pos] = '\0';
    } else {
        output[output_limit - 1] = '\0';
    }
    
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
    
    /* Cache string lengths to avoid multiple strlen calls */
    size_t code_len = strlen(code);
    size_t args_len = strlen(arguments);
    
    if (code_len == 0) {
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }
    
    /* Adaptive buffer management */
    char* clean_code = NULL;
    int allocated = 0;
    
    const size_t stack_threshold = 8192;
    char stack_buffer[8192];
    
    if (code_len < stack_threshold) {
        clean_code = stack_buffer;
    } else {
        clean_code = (char*)malloc(code_len + 1);
        if (!clean_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        allocated = 1;
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    size_t clean_code_len = strlen(clean_code);
    if (clean_code_len == 0) {
        if (allocated) free(clean_code);
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY AFTER CLEANING");
        return 1;
    }
    
    /* Get the single state */
    lua_single_state_t* state = lua_get_single_state();
    if (!state || !state->L) {
        if (allocated) free(clean_code);
        kh_set_error(output, output_size, "FAILED TO INITIALIZE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    int initial_stack_top = lua_gettop(L);
    int had_error = 0;
    
    /* Always clear argument globals first */
    lua_clear_argument_globals(L);
    
    /* Parse arguments if provided */
    if (args_len > 0) {
        const char* args_check = arguments;
        while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
        
        if (*args_check == '[') {
            args_check++;
            while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
            if (*args_check != ']') {
                int parsed_arg_count = lua_parse_args_optimized(L, arguments);
                if (parsed_arg_count < 0) {
                    lua_settop(L, initial_stack_top);
                    if (allocated) free(clean_code);
                    kh_set_error(output, output_size, "ARGUMENT PARSING FAILED");
                    /* Reset state on argument parsing error */
                    lua_reset_single_state();
                    return 1;
                }
            } else {
                lua_pushinteger(L, 0);
                lua_setglobal(L, "argc");
            }
        } else {
            lua_pushinteger(L, 0);
            lua_setglobal(L, "argc");
        }
    } else {
        lua_pushinteger(L, 0);
        lua_setglobal(L, "argc");
    }
    
    /* Compile the code */
    if (luaL_loadstring(L, clean_code) == LUA_OK) {
        /* Execute the code */
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            /* Success - format result */
            if (lua_format_result_optimized(L, output, output_size)) {
                result = 0; /* Success */
            } else {
                kh_set_error(output, output_size, "RESULT FORMAT FAILED");
                had_error = 1;
            }
        } else {
            /* Execution error - use kh_set_error for consistency */
            const char* error_msg = lua_tostring(L, -1);
            if (error_msg && strlen(error_msg) > 0) {
                /* Create a formatted error message with size limit */
                char formatted_error[1024];
                int format_result = _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                                               "EXECUTION: %s", error_msg);
                if (format_result > 0) {
                    kh_set_error(output, output_size, formatted_error);
                } else {
                    kh_set_error(output, output_size, "EXECUTION: ERROR MESSAGE TOO LONG");
                }
            } else {
                kh_set_error(output, output_size, "EXECUTION: UNKNOWN ERROR");
            }
            had_error = 1;
        }
    } else {
        /* Compilation error - use kh_set_error for consistency */
        const char* error_msg = lua_tostring(L, -1);
        if (error_msg && strlen(error_msg) > 0) {
            /* Create a formatted error message with size limit */
            char formatted_error[1024];
            int format_result = _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                                           "COMPILATION: %s", error_msg);
            if (format_result > 0) {
                kh_set_error(output, output_size, formatted_error);
            } else {
                kh_set_error(output, output_size, "COMPILATION: ERROR MESSAGE TOO LONG");
            }
        } else {
            kh_set_error(output, output_size, "COMPILATION: SYNTAX ERROR");
        }
        had_error = 1;
    }
    
    /* Cleanup stack */
    lua_settop(L, initial_stack_top);
    
    /* Reset state on any error to prevent corruption */
    if (had_error) {
        lua_reset_single_state();
    }
    
    if (allocated) free(clean_code);
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
    
    /* Get the single Lua state for compilation */
    lua_single_state_t* state = lua_get_single_state();
    if (!state || !state->L) {
        free(clean_code);
        kh_set_error(output, output_size, "FAILED TO INITIALIZE LUA STATE");
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
        /* Compilation failed - use kh_set_error for consistency */
        const char* error_msg = lua_tostring(L, -1);
        if (error_msg && strlen(error_msg) > 0) {
            /* Create a formatted error message with size limit */
            char formatted_error[1024];
            int format_result = _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                                           "COMPILATION: %s", error_msg);
            if (format_result > 0) {
                kh_set_error(output, output_size, formatted_error);
            } else {
                kh_set_error(output, output_size, "COMPILATION: ERROR MESSAGE TOO LONG");
            }
        } else {
            kh_set_error(output, output_size, "COMPILATION: SYNTAX ERROR");
        }
    }
    
    /* Ensure stack is properly cleaned up */
    lua_settop(L, initial_stack_top);
    
    /* Cleanup memory */
    free(clean_code);
    
    return result;
}

/* Lua C function: ArmaCallback(data, function) - High-performance implementation */
static int lua_c_arma_callback(lua_State* L) {
    /* Validate argument count first */
    int argc = lua_gettop(L);
    if (argc != 2) {
        if (lua_checkstack(L, 2)) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "REQUIRES 2 ARGUMENTS: ArmaCallback(data, function)");
        }
        return argc < 2 ? argc : 2;
    }
    
    /* Validate callback initialization */
    if (!g_callback_initialized || !g_arma_callback) {
        if (lua_checkstack(L, 2)) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "ARMA CALLBACK NOT AVAILABLE");
        }
        return 2;
    }
    
    /* Check adequate stack space for all operations */
    if (!lua_checkstack(L, 5)) {
        /* Cannot safely continue without adequate stack space */
        return 0;
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

#endif /* LUA_INTEGRATION_H */