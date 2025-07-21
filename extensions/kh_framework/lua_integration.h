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

/* Enhanced constants for better performance and safety */
#define LUA_MAX_RECURSION_DEPTH 24        /* Reduced for safety */
#define LUA_STACK_SAFETY_MARGIN 10        /* Safety margin for Lua stack */
#define LUA_CACHE_CHAIN_MAX_LENGTH 6      /* Max cache chain length */
#define LUA_POOL_ALIGNMENT 16             /* Memory alignment for performance */
#define LUA_PARSE_BUFFER_SIZE 4096        /* FIXED: Reduced from 8192 for stack safety */
#define LUA_POOL_RESET_THRESHOLD 0.8      /* Reset pool when 80% used */
#define LUA_POOL_RESET_INTERVAL 100       /* Reset after 100 operations */

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

/* Bytecode cache entry with proper reference counting */
typedef struct bytecode_cache_entry_s {
    uint64_t code_hash;                  
    char* bytecode;                      
    size_t bytecode_size;                
    int ref_count;                       
    DWORD last_used;                     
    int marked_for_deletion;             /* Flag for safe deletion */
    struct bytecode_cache_entry_s* next; 
} bytecode_cache_entry_t;

/* String result cache entry with enhanced cleanup */
typedef struct string_cache_entry_s {
    uint64_t args_hash;                  
    char* result;                        
    size_t result_size;                  
    DWORD last_used;                     
    int marked_for_deletion;             /* Flag for safe deletion */
    struct string_cache_entry_s* next;   
} string_cache_entry_t;

/* Enhanced persistent Lua state with better error tracking */
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
} lua_persistent_state_t;

/* Enhanced memory pool allocator with reset capability */
typedef struct {
    char* pool;                          
    size_t size;                         
    size_t used;                         
    size_t peak_used;                    /* Track peak usage for optimization */
    pool_block_t* free_blocks;           /* Simple free list */
    int initialized;                     
    int operation_count;                 /* Track operations for reset timing */
    DWORD last_reset_time;               /* When pool was last reset */
} lua_memory_pool_t;

/* Global optimization caches - single-threaded, so no locking needed */
static lua_persistent_state_t g_lua_states[LUA_STATE_POOL_SIZE] = {0};
static int g_lua_pool_initialized = 0;

static bytecode_cache_entry_t* g_bytecode_cache[LUA_BYTECODE_CACHE_SIZE] = {0};
static string_cache_entry_t* g_string_cache[LUA_STRING_CACHE_SIZE] = {0};

static lua_memory_pool_t g_memory_pool = {0};

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
    g_memory_pool.last_reset_time = GetTickCount();
    
    return 1;
}

/* FIXED: Add pool reset functionality for long-term performance */
static void lua_reset_memory_pool(void) {
    if (!g_memory_pool.initialized) return;
    
    /* Reset pool usage - all allocations become invalid after this */
    g_memory_pool.used = 0;
    g_memory_pool.free_blocks = NULL;
    g_memory_pool.operation_count = 0;
    g_memory_pool.last_reset_time = GetTickCount();
    
    /* Update peak tracking */
    if (g_memory_pool.used > g_memory_pool.peak_used) {
        g_memory_pool.peak_used = g_memory_pool.used;
    }
}

/* FIXED: Enhanced pool allocator with reset capability and better performance */
static void* lua_pool_alloc(size_t size) {
    if (!g_memory_pool.initialized && !lua_init_memory_pool()) {
        return malloc(size); /* Fallback to system malloc */
    }
    
    /* Check if pool should be reset for performance */
    DWORD current_time = GetTickCount();
    if ((g_memory_pool.used > g_memory_pool.size * LUA_POOL_RESET_THRESHOLD) ||
        (g_memory_pool.operation_count > LUA_POOL_RESET_INTERVAL) ||
        (current_time - g_memory_pool.last_reset_time > 300000)) { /* 5 minutes */
        
        /* Only reset if no Lua states are currently active */
        int states_in_use = 0;
        for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
            if (g_lua_states[i].in_use) {
                states_in_use = 1;
                break;
            }
        }
        
        if (!states_in_use) {
            lua_reset_memory_pool();
        }
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
    
    /* Pool exhausted, fallback to system malloc */
    return malloc(size);
}

/* FIXED: Simplified check for pool allocation */
static int lua_is_pool_allocation(void* ptr) {
    if (!ptr || !g_memory_pool.initialized || !g_memory_pool.pool) return 0;
    
    return (ptr >= (void*)g_memory_pool.pool && 
            ptr < (void*)(g_memory_pool.pool + g_memory_pool.size)) ? 1 : 0;
}

/* Enhanced print function for security */
static int lua_secure_print_fast(lua_State* L) {
    /* Consume all arguments but produce no output for security */
    int n = lua_gettop(L);
    lua_pop(L, n);
    return 0;
}

/* FIXED: Enhanced custom Lua allocator with proper reallocation handling */
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
        /* FIXED: Reallocation - Proper Lua allocator contract compliance */
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
            /* FIXED: Allocation failed - return NULL as per Lua contract */
            /* This tells Lua the reallocation failed and it should handle the error */
            return NULL;
        }
    }
}

/* Enhanced bytecode cache operations with proper reference counting */
static bytecode_cache_entry_t* lua_find_cached_bytecode(uint64_t hash) {
    if (!hash) return NULL;
    
    int slot = (int)(hash % LUA_BYTECODE_CACHE_SIZE);
    bytecode_cache_entry_t* entry = g_bytecode_cache[slot];
    
    while (entry) {
        if (entry->code_hash == hash && entry->bytecode && !entry->marked_for_deletion) {
            entry->ref_count++;
            entry->last_used = GetTickCount();
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/* FIXED: Add proper reference count management */
static void lua_release_cached_bytecode(bytecode_cache_entry_t* entry) {
    if (!entry) return;
    
    entry->ref_count--;
    if (entry->ref_count <= 0) {
        entry->marked_for_deletion = 1;
    }
}

/* FIXED: Better cache entry removal with safe deletion */
static int lua_remove_old_bytecode_entry(bytecode_cache_entry_t** head) {
    if (!head || !*head) return 0;
    
    bytecode_cache_entry_t* oldest = NULL;
    bytecode_cache_entry_t** oldest_prev = NULL;
    bytecode_cache_entry_t* current = *head;
    bytecode_cache_entry_t** prev = head;
    
    /* First pass: find entries marked for deletion or with ref_count = 0 */
    while (current) {
        if (current->marked_for_deletion || current->ref_count <= 0) {
            *prev = current->next;
            if (current->bytecode) free(current->bytecode);
            free(current);
            return 1; /* Removed an entry */
        }
        prev = &current->next;
        current = current->next;
    }
    
    /* Second pass: find oldest entry if no deletable entries found */
    current = *head;
    prev = head;
    
    while (current) {
        if (!oldest || current->last_used < oldest->last_used) {
            oldest = current;
            oldest_prev = prev;
        }
        prev = &current->next;
        current = current->next;
    }
    
    if (oldest && oldest_prev) {
        *oldest_prev = oldest->next;
        if (oldest->bytecode) free(oldest->bytecode);
        free(oldest);
        return 1;
    }
    
    return 0;
}

static int lua_cache_bytecode(uint64_t hash, const char* bytecode, size_t size) {
    if (!bytecode || size == 0 || size > LUA_MEMORY_POOL_SIZE / 4 || !hash) {
        return 0;
    }
    
    int slot = (int)(hash % LUA_BYTECODE_CACHE_SIZE);
    
    /* Limit chain length for performance */
    bytecode_cache_entry_t** current = &g_bytecode_cache[slot];
    int chain_length = 0;
    
    while (*current) {
        chain_length++;
        if (chain_length >= LUA_CACHE_CHAIN_MAX_LENGTH) {
            if (!lua_remove_old_bytecode_entry(&g_bytecode_cache[slot])) {
                return 0;
            }
            break;
        }
        current = &(*current)->next;
    }
    
    /* Create new cache entry with proper error handling */
    bytecode_cache_entry_t* new_entry = (bytecode_cache_entry_t*)malloc(sizeof(bytecode_cache_entry_t));
    if (!new_entry) return 0;
    
    /* Initialize all fields first */
    memset(new_entry, 0, sizeof(bytecode_cache_entry_t));
    
    new_entry->bytecode = (char*)malloc(size);
    if (!new_entry->bytecode) {
        free(new_entry);
        return 0;
    }
    
    memcpy(new_entry->bytecode, bytecode, size);
    new_entry->code_hash = hash;
    new_entry->bytecode_size = size;
    new_entry->ref_count = 1;
    new_entry->last_used = GetTickCount();
    new_entry->marked_for_deletion = 0;
    new_entry->next = g_bytecode_cache[slot];
    g_bytecode_cache[slot] = new_entry;
    
    return 1;
}

/* Enhanced string result cache operations */
static char* lua_find_cached_result(uint64_t hash) {
    if (!hash) return NULL;
    
    int slot = (int)(hash % LUA_STRING_CACHE_SIZE);
    string_cache_entry_t* entry = g_string_cache[slot];
    
    while (entry) {
        if (entry->args_hash == hash && entry->result && !entry->marked_for_deletion) {
            entry->last_used = GetTickCount();
            return entry->result;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/* FIXED: Better cache entry removal for string cache */
static int lua_remove_old_string_entry(string_cache_entry_t** head) {
    if (!head || !*head) return 0;
    
    string_cache_entry_t* oldest = NULL;
    string_cache_entry_t** oldest_prev = NULL;
    string_cache_entry_t* current = *head;
    string_cache_entry_t** prev = head;
    
    /* First pass: find entries marked for deletion */
    while (current) {
        if (current->marked_for_deletion) {
            *prev = current->next;
            if (current->result) free(current->result);
            free(current);
            return 1;
        }
        prev = &current->next;
        current = current->next;
    }
    
    /* Second pass: find oldest entry */
    current = *head;
    prev = head;
    
    while (current) {
        if (!oldest || current->last_used < oldest->last_used) {
            oldest = current;
            oldest_prev = prev;
        }
        prev = &current->next;
        current = current->next;
    }
    
    if (oldest && oldest_prev) {
        *oldest_prev = oldest->next;
        if (oldest->result) free(oldest->result);
        free(oldest);
        return 1;
    }
    
    return 0;
}

static int lua_cache_result(uint64_t hash, const char* result) {
    if (!result || !hash) return 0;
    
    size_t result_len = strlen(result);
    if (result_len == 0 || result_len > KH_MAX_OUTPUT_SIZE) return 0;
    
    int slot = (int)(hash % LUA_STRING_CACHE_SIZE);
    
    /* Limit chain length */
    string_cache_entry_t** current = &g_string_cache[slot];
    int chain_length = 0;
    
    while (*current) {
        chain_length++;
        if (chain_length >= LUA_CACHE_CHAIN_MAX_LENGTH) {
            if (!lua_remove_old_string_entry(&g_string_cache[slot])) {
                return 0;
            }
            break;
        }
        current = &(*current)->next;
    }
    
    /* Create new result cache entry with proper error handling */
    string_cache_entry_t* new_entry = (string_cache_entry_t*)malloc(sizeof(string_cache_entry_t));
    if (!new_entry) return 0;
    
    /* Initialize all fields */
    memset(new_entry, 0, sizeof(string_cache_entry_t));
    
    new_entry->result = (char*)malloc(result_len + 1);
    if (!new_entry->result) {
        free(new_entry);
        return 0;
    }
    
    strcpy_s(new_entry->result, result_len + 1, result);
    new_entry->args_hash = hash;
    new_entry->result_size = result_len;
    new_entry->last_used = GetTickCount();
    new_entry->marked_for_deletion = 0;
    new_entry->next = g_string_cache[slot];
    g_string_cache[slot] = new_entry;
    
    return 1;
}

/* Enhanced Lua state initialization with better error handling */
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
    
    return 1;
}

/* Enhanced state management with health monitoring */
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
        return best_state;
    }
    
    return NULL; /* No available states */
}

/* FIXED: Comprehensive global cleanup function */
static void lua_clear_all_globals(lua_State* L) {
    if (!L || !lua_checkstack(L, 3)) return;
    
    /* Get the global table */
    lua_pushglobaltable(L);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    
    /* Collect all keys first to avoid iterator invalidation */
    lua_newtable(L); /* Table to store keys to clear */
    int keys_table = lua_gettop(L);
    int key_count = 0;
    
    /* Iterate through globals and collect user-defined keys */
    lua_pushnil(L);
    while (lua_next(L, -3) != 0) {
        /* Check if this is a user-defined global (not a standard library function) */
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);
            if (key) {
                /* Skip essential globals that should remain */
                const char* essential[] = {
                    "type", "next", "pairs", "ipairs", "tostring", "tonumber",
                    "math", "string", "table", "print", "jit", "_G", "_VERSION", NULL
                };
                
                int is_essential = 0;
                for (int i = 0; essential[i]; i++) {
                    if (strcmp(key, essential[i]) == 0) {
                        is_essential = 1;
                        break;
                    }
                }
                
                /* If not essential, mark for deletion */
                if (!is_essential) {
                    lua_pushvalue(L, -2); /* Duplicate key */
                    lua_rawseti(L, keys_table, ++key_count);
                }
            }
        }
        lua_pop(L, 1); /* Remove value, keep key for next iteration */
    }
    
    /* Now clear all collected keys */
    for (int i = 1; i <= key_count; i++) {
        lua_rawgeti(L, keys_table, i);
        lua_pushnil(L);
        lua_rawset(L, -4); /* Set global[key] = nil */
    }
    
    lua_pop(L, 2); /* Remove keys table and global table */
}

/* FIXED: Enhanced state release with immediate comprehensive cleanup */
static void lua_release_optimized_state(lua_persistent_state_t* state) {
    if (!state || !state->L) return;
    
    /* Enhanced cleanup with error checking */
    int stack_top = lua_gettop(state->L);
    if (stack_top > 0) {
        lua_settop(state->L, 0); /* Clear stack */
    }
    
    /* FIXED: Always perform comprehensive global cleanup on every release */
    lua_clear_all_globals(state->L);
    
    /* Run GC more frequently to clean up immediately */
    lua_gc(state->L, LUA_GCCOLLECT, 0);
    
    /* Update cleanup time */
    state->last_cleanup_time = GetTickCount();
    
    /* Reset state if it has too many errors */
    if (state->error_count > 10) {
        lua_close(state->L);
        state->L = NULL;
        state->initialized = 0;
        state->error_count = 0;
    }
    
    state->in_use = 0;
}

/* Forward declaration for recursive parsing with depth limiting */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth);

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

/* Enhanced Arma array parsing with depth limiting */
static const char* lua_parse_arma_array(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr || depth > LUA_MAX_RECURSION_DEPTH) return NULL;
    
    /* Ensure stack space */
    if (!lua_checkstack(L, 3)) return NULL;
    
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

/* FIXED: Enhanced recursive value parsing with heap allocation for large buffers */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr || ptr >= end || depth > LUA_MAX_RECURSION_DEPTH) return NULL;
    
    /* Ensure stack space */
    if (!lua_checkstack(L, 2)) return NULL;
    
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
    size_t max_value_length = LUA_PARSE_BUFFER_SIZE - 1;
    
    while (value_end < end && (value_end - value_start) < (ptrdiff_t)max_value_length) {
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
    
    /* Extract and parse the value with size validation */
    size_t value_len = value_end - value_start;
    if (value_len == 0 || value_len >= LUA_PARSE_BUFFER_SIZE) return NULL;
    
    /* FIXED: Use heap allocation for buffer to avoid stack overflow in deep recursion */
    char* value_buffer = (char*)malloc(LUA_PARSE_BUFFER_SIZE);
    if (!value_buffer) return NULL;
    
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
    
    free(value_buffer);
    return result_ptr;
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

/* FIXED: Enhanced argument parsing with comprehensive error handling and proper return values */
static int lua_parse_args_optimized(lua_State* L, const char* args_str) {
    if (!args_str || !L || !lua_checkstack(L, LUA_STACK_SAFETY_MARGIN)) return -1; /* FIXED: Return -1 for errors */
    
    /* Clear all previous argument globals first */
    lua_clear_argument_globals(L);
    
    /* Skip to opening bracket */
    const char* ptr = args_str;
    while (*ptr && *ptr != '[') ptr++;
    if (*ptr != '[') return -1; /* FIXED: Return -1 for errors */
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
    
    /* General parsing with recursive array support - FIXED: Better error handling */
    int arg_count = 0;
    int initial_stack_top = lua_gettop(L);
    
    while (ptr < end && *ptr != ']' && arg_count < LUA_MAX_SIMPLE_ARGS) {
        /* Skip separators and whitespace */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
        if (ptr >= end || *ptr == ']') break;
        
        /* Parse this argument recursively */
        const char* next_ptr = lua_parse_value_recursive(L, ptr, end, 0);
        if (!next_ptr) {
            /* FIXED: Clean up stack on parsing failure */
            lua_settop(L, initial_stack_top);
            return -1; /* FIXED: Return -1 for errors */
        }
        
        arg_count++;
        
        /* Set as global variable argN */
        char arg_name[16];
        if (_snprintf_s(arg_name, sizeof(arg_name), _TRUNCATE, "arg%d", arg_count) > 0) {
            lua_setglobal(L, arg_name);
        } else {
            lua_pop(L, 1); /* Remove the value from stack if naming failed */
            break;
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

/* FIXED: Enhanced execution function with proper error handling for argument parsing */
static int kh_execute_lua_optimized(const char* arguments, const char* code, 
                                   char* output, int output_size) {
    if (!arguments || !code || !output || output_size <= 0) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    /* Input validation */
    size_t args_len = strlen(arguments);
    size_t code_len = strlen(code);
    
    if (args_len > LUA_STACK_BUFFER_SIZE || code_len > LUA_STACK_BUFFER_SIZE) {
        kh_set_error(output, output_size, "INPUT TOO LARGE");
        return 1;
    }
    
    /* Create combined hash for result caching */
    char combined_input[16384];
    int combined_len = _snprintf_s(combined_input, sizeof(combined_input), _TRUNCATE, 
                                  "%s|%s", arguments, code);
    
    uint64_t combined_hash = 0;
    if (combined_len > 0) {
        combined_hash = lua_hash_fnv1a(combined_input, combined_len);
        
        /* Check result cache first */
        char* cached_result = lua_find_cached_result(combined_hash);
        if (cached_result) {
            strncpy_s(output, output_size, cached_result, _TRUNCATE);
            return 0;
        }
    }
    
    /* Clean the code string */
    char* clean_code = NULL;
    int code_allocated = 0;
    
    if (code_len < LUA_STACK_BUFFER_SIZE) {
        clean_code = (char*)alloca(code_len + 1); /* Stack allocation for small code */
    } else {
        clean_code = (char*)malloc(code_len + 1);
        if (!clean_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        code_allocated = 1;
    }
    
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    /* Hash the cleaned code for bytecode caching */
    uint64_t code_hash = lua_hash_fnv1a(clean_code, strlen(clean_code));
    
    /* Check bytecode cache */
    bytecode_cache_entry_t* cached_bytecode = lua_find_cached_bytecode(code_hash);
    
    /* Get optimized Lua state */
    lua_persistent_state_t* state = lua_get_optimized_state();
    if (!state) {
        if (code_allocated) free(clean_code);
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    
    /* FIXED: Track initial stack position for cleanup */
    int initial_stack_top = lua_gettop(L);
    
    /* FIXED: Parse arguments and check for errors */
    int arg_parse_result = lua_parse_args_optimized(L, arguments);
    if (arg_parse_result < 0) {
        /* Argument parsing failed */
        if (cached_bytecode) lua_release_cached_bytecode(cached_bytecode);
        lua_settop(L, initial_stack_top);
        lua_release_optimized_state(state);
        if (code_allocated) free(clean_code);
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
            /* FIXED: Simplified bytecode extraction with better error handling */
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
                        if (bytecode && bytecode_size > 0 && bytecode_size < LUA_MEMORY_POOL_SIZE / 4) {
                            if (lua_cache_bytecode(code_hash, bytecode, bytecode_size)) {
                                bytecode_extraction_success = 1;
                            }
                        }
                    }
                }
            }
            
            /* FIXED: Restore stack to known state regardless of extraction outcome */
            lua_settop(L, stack_before_extraction);
        }
    }
    
    if (compile_result == LUA_OK) {
        /* Execute with error handling */
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            if (lua_format_result_optimized(L, output, output_size)) {
                /* Cache the result for future use */
                if (combined_len > 0 && strlen(output) < KH_MAX_OUTPUT_SIZE / 2) {
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
    
    /* FIXED: Release cached bytecode reference properly */
    if (cached_bytecode) {
        lua_release_cached_bytecode(cached_bytecode);
    }
    
    /* FIXED: Ensure stack is properly cleaned up */
    lua_settop(L, initial_stack_top);
    
    /* Cleanup */
    lua_release_optimized_state(state);
    if (code_allocated) free(clean_code);
    
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
    
    if (code_len >= LUA_STACK_BUFFER_SIZE) {
        kh_set_error(output, output_size, "CODE TOO LARGE");
        return 1;
    }
    
    /* Clean the code string */
    char* clean_code = (char*)alloca(code_len + 1);
    kh_clean_string(code, clean_code, (int)code_len + 1);
    
    /* Hash the cleaned code */
    uint64_t code_hash = lua_hash_fnv1a(clean_code, strlen(clean_code));
    
    /* Check if already cached */
    bytecode_cache_entry_t* existing = lua_find_cached_bytecode(code_hash);
    if (existing) {
        /* FIXED: Release the reference properly */
        lua_release_cached_bytecode(existing);
        _snprintf_s(output, output_size, _TRUNCATE, "[\"ALREADY_CACHED\"]");
        return 0;
    }
    
    /* Get a Lua state for compilation */
    lua_persistent_state_t* state = lua_get_optimized_state();
    if (!state) {
        kh_set_error(output, output_size, "NO AVAILABLE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    
    /* FIXED: Track initial stack position */
    int initial_stack_top = lua_gettop(L);
    
    /* Compile the code */
    int compile_result = luaL_loadstring(L, clean_code);
    
    if (compile_result == LUA_OK) {
        /* FIXED: Simplified bytecode extraction with better stack management */
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
                    if (bytecode && bytecode_size > 0 && bytecode_size < LUA_MEMORY_POOL_SIZE / 4) {
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
        
        /* FIXED: Restore stack to known state */
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
    
    /* FIXED: Ensure proper stack cleanup */
    lua_settop(L, initial_stack_top);
    
    /* Cleanup */
    lua_release_optimized_state(state);
    
    return result;
}

/* FIXED: Enhanced cleanup function with comprehensive memory management */
static void kh_cleanup_lua_states(void) {
    /* Clean up bytecode cache with proper reference handling */
    for (int i = 0; i < LUA_BYTECODE_CACHE_SIZE; i++) {
        bytecode_cache_entry_t* entry = g_bytecode_cache[i];
        while (entry) {
            bytecode_cache_entry_t* next = entry->next;
            if (entry->bytecode) free(entry->bytecode);
            free(entry);
            entry = next;
        }
        g_bytecode_cache[i] = NULL;
    }
    
    /* Clean up string cache */
    for (int i = 0; i < LUA_STRING_CACHE_SIZE; i++) {
        string_cache_entry_t* entry = g_string_cache[i];
        while (entry) {
            string_cache_entry_t* next = entry->next;
            if (entry->result) free(entry->result);
            free(entry);
            entry = next;
        }
        g_string_cache[i] = NULL;
    }
    
    /* Clean up Lua states */
    for (int i = 0; i < LUA_STATE_POOL_SIZE; i++) {
        if (g_lua_states[i].L) {
            lua_close(g_lua_states[i].L);
            g_lua_states[i].L = NULL;
            g_lua_states[i].initialized = 0;
            g_lua_states[i].in_use = 0;
            g_lua_states[i].error_count = 0;
        }
    }
    
    /* Clean up memory pool */
    if (g_memory_pool.pool) {
        free(g_memory_pool.pool);
        g_memory_pool.pool = NULL;
        g_memory_pool.used = 0;
        g_memory_pool.size = 0;
        g_memory_pool.peak_used = 0;
        g_memory_pool.free_blocks = NULL;
        g_memory_pool.initialized = 0;
        g_memory_pool.operation_count = 0;
        g_memory_pool.last_reset_time = 0;
    }
    
    g_lua_pool_initialized = 0;
}

#endif /* LUA_INTEGRATION_H */