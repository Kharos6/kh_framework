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

/* Simplified memory tracking - reduced overhead */
typedef struct pool_block_s {
    size_t size;
    struct pool_block_s* next;
} pool_block_t;

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

/* Simplified bytecode cache entry - removed deletion tracking and size limits */
typedef struct bytecode_cache_entry_s {
    uint64_t code_hash;                  
    char* bytecode;                      
    size_t bytecode_size;                
    int ref_count;                       
    DWORD last_used;                     
    struct bytecode_cache_entry_s* next; 
} bytecode_cache_entry_t;

/* Simplified string result cache entry - removed deletion tracking and size limits */
typedef struct string_cache_entry_s {
    uint64_t args_hash;                  
    char* result;                        
    size_t result_size;                  
    DWORD last_used;                     
    struct string_cache_entry_s* next;   
} string_cache_entry_t;

/* Store initial globals state for restoration - FIXED: Track allocation success */
typedef struct {
    char** initial_globals;     /* Array of initial global names */
    int* allocation_success;    /* Track which allocations succeeded */
    int global_count;          /* Number of initial globals */
    int initialized;           /* Whether initial state is captured */
} lua_initial_state_t;

/* Enhanced persistent Lua state with better error tracking and initial state management */
typedef struct {
    lua_State* L;                        
    int in_use;                          
    int initialized;                     
    int jit_enabled;                     
    void* memory_pool;                   
    size_t memory_used;                  
    size_t memory_limit;                 
    int error_count;                     
    DWORD last_error_time;               
    DWORD last_cleanup_time;
    lua_initial_state_t initial_state;  /* Track initial globals for restoration */
} lua_persistent_state_t;

/* FIXED: Simplified memory pool allocator - no automatic resets for maximum performance */
typedef struct {
    char* pool;                          
    size_t size;                         
    size_t used;                         
    size_t peak_used;                    /* Track peak usage for optimization */
    pool_block_t* free_blocks;           /* Simple free list */
    int initialized;                     
    int operation_count;                 /* Track operations for statistics */
    DWORD creation_time;                 /* When pool was created */
} lua_memory_pool_t;

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

/* Global optimization caches - single-threaded, so no locking needed */
static lua_persistent_state_t g_lua_states[LUA_STATE_POOL_SIZE] = {0};
static int g_lua_pool_initialized = 0;

static bytecode_cache_entry_t* g_bytecode_cache[LUA_BYTECODE_CACHE_SIZE] = {0};
static string_cache_entry_t* g_string_cache[LUA_STRING_CACHE_SIZE] = {0};

static lua_memory_pool_t g_memory_pool = {0};

/* Global persistent Lua variables storage with hash table */
static lua_variable_storage_t g_lua_variable_storage = {0};

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

/* Enhanced memory pool initialization with better error handling */
static int lua_init_memory_pool(void) {
    if (g_memory_pool.initialized) return 1;
    
    g_memory_pool.pool = (char*)malloc(LUA_MEMORY_POOL_SIZE);
    if (!g_memory_pool.pool) return 0;
    
    /* Initialize all fields properly */
    g_memory_pool.size = LUA_MEMORY_POOL_SIZE;
    g_memory_pool.used = 0;
    g_memory_pool.peak_used = 0;
    g_memory_pool.free_blocks = NULL;
    g_memory_pool.initialized = 1;
    g_memory_pool.operation_count = 0;
    g_memory_pool.creation_time = GetTickCount();
    
    return 1;
}

/* Persistent Lua variable management functions */

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
                /* Full hashmap merging would require implementing the merge logic here */
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
            } else {
                /* Concatenate arrays */
                size_t existing_clean_len = strlen(clean_existing);
                size_t new_clean_len = strlen(clean_new);
                
                if (existing_clean_len >= 2 && new_clean_len >= 2) {
                    if (existing_clean_len == 2 || (existing_clean_len == 3 && clean_existing[1] == ' ')) {
                        strcpy_s(temp_result, result_size, clean_new);
                    } else if (existing_clean_len + new_clean_len < result_size) {
                        _snprintf_s(temp_result, result_size, _TRUNCATE, "%.*s,%s", 
                                   (int)(existing_clean_len - 1), clean_existing, clean_new + 1);
                    }
                }
            }
            success = 1;
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
    
    /* FIXED: Allocate all memory first, then assign to avoid partial leaks */
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
    g_lua_variable_storage.variables[var_index].name = name_copy;
    g_lua_variable_storage.variables[var_index].original_value = value_copy;
    g_lua_variable_storage.variables[var_index].type = type_copy;
    g_lua_variable_storage.variables[var_index].lua_type = var_type;
    g_lua_variable_storage.variable_count++;
    
    /* Add to hash table - now that we've already rebuilt if necessary */
    uint32_t name_hash = lua_hash_variable_name(name);
    if (!lua_hash_table_insert_robin_hood(g_lua_variable_storage.hash_table.entries,
                                         g_lua_variable_storage.hash_table.size, 
                                         name_hash, var_index)) {
        /* Hash table insertion failed - rollback the variable addition */
        g_lua_variable_storage.variable_count--;
        free(name_copy);
        free(value_copy);
        free(type_copy);
        memset(&g_lua_variable_storage.variables[var_index], 0, sizeof(lua_variable_t));
        return 0;
    }
    
    g_lua_variable_storage.hash_table.used_count++;
    return 1;
}

/* Forward declaration for recursive parsing with depth limiting */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth);
static void lua_release_optimized_state(lua_persistent_state_t* state);
static lua_persistent_state_t* lua_get_optimized_state(void);

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
    
    /* Remove from hash table */
    lua_hash_table_delete(g_lua_variable_storage.hash_table.entries,
                         g_lua_variable_storage.hash_table.size, name_hash);
    g_lua_variable_storage.hash_table.used_count--;
    g_lua_variable_storage.hash_table.deleted_count++;
    
    /* Shift array elements */
    for (uint32_t i = var_index; i < g_lua_variable_storage.variable_count - 1; i++) {
        g_lua_variable_storage.variables[i] = g_lua_variable_storage.variables[i + 1];
    }
    
    /* Clear last element */
    memset(&g_lua_variable_storage.variables[g_lua_variable_storage.variable_count - 1], 0, sizeof(lua_variable_t));
    g_lua_variable_storage.variable_count--;
    
    /* Update hash table indices after array shift */
    for (uint32_t i = 0; i < g_lua_variable_storage.hash_table.size; i++) {
        if (g_lua_variable_storage.hash_table.entries[i].name_hash != LUA_VAR_HASH_EMPTY && 
            !g_lua_variable_storage.hash_table.entries[i].deleted) {
            if (g_lua_variable_storage.hash_table.entries[i].var_index > var_index) {
                g_lua_variable_storage.hash_table.entries[i].var_index--;
            }
        }
    }
    
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

/* Inject all stored variables into a Lua state */
static void lua_inject_all_variables(lua_State* L) {
    if (!L || !g_lua_variable_storage.initialized) return;
    
    for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
        lua_inject_variable(L, &g_lua_variable_storage.variables[i]);
    }
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
    lua_persistent_state_t* state = lua_get_optimized_state();
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
    
    /* Clear all persistent variables and track freed memory */
    if (g_lua_variable_storage.initialized) {
        cleared_count = g_lua_variable_storage.variable_count;
        
        /* Free all variable data and calculate freed memory */
        for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
            if (g_lua_variable_storage.variables[i].name) {
                memory_freed += strlen(g_lua_variable_storage.variables[i].name) + 1;
                free(g_lua_variable_storage.variables[i].name);
                g_lua_variable_storage.variables[i].name = NULL;
            }
            if (g_lua_variable_storage.variables[i].original_value) {
                memory_freed += strlen(g_lua_variable_storage.variables[i].original_value) + 1;
                free(g_lua_variable_storage.variables[i].original_value);
                g_lua_variable_storage.variables[i].original_value = NULL;
            }
            if (g_lua_variable_storage.variables[i].type) {
                memory_freed += strlen(g_lua_variable_storage.variables[i].type) + 1;
                free(g_lua_variable_storage.variables[i].type);
                g_lua_variable_storage.variables[i].type = NULL;
            }
        }
        
        /* Reset counters */
        g_lua_variable_storage.variable_count = 0;
        
        /* Clear and reset hash table efficiently */
        if (g_lua_variable_storage.hash_table.entries) {
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

/* NEW: LuaClearFunctions operation - Clear all cached functions/bytecode */
static int kh_process_lua_clear_functions_operation(char* output, int output_size, 
                                                   const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc != 0) {
        kh_set_error(output, output_size, "REQUIRES 0 ARGUMENTS");
        return 1;
    }
    
    int bytecode_cleared = 0;
    int string_cleared = 0;
    size_t pool_memory_freed = 0;
    
    /* Count and clear bytecode cache entries - optimized with single loop */
    for (int i = 0; i < LUA_BYTECODE_CACHE_SIZE; i++) {
        if (g_bytecode_cache[i] != NULL) {
            bytecode_cleared++;
        }
    }
    
    /* Count and clear string result cache entries - optimized with single loop */
    for (int i = 0; i < LUA_STRING_CACHE_SIZE; i++) {
        if (g_string_cache[i] != NULL) {
            string_cleared++;
        }
    }
    
    /* Clear caches efficiently with memset */
    memset(g_bytecode_cache, 0, sizeof(g_bytecode_cache));
    memset(g_string_cache, 0, sizeof(g_string_cache));
    
    /* Reset memory pool to beginning (this effectively "frees" all pool allocations) */
    if (g_memory_pool.initialized) {
        pool_memory_freed = g_memory_pool.used;
        g_memory_pool.used = 0;
        g_memory_pool.operation_count = 0;
        /* Keep peak_used and creation_time for monitoring/debugging */
    }
    
    _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\",%d,%d,%zu]", 
               bytecode_cleared, string_cleared, pool_memory_freed);
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

/* FIXED: Ultra-fast pool allocator - no resets, maximum performance */
static void* lua_pool_alloc(size_t size) {
    if (!g_memory_pool.initialized && !lua_init_memory_pool()) {
        return malloc(size); /* Fallback to system malloc */
    }
    
    /* Align to POOL_ALIGNMENT-byte boundary for performance */
    size = (size + LUA_POOL_ALIGNMENT - 1) & ~(LUA_POOL_ALIGNMENT - 1);
    
    /* Try to allocate from pool first */
    if (g_memory_pool.used + size <= g_memory_pool.size) {
        void* ptr = g_memory_pool.pool + g_memory_pool.used;
        g_memory_pool.used += size;
        g_memory_pool.operation_count++;
        
        /* Update peak usage tracking */
        if (g_memory_pool.used > g_memory_pool.peak_used) {
            g_memory_pool.peak_used = g_memory_pool.used;
        }
        
        return ptr;
    }
    
    /* Pool exhausted, fallback to system malloc - this is fine for performance */
    return malloc(size);
}

/* Simplified check for pool allocation */
static int lua_is_pool_allocation(void* ptr) {
    if (!ptr || !g_memory_pool.initialized || !g_memory_pool.pool) return 0;
    
    /* Check if pointer is within pool bounds AND pool is actually initialized */
    if (g_memory_pool.size == 0) return 0;
    
    uintptr_t pool_start = (uintptr_t)g_memory_pool.pool;
    uintptr_t pool_end = pool_start + g_memory_pool.size;
    uintptr_t ptr_addr = (uintptr_t)ptr;
    
    return (ptr_addr >= pool_start && ptr_addr < pool_end) ? 1 : 0;
}

/* Enhanced print function for security */
static int lua_secure_print_fast(lua_State* L) {
    /* Consume all arguments but produce no output for security */
    int n = lua_gettop(L);
    lua_pop(L, n);
    return 0;
}

/* Enhanced custom Lua allocator with proper reallocation handling */
static void* lua_custom_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud; /* Unused parameter */
    
    if (nsize == 0) {
        /* Free operation */
        if (ptr && !lua_is_pool_allocation(ptr)) {
            free(ptr); /* Only free system malloc allocations */
        }
        /* Pool allocations are not individually freed */
        return NULL;
    } else if (ptr == NULL) {
        /* Allocation */
        return lua_pool_alloc(nsize);
    } else {
        /* Reallocation - Proper Lua allocator contract compliance */
        void* new_ptr = lua_pool_alloc(nsize);
        if (new_ptr) {
            /* Success - copy old data and free old pointer */
            if (osize > 0) {
                size_t copy_size = osize < nsize ? osize : nsize;
                memcpy(new_ptr, ptr, copy_size);
            }
            
            /* Free old pointer if not from pool */
            if (!lua_is_pool_allocation(ptr)) {
                free(ptr);
            }
            
            return new_ptr;
        } else {
            /* Allocation failed - return NULL as per Lua contract */
            return NULL;
        }
    }
}

/* Simplified bytecode cache operations - unlimited caching */
static bytecode_cache_entry_t* lua_find_cached_bytecode(uint64_t hash) {
    if (!hash) return NULL;
    
    int slot = (int)(hash % LUA_BYTECODE_CACHE_SIZE);
    bytecode_cache_entry_t* entry = g_bytecode_cache[slot];
    
    while (entry) {
        if (entry->code_hash == hash && entry->bytecode) {
            entry->ref_count++;
            entry->last_used = GetTickCount();
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/* Simplified reference management - no deletion marking needed */
static void lua_release_cached_bytecode(bytecode_cache_entry_t* entry) {
    if (!entry) return;
    entry->ref_count--;
    /* Note: We never delete cached bytecode entries for maximum performance */
}

/* FIXED: High-performance bytecode caching using pool allocation with proper error handling */
static int lua_cache_bytecode(uint64_t hash, const char* bytecode, size_t size) {
    if (!bytecode || size == 0 || !hash) {
        return 0;
    }
    
    int slot = (int)(hash % LUA_BYTECODE_CACHE_SIZE);
    
    /* Check if already exists to avoid duplicates */
    bytecode_cache_entry_t* existing = g_bytecode_cache[slot];
    while (existing) {
        if (existing->code_hash == hash) {
            return 1; /* Already cached */
        }
        existing = existing->next;
    }
    
    /* FIXED: Allocate bytecode first, then entry to avoid waste on failure */
    char* cached_bytecode = (char*)lua_pool_alloc(size);
    if (!cached_bytecode) return 0;
    
    bytecode_cache_entry_t* new_entry = (bytecode_cache_entry_t*)lua_pool_alloc(sizeof(bytecode_cache_entry_t));
    if (!new_entry) return 0; /* Bytecode already allocated but can't be freed - acceptable in pool system */
    
    /* Initialize all fields */
    memset(new_entry, 0, sizeof(bytecode_cache_entry_t));
    
    memcpy(cached_bytecode, bytecode, size);
    new_entry->bytecode = cached_bytecode;
    new_entry->code_hash = hash;
    new_entry->bytecode_size = size;
    new_entry->ref_count = 1;
    new_entry->last_used = GetTickCount();
    new_entry->next = g_bytecode_cache[slot];
    g_bytecode_cache[slot] = new_entry;
    
    return 1;
}

/* Simplified string result cache operations - unlimited caching */
static char* lua_find_cached_result(uint64_t hash) {
    if (!hash) return NULL;
    
    int slot = (int)(hash % LUA_STRING_CACHE_SIZE);
    string_cache_entry_t* entry = g_string_cache[slot];
    
    while (entry) {
        if (entry->args_hash == hash && entry->result) {
            entry->last_used = GetTickCount();
            return entry->result;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/* FIXED: High-performance result caching using pool allocation with proper error handling */
static int lua_cache_result(uint64_t hash, const char* result) {
    if (!result || !hash) return 0;
    
    size_t result_len = strlen(result);
    if (result_len == 0) return 0;
    
    int slot = (int)(hash % LUA_STRING_CACHE_SIZE);
    
    /* Check if already exists to avoid duplicates */
    string_cache_entry_t* existing = g_string_cache[slot];
    while (existing) {
        if (existing->args_hash == hash) {
            return 1; /* Already cached */
        }
        existing = existing->next;
    }
    
    /* FIXED: Allocate result first, then entry to avoid waste on failure */
    char* cached_result = (char*)lua_pool_alloc(result_len + 1);
    if (!cached_result) return 0;
    
    string_cache_entry_t* new_entry = (string_cache_entry_t*)lua_pool_alloc(sizeof(string_cache_entry_t));
    if (!new_entry) return 0; /* Result already allocated but can't be freed - acceptable in pool system */
    
    /* Initialize all fields */
    memset(new_entry, 0, sizeof(string_cache_entry_t));
    
    strcpy_s(cached_result, result_len + 1, result);
    new_entry->result = cached_result;
    new_entry->args_hash = hash;
    new_entry->result_size = result_len;
    new_entry->last_used = GetTickCount();
    new_entry->next = g_string_cache[slot];
    g_string_cache[slot] = new_entry;
    
    return 1;
}

/* FIXED: Capture initial global state after Lua initialization with proper allocation tracking */
static int lua_capture_initial_globals(lua_State* L, lua_initial_state_t* initial_state) {
    if (!L || !initial_state || !lua_checkstack(L, 3)) return 0;
    
    if (initial_state->initialized) return 1; /* Already captured */
    
    /* Count globals first */
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    
    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            count++;
        }
        lua_pop(L, 1);
    }
    
    if (count == 0) {
        lua_pop(L, 1);
        return 0;
    }
    
    /* FIXED: Allocate both arrays - use single allocation to avoid partial allocation failures */
    size_t total_size = (count * sizeof(char*)) + (count * sizeof(int));
    void* memory_block = malloc(total_size);
    if (!memory_block) {
        lua_pop(L, 1);
        return 0;
    }
    
    initial_state->initial_globals = (char**)memory_block;
    initial_state->allocation_success = (int*)((char*)memory_block + (count * sizeof(char*)));
    
    /* Initialize success tracking array */
    memset(initial_state->allocation_success, 0, count * sizeof(int));
    
    /* Capture global names with proper tracking */
    int index = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0 && index < count) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);
            if (key) {
                size_t key_len = strlen(key);
                initial_state->initial_globals[index] = (char*)malloc(key_len + 1);
                if (initial_state->initial_globals[index]) {
                    strcpy_s(initial_state->initial_globals[index], key_len + 1, key);
                    initial_state->allocation_success[index] = 1; /* Mark as successfully allocated */
                } else {
                    initial_state->allocation_success[index] = 0; /* Mark as failed */
                }
                index++;
            }
        }
        lua_pop(L, 1);
    }
    
    lua_pop(L, 1); /* Remove global table */
    
    initial_state->global_count = index;
    initial_state->initialized = 1;
    
    return 1;
}

/* Restore globals to initial state (remove any user-added globals) */
static void lua_restore_initial_globals(lua_State* L, lua_initial_state_t* initial_state) {
    if (!L || !initial_state || !initial_state->initialized || !lua_checkstack(L, 4)) return;
    
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    
    /* Create set of initial globals for fast lookup */
    lua_newtable(L); /* initial_set */
    int initial_set_index = lua_gettop(L);
    
    /* FIXED: Only add successfully allocated globals to the set */
    for (int i = 0; i < initial_state->global_count; i++) {
        if (initial_state->allocation_success[i] && initial_state->initial_globals[i]) {
            lua_pushstring(L, initial_state->initial_globals[i]);
            lua_pushboolean(L, 1);
            lua_settable(L, initial_set_index);
        }
    }
    
    /* Collect globals to remove */
    lua_newtable(L); /* to_remove */
    int to_remove_index = lua_gettop(L);
    int remove_count = 0;
    
    lua_pushnil(L);
    while (lua_next(L, -4) != 0) { /* Iterate global table */
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);
            if (key) {
                /* Check if this global was in initial state */
                lua_pushstring(L, key);
                lua_gettable(L, initial_set_index);
                int was_initial = lua_toboolean(L, -1);
                lua_pop(L, 1);
                
                if (!was_initial) {
                    /* This is a user-added global, mark for removal */
                    lua_pushvalue(L, -2); /* Duplicate key */
                    lua_rawseti(L, to_remove_index, ++remove_count);
                }
            }
        }
        lua_pop(L, 1);
    }
    
    /* Remove user-added globals */
    for (int i = 1; i <= remove_count; i++) {
        lua_rawgeti(L, to_remove_index, i);
        lua_pushnil(L);
        lua_rawset(L, -5); /* global_table[key] = nil */
    }
    
    lua_pop(L, 3); /* Remove to_remove, initial_set, and global_table */
}

/* FIXED: Enhanced Lua state initialization with Lua-callable functions registration */
static int lua_init_turbo_state(lua_State* L) {
    if (!L) return 0;
    
    /* Ensure we have enough stack space for operations */
    if (!lua_checkstack(L, LUA_STACK_SAFETY_MARGIN)) return 0;
    
    /* Load only essential libraries for maximum performance */
    luaopen_base(L);
    luaopen_math(L);     
    luaopen_string(L);   
    luaopen_table(L);    
    
    /* Configure LuaJIT for maximum performance with error checking */
    lua_getglobal(L, "jit");
    if (!lua_isnil(L, -1)) {
        /* Enable JIT with aggressive optimization */
        lua_getfield(L, -1, "opt");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "start");
            lua_pushinteger(L, 3);    /* Maximum optimization level */
            if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
                lua_pop(L, 2); /* Pop error and jit table */
                return 0;
            }
        } else {
            lua_pop(L, 1);
        }
        
        /* Configure JIT parameters */
        lua_getfield(L, -1, "config");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, "hotloop=1");      
            lua_pushstring(L, "hotexit=1");      
            lua_pushstring(L, "tryside=1");      
            lua_pushstring(L, "maxmcode=64");    
            lua_pushstring(L, "maxirconst=1000"); 
            if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
                lua_pop(L, 2); /* Pop error and jit table */
                return 0;
            }
        } else {
            lua_pop(L, 1);
        }
        
        lua_pop(L, 1); /* Pop jit table */
    } else {
        lua_pop(L, 1); /* Pop nil */
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
    
    /* FIXED: Register custom persistent variable functions */
    lua_pushcfunction(L, lua_c_get_persistent_variable);
    lua_setglobal(L, "GetPersistentVariable");
    
    lua_pushcfunction(L, lua_c_set_persistent_variable);
    lua_setglobal(L, "SetPersistentVariable");
    
    lua_pushcfunction(L, lua_c_delete_persistent_variable);
    lua_setglobal(L, "DeletePersistentVariable");
    
    return 1;
}

/* Enhanced state management with health monitoring and initial state capture */
static lua_persistent_state_t* lua_get_optimized_state(void) {
    if (!g_lua_pool_initialized) {
        memset(g_lua_states, 0, sizeof(g_lua_states));
        g_lua_pool_initialized = 1;
    }
    
    /* Find available state, preferring healthy states */
    lua_persistent_state_t* best_state = NULL;
    
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        lua_persistent_state_t* state = &g_lua_states[i];
        
        if (!state->in_use) {
            if (!state->initialized) {
                /* Create new state */
                state->L = lua_newstate(lua_custom_allocator, NULL);
                if (!state->L) continue;
                
                state->jit_enabled = lua_init_turbo_state(state->L);
                if (!state->jit_enabled) {
                    lua_close(state->L);
                    state->L = NULL;
                    continue;
                }
                
                /* Capture initial global state after initialization */
                if (!lua_capture_initial_globals(state->L, &state->initial_state)) {
                    lua_close(state->L);
                    state->L = NULL;
                    continue;
                }
                
                state->initialized = 1;
                state->memory_used = 0;
                state->memory_limit = LUA_MEMORY_POOL_SIZE / LUA_STATE_POOL_SIZE;
                state->error_count = 0;
                state->last_error_time = 0;
                state->last_cleanup_time = GetTickCount();
            }
            
            /* Prefer states with fewer errors */
            if (!best_state || state->error_count < best_state->error_count) {
                best_state = state;
            }
        }
    }
    
    if (best_state) {
        best_state->in_use = 1;
        
        /* Inject all persistent variables into this state */
        lua_inject_all_variables(best_state->L);
        
        return best_state;
    }
    
    return NULL; /* No available states */
}

/* Enhanced argument clearing with better error handling */
static void lua_clear_argument_globals(lua_State* L) {
    if (!L || !lua_checkstack(L, 2)) return;
    
    /* Get previous argc for safe cleanup */
    lua_getglobal(L, "argc");
    int prev_argc = 0;
    if (lua_isnumber(L, -1)) {
        prev_argc = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    
    /* Clear up to maximum safe number of arguments */
    int max_clear = (prev_argc > 0 && prev_argc <= LUA_MAX_SIMPLE_ARGS) ? prev_argc : LUA_MAX_SIMPLE_ARGS;
    
    for (int i = 1; i <= max_clear; i++) {
        char arg_name[16];
        if (_snprintf_s(arg_name, sizeof(arg_name), _TRUNCATE, "arg%d", i) > 0) {
            lua_pushnil(L);
            lua_setglobal(L, arg_name);
        }
    }
    
    lua_pushnil(L);
    lua_setglobal(L, "argc");
}

/* Enhanced state release with proper global restoration */
static void lua_release_optimized_state(lua_persistent_state_t* state) {
    if (!state || !state->L) return;
    
    /* Enhanced cleanup with error checking */
    int stack_top = lua_gettop(state->L);
    if (stack_top > 0) {
        lua_settop(state->L, 0); /* Clear stack */
    }
    
    /* Restore to initial global state instead of manual clearing */
    lua_restore_initial_globals(state->L, &state->initial_state);
    
    /* Clear argument globals */
    lua_clear_argument_globals(state->L);
    
    /* Run GC more frequently to clean up immediately */
    lua_gc(state->L, LUA_GCCOLLECT, 0);
    
    /* Update cleanup time */
    state->last_cleanup_time = GetTickCount();
    
    /* Reset state if it has too many errors */
    if (state->error_count > 10) {
        /* Clean up initial state before destroying */
        if (state->initial_state.initialized) {
            if (state->initial_state.initial_globals && state->initial_state.allocation_success) {
                for (int i = 0; i < state->initial_state.global_count; i++) {
                    if (state->initial_state.allocation_success[i]) {
                        free(state->initial_state.initial_globals[i]);
                    }
                }
            }
            /* FIXED: Free the combined allocation block */
            free(state->initial_state.initial_globals);
            state->initial_state.initial_globals = NULL;
            state->initial_state.allocation_success = NULL;
            state->initial_state.global_count = 0;
            state->initial_state.initialized = 0;
        }
        
        lua_close(state->L);
        state->L = NULL;
        state->initialized = 0;
        state->error_count = 0;
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
        
        /* Parse this argument recursively */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, 0);
        if (!next_ptr) {
            /* FIXED: Clean up any globals we set before the failure */
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
            /* FIXED: If naming failed, clean up what we've set so far */
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
    
    /* FIXED: More precise stack space calculation */
    int required_stack = 3 + (depth < 10 ? 2 : 1); /* Less aggressive stack reservation */
    if (!lua_checkstack(L, required_stack)) return 0;
    
    int type = lua_type(L, stack_index);
    
    switch (type) {
        case LUA_TNIL:
            if (*pos + 3 < buffer_size) {
                memcpy(buffer + *pos, "nil", 3);
                *pos += 3;
                return 1;
            }
            break;
            
        case LUA_TBOOLEAN: {
            const char* bool_str = lua_toboolean(L, stack_index) ? "true" : "false";
            size_t bool_len = strlen(bool_str);
            if (*pos + bool_len < buffer_size) {
                memcpy(buffer + *pos, bool_str, bool_len);
                *pos += bool_len;
                return 1;
            }
            break;
        }
        
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, stack_index);
            
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
            const char* str = lua_tostring(L, stack_index);
            if (!str) return 0;
            
            size_t str_len = strlen(str);
            if (*pos + str_len + 2 < buffer_size) {
                buffer[(*pos)++] = '"';
                
                /* Copy string, escaping internal quotes */
                for (size_t i = 0; i < str_len && *pos < buffer_size - 2; i++) {
                    if (str[i] == '"') {
                        if (*pos < buffer_size - 3) {
                            buffer[(*pos)++] = '"';
                            buffer[(*pos)++] = '"';
                        } else {
                            break;
                        }
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
            return lua_table_to_arma_recursive(L, stack_index, buffer, buffer_size, pos, depth + 1);
            
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
static int lua_table_to_arma_recursive(lua_State* L, int stack_index, char* buffer, 
                                      size_t buffer_size, size_t* pos, int depth) {
    if (*pos >= buffer_size - 2 || depth > LUA_MAX_RECURSION_DEPTH) return 0;
    
    if (!lua_checkstack(L, 3)) return 0;
    
    buffer[(*pos)++] = '[';
    
    if (lua_is_array(L, stack_index)) {
        /* Convert as array */
        int array_length = (int)lua_rawlen(L, stack_index);
        
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
            
            lua_rawgeti(L, stack_index, i);
            int success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            lua_pop(L, 1);
            
            if (!success) return 0;
        }
    } else {
        /* Convert as hashmap */
        lua_pushnil(L);
        int first_pair = 1;
        int pair_count = 0;
        const int max_pairs = 100; /* Limit hashmap size */
        
        while (lua_next(L, stack_index) != 0 && pair_count < max_pairs) {
            if (!first_pair) {
                if (*pos + 1 < buffer_size) {
                    buffer[(*pos)++] = ',';
                } else {
                    lua_pop(L, 2);
                    return 0;
                }
            }
            first_pair = 0;
            pair_count++;
            
            /* Start key-value pair array */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = '[';
            } else {
                lua_pop(L, 2);
                return 0;
            }
            
            /* Add key */
            int key_success = lua_value_to_arma_string(L, -2, buffer, buffer_size, pos, depth);
            if (!key_success) {
                lua_pop(L, 2);
                return 0;
            }
            
            /* Add comma */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = ',';
            } else {
                lua_pop(L, 2);
                return 0;
            }
            
            /* Add value */
            int value_success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            if (!value_success) {
                lua_pop(L, 2);
                return 0;
            }
            
            /* End key-value pair array */
            if (*pos + 1 < buffer_size) {
                buffer[(*pos)++] = ']';
            } else {
                lua_pop(L, 2);
                return 0;
            }
            
            lua_pop(L, 1);
        }
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
    
    if (!lua_checkstack(L, 1)) return 0;
    
    /* Always start with opening bracket */
    output[0] = '[';
    size_t pos = 1;
    
    int top = lua_gettop(L);
    if (top == 0) {
        /* No results, output [nil] */
        if (pos + 4 < (size_t)output_size) {
            strcpy_s(output + pos, output_size - pos, "nil]");
            return 1;
        }
        return 0;
    }
    
    /* Handle the top result */
    if (!lua_value_to_arma_string(L, -1, output, output_size, &pos, 0)) {
        /* Fallback for conversion failure */
        const char* fallback = "nil";
        size_t fallback_len = strlen(fallback);
        if (pos + fallback_len + 1 < (size_t)output_size) {
            memcpy(output + pos, fallback, fallback_len);
            pos += fallback_len;
        } else {
            return 0;
        }
    }
    
    /* Close the array */
    if (pos < (size_t)output_size) {
        output[pos] = ']';
        output[pos + 1] = '\0';
        return 1;
    }
    
    return 0;
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
    
    /* Get input lengths */
    size_t args_len = strlen(arguments);
    size_t code_len = strlen(code);
    
    /* Create combined hash for result caching - STACK/POOL ALLOCATION */
    size_t combined_size = args_len + code_len + 2;
    char stack_combined[2048];
    char* combined_input = NULL;

    if (combined_size <= sizeof(stack_combined)) {
        combined_input = stack_combined;
    } else {
        combined_input = (char*)lua_pool_alloc(combined_size);
        if (!combined_input) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
    }
    
    int combined_len = _snprintf_s(combined_input, combined_size, _TRUNCATE, 
                                  "%s|%s", arguments, code);
    
    uint64_t combined_hash = 0;
    if (combined_len > 0) {
        combined_hash = lua_hash_fnv1a(combined_input, combined_len);
        
        /* Check result cache first */
        char* cached_result = lua_find_cached_result(combined_hash);
        if (cached_result) {
            strncpy_s(output, output_size, cached_result, _TRUNCATE);
            /* Cleanup - check if allocations need freeing */
            if (combined_input != stack_combined && !lua_is_pool_allocation(combined_input)) {
                free(combined_input);
            }
            return 0;
        }
    }
    
    /* Clean the code string - STACK/POOL ALLOCATION */
    char stack_code[1024];
    char* clean_code = NULL;

    if (code_len + 1 <= sizeof(stack_code)) {
        clean_code = stack_code;
    } else {
        clean_code = (char*)lua_pool_alloc(code_len + 1);
        if (!clean_code) {
            /* Cleanup - check if allocations need freeing */
            if (combined_input != stack_combined && !lua_is_pool_allocation(combined_input)) {
                free(combined_input);
            }
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    /* Hash the cleaned code for bytecode caching */
    uint64_t code_hash = lua_hash_fnv1a(clean_code, strlen(clean_code));
    
    /* Check bytecode cache */
    bytecode_cache_entry_t* cached_bytecode = lua_find_cached_bytecode(code_hash);
    
    /* Get optimized Lua state */
    lua_persistent_state_t* state = lua_get_optimized_state();
    if (!state) {
        /* Cleanup - check if allocations need freeing */
        if (combined_input != stack_combined && !lua_is_pool_allocation(combined_input)) {
            free(combined_input);
        }
        if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
            free(clean_code);
        }
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    
    /* Track initial stack position for cleanup */
    int initial_stack_top = lua_gettop(L);
    
    /* Parse arguments and check for errors */
    int arg_parse_result = lua_parse_args_optimized(L, arguments);
    if (arg_parse_result < 0) {
        /* Argument parsing failed */
        if (cached_bytecode) lua_release_cached_bytecode(cached_bytecode);
        lua_settop(L, initial_stack_top);
        lua_release_optimized_state(state);
        /* Cleanup - check if allocations need freeing */
        if (combined_input != stack_combined && !lua_is_pool_allocation(combined_input)) {
            free(combined_input);
        }
        if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
            free(clean_code);
        }
        kh_set_error(output, output_size, "ARGUMENT PARSING FAILED");
        return 1;
    }
    
    /* Compile or load from cache */
    int compile_result;
    if (cached_bytecode) {
        /* Load from bytecode cache */
        compile_result = luaL_loadbuffer(L, cached_bytecode->bytecode, 
                                        cached_bytecode->bytecode_size, "cached");
    } else {
        /* Compile and potentially cache */
        compile_result = luaL_loadstring(L, clean_code);
        
        if (compile_result == LUA_OK) {
            /* Simplified bytecode extraction with better error handling */
            int bytecode_extraction_success = 0;
            int stack_before_extraction = lua_gettop(L);
            
            /* Try to extract bytecode for caching */
            lua_pushvalue(L, -1); /* Duplicate function for dumping */
            
            lua_getglobal(L, "string");
            if (!lua_isnil(L, -1)) {
                lua_getfield(L, -1, "dump");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, -3); /* The function */
                    if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                        size_t bytecode_size;
                        const char* bytecode = lua_tolstring(L, -1, &bytecode_size);
                        if (bytecode && bytecode_size > 0) {
                            if (lua_cache_bytecode(code_hash, bytecode, bytecode_size)) {
                                bytecode_extraction_success = 1;
                            }
                        }
                    }
                }
            }
            
            /* Restore stack to known state regardless of extraction outcome */
            lua_settop(L, stack_before_extraction);
        }
    }
    
    if (compile_result == LUA_OK) {
        /* Execute with error handling */
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            if (lua_format_result_optimized(L, output, output_size)) {
                /* Cache the result for future use */
                if (combined_len > 0) {
                    if (!lua_cache_result(combined_hash, output)) {
                        /* Cache failed, but result is still valid */
                    }
                }
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
    
    /* Release cached bytecode reference properly */
    if (cached_bytecode) {
        lua_release_cached_bytecode(cached_bytecode);
    }
    
    /* Ensure stack is properly cleaned up */
    lua_settop(L, initial_stack_top);
    
    /* Cleanup */
    lua_release_optimized_state(state);
    /* Cleanup - check if allocations need freeing */
    if (combined_input != stack_combined && !lua_is_pool_allocation(combined_input)) {
        free(combined_input);
    }
    if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
        free(clean_code);
    }
    
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

/* Enhanced compile operation with better error handling */
static int kh_process_lua_compile_operation(char* output, int output_size, 
                                          const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 1 || !argv || !argv[0]) {
        kh_set_error(output, output_size, "REQUIRES 1 ARGUMENT: [CODE]");
        return 1;
    }
    
    const char* code = argv[0];
    size_t code_len = strlen(code);
    
    if (code_len == 0) {
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }
    
    /* Clean the code string - STACK/POOL ALLOCATION */
    char stack_code[1024];
    char* clean_code = NULL;

    if (code_len + 1 <= sizeof(stack_code)) {
        clean_code = stack_code;
    } else {
        clean_code = (char*)lua_pool_alloc(code_len + 1);
        if (!clean_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    /* Hash the cleaned code */
    uint64_t code_hash = lua_hash_fnv1a(clean_code, strlen(clean_code));
    
    /* Check if already cached */
    bytecode_cache_entry_t* existing = lua_find_cached_bytecode(code_hash);
    if (existing) {
        /* Release the reference properly */
        lua_release_cached_bytecode(existing);
        _snprintf_s(output, output_size, _TRUNCATE, "[\"ALREADY_CACHED\"]");
        /* Cleanup - check if allocations need freeing */
        if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
            free(clean_code);
        }
        return 0;
    }
    
    /* Get a Lua state for compilation */
    lua_persistent_state_t* state = lua_get_optimized_state();
    if (!state) {
        /* Cleanup - check if allocations need freeing */
        if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
            free(clean_code);
        }
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    
    /* Track initial stack position */
    int initial_stack_top = lua_gettop(L);
    
    /* Compile the code */
    int compile_result = luaL_loadstring(L, clean_code);
    
    if (compile_result == LUA_OK) {
        /* Simplified bytecode extraction with better stack management */
        int extraction_success = 0;
        int stack_before_extraction = lua_gettop(L);
        
        lua_getglobal(L, "string");
        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "dump");
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, -3); /* The compiled function */
                if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                    size_t bytecode_size;
                    const char* bytecode = lua_tolstring(L, -1, &bytecode_size);
                    if (bytecode && bytecode_size > 0) {
                        if (lua_cache_bytecode(code_hash, bytecode, bytecode_size)) {
                            _snprintf_s(output, output_size, _TRUNCATE, 
                                       "[\"COMPILED_AND_CACHED\",%.0f,%zu]", 
                                       (double)code_hash, bytecode_size);
                            extraction_success = 1;
                            result = 0;
                        }
                    }
                }
            }
        }
        
        /* Restore stack to known state */
        lua_settop(L, stack_before_extraction);
        
        if (!extraction_success) {
            kh_set_error(output, output_size, "BYTECODE EXTRACTION FAILED");
        }
    } else {
        const char* error_msg = lua_tostring(L, -1);
        _snprintf_s(output, output_size, _TRUNCATE, 
                   KH_ERROR_PREFIX "COMPILATION: %s", 
                   error_msg ? error_msg : "syntax error");
    }
    
    /* Ensure proper stack cleanup */
    lua_settop(L, initial_stack_top);
    
    /* Cleanup */
    lua_release_optimized_state(state);
    /* Cleanup - check if allocations need freeing */
    if (clean_code != stack_code && !lua_is_pool_allocation(clean_code)) {
        free(clean_code);
    }
    
    return result;
}

/* FIXED: Cleanup function with proper memory management */
static void kh_cleanup_lua_states(void) {
    /* Clean up persistent Lua variables with hash table */
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
    
    /* Clean up bytecode cache - safe to clean since we're shutting down */
    for (int i = 0; i < LUA_BYTECODE_CACHE_SIZE; i++) {
        bytecode_cache_entry_t* entry = g_bytecode_cache[i];
        while (entry) {
            bytecode_cache_entry_t* next = entry->next;
            /* These might be pool allocations, but that's OK since we're cleaning the pool too */
            entry = next;
        }
        g_bytecode_cache[i] = NULL;
    }
    
    /* Clean up string cache - safe to clean since we're shutting down */
    for (int i = 0; i < LUA_STRING_CACHE_SIZE; i++) {
        string_cache_entry_t* entry = g_string_cache[i];
        while (entry) {
            string_cache_entry_t* next = entry->next;
            /* These might be pool allocations, but that's OK since we're cleaning the pool too */
            entry = next;
        }
        g_string_cache[i] = NULL;
    }
    
    /* FIXED: Clean up Lua states and their initial state data properly */
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        if (g_lua_states[i].L) {
            /* FIXED: Clean up initial state with proper pointer checking */
            if (g_lua_states[i].initial_state.initialized) {
                if (g_lua_states[i].initial_state.initial_globals && 
                    g_lua_states[i].initial_state.allocation_success) {
                    for (int j = 0; j < g_lua_states[i].initial_state.global_count; j++) {
                        if (g_lua_states[i].initial_state.allocation_success[j]) {
                            free(g_lua_states[i].initial_state.initial_globals[j]);
                        }
                    }
                }
                /* FIXED: Free the combined allocation block */
                free(g_lua_states[i].initial_state.initial_globals);
                g_lua_states[i].initial_state.initial_globals = NULL;
                g_lua_states[i].initial_state.allocation_success = NULL;
            }
            
            lua_close(g_lua_states[i].L);
            g_lua_states[i].L = NULL;
            g_lua_states[i].initialized = 0;
            g_lua_states[i].in_use = 0;
            g_lua_states[i].error_count = 0;
            
            /* Reset initial state */
            memset(&g_lua_states[i].initial_state, 0, sizeof(lua_initial_state_t));
        }
    }
    
    /* Clean up memory pool - now safe since all states are closed */
    if (g_memory_pool.pool) {
        free(g_memory_pool.pool);
        memset(&g_memory_pool, 0, sizeof(lua_memory_pool_t));
    }
    
    g_lua_pool_initialized = 0;
}

#endif /* LUA_INTEGRATION_H */