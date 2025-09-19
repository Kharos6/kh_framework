#ifndef LUA_INTEGRATION_H
#define LUA_INTEGRATION_H

#include "common_defines.h"
#include "process_kh_data.h"
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
    kh_generic_hash_table_t hash_table; /* Hash table for fast lookup */
    int initialized;              /* Whether storage is initialized */
} lua_variable_storage_t;

/* Structure to hold persistent Lua functions */
typedef struct lua_function_s {
    char* name;           /* Function name */
    char* code;           /* Original function code */
} lua_function_t;

/* Function storage with hash table for fast lookup */
typedef struct {
    lua_function_t* functions;        /* Array of functions */
    uint32_t function_count;          /* Current number of functions */
    uint32_t function_capacity;       /* Allocated capacity */
    kh_generic_hash_table_t hash_table;  /* Hash table for fast lookup */
    int initialized;                  /* Whether storage is initialized */
} lua_function_storage_t;

/* Global persistent Lua functions storage */
static lua_function_storage_t g_lua_function_storage = {0};

/* Global persistent Lua variables storage with hash table */
static lua_variable_storage_t g_lua_variable_storage = {0};

/* Initialize variable storage with hash table */
static int lua_init_variable_storage(void) {
    if (g_lua_variable_storage.initialized) return 1;
    
    g_lua_variable_storage.variable_capacity = 16;
    g_lua_variable_storage.variables = (lua_variable_t*)calloc(g_lua_variable_storage.variable_capacity, 
                                                               sizeof(lua_variable_t));
    if (!g_lua_variable_storage.variables) return 0;
    
    g_lua_variable_storage.hash_table.size = LUA_VAR_HASH_TABLE_MIN_SIZE;
    g_lua_variable_storage.hash_table.entries = (kh_generic_hash_entry_t*)calloc(g_lua_variable_storage.hash_table.size, 
                                                                              sizeof(kh_generic_hash_entry_t));
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
    uint32_t new_size = kh_calculate_optimal_hash_size(g_lua_variable_storage.variable_count);
    
    /* Allocate new hash table */
    kh_generic_hash_entry_t* new_entries = (kh_generic_hash_entry_t*)calloc(new_size, sizeof(kh_generic_hash_entry_t));
    if (!new_entries) return 0;
    
    /* Rebuild hash table entries in the new table first */
    uint32_t new_used_count = 0;
    for (uint32_t i = 0; i < g_lua_variable_storage.variable_count; i++) {
        if (g_lua_variable_storage.variables[i].name) {
            uint32_t name_hash = kh_hash_name_case_insensitive(g_lua_variable_storage.variables[i].name);
            if (!kh_generic_hash_insert(new_entries, new_size, name_hash, i)) {
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

/* Rebuild function hash table when needed */
static int lua_rebuild_function_hash_table(void) {
    if (!g_lua_function_storage.initialized) return 0;
    
    /* Calculate new size */
    uint32_t new_size = kh_calculate_optimal_hash_size(g_lua_function_storage.function_count);
    
    /* Allocate new hash table */
    kh_generic_hash_entry_t* new_entries = (kh_generic_hash_entry_t*)calloc(new_size, sizeof(kh_generic_hash_entry_t));
    if (!new_entries) return 0;
    
    /* Rebuild hash table entries in the new table first */
    uint32_t new_used_count = 0;
    for (uint32_t i = 0; i < g_lua_function_storage.function_count; i++) {
        if (g_lua_function_storage.functions[i].name) {
            uint32_t name_hash = kh_hash_name_case_insensitive(g_lua_function_storage.functions[i].name);
            if (!kh_generic_hash_insert(new_entries, new_size, name_hash, i)) {
                /* Rebuild failed - free the new table and keep the old one */
                free(new_entries);
                return 0;
            }
            new_used_count++;
        }
    }
    
    /* Only now that we're sure rebuild succeeded, replace the old table */
    free(g_lua_function_storage.hash_table.entries);
    g_lua_function_storage.hash_table.entries = new_entries;
    g_lua_function_storage.hash_table.size = new_size;
    g_lua_function_storage.hash_table.used_count = new_used_count;
    g_lua_function_storage.hash_table.deleted_count = 0;
    g_lua_function_storage.hash_table.needs_rebuild = 0;
    
    return 1;
}

/* Find a stored Lua variable by name using hash table */
static lua_variable_t* lua_find_variable(const char* name) {
    if (!name || !g_lua_variable_storage.initialized) return NULL;
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    int found = 0;
    uint32_t index = kh_generic_hash_find(g_lua_variable_storage.hash_table.entries,
                                                   g_lua_variable_storage.hash_table.size, 
                                                   name_hash, &found);
    
    if (found && !g_lua_variable_storage.hash_table.entries[index].deleted) {
        uint32_t data_index = g_lua_variable_storage.hash_table.entries[index].data_index;
        if (data_index < g_lua_variable_storage.variable_count) {
            lua_variable_t* var = &g_lua_variable_storage.variables[data_index];
            if (var->name && strcmp(var->name, name) == 0) {
                return var;
            }
        }
    }
    
    return NULL;
}

/* Forward declarations */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth);
static lua_single_state_t* lua_get_single_state(void);
static int lua_inject_function(lua_State* L, lua_function_t* func);
static int lua_inject_variable(lua_State* L, lua_variable_t* var);
static int lua_table_to_arma_recursive(lua_State* L, int stack_index, char** buffer, 
                                              size_t* buffer_size, size_t* pos, int depth);
static int lua_is_array(lua_State* L, int stack_index);

/* Initialize function storage with hash table */
static int lua_init_function_storage(void) {
    if (g_lua_function_storage.initialized) return 1;
    
    g_lua_function_storage.function_capacity = 16;
    g_lua_function_storage.functions = (lua_function_t*)calloc(g_lua_function_storage.function_capacity, 
                                                               sizeof(lua_function_t));
    if (!g_lua_function_storage.functions) return 0;
    
    g_lua_function_storage.hash_table.size = LUA_VAR_HASH_TABLE_MIN_SIZE;
    g_lua_function_storage.hash_table.entries = (kh_generic_hash_entry_t*)calloc(g_lua_function_storage.hash_table.size, 
                                                                              sizeof(kh_generic_hash_entry_t));
    if (!g_lua_function_storage.hash_table.entries) {
        free(g_lua_function_storage.functions);
        return 0;
    }
    
    g_lua_function_storage.function_count = 0;
    g_lua_function_storage.hash_table.used_count = 0;
    g_lua_function_storage.hash_table.deleted_count = 0;
    g_lua_function_storage.hash_table.needs_rebuild = 0;
    g_lua_function_storage.initialized = 1;
    
    return 1;
}

/* Find a stored Lua function by name using hash table */
static lua_function_t* lua_find_function(const char* name) {
    if (!name || !g_lua_function_storage.initialized) return NULL;
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    int found = 0;
    uint32_t index = kh_generic_hash_find(g_lua_function_storage.hash_table.entries,
                                                   g_lua_function_storage.hash_table.size, 
                                                   name_hash, &found);
    
    if (found && !g_lua_function_storage.hash_table.entries[index].deleted) {
        uint32_t func_index = g_lua_function_storage.hash_table.entries[index].data_index;
        if (func_index < g_lua_function_storage.function_count) {
            lua_function_t* func = &g_lua_function_storage.functions[func_index];
            if (func->name && strcmp(func->name, name) == 0) {
                return func;
            }
        }
    }
    
    return NULL;
}

/* Set a persistent Lua function with proper memory management */
static int lua_set_function(const char* name, const char* code) {
    if (!name || !code) return 0;
    
    /* Validate function name */
    if (!kh_validate_variable_name(name)) return 0;
    
    /* Find existing function */
    lua_function_t* existing_func = lua_find_function(name);
    if (existing_func) {
        /* Update existing function */
        char* new_code = (char*)malloc(strlen(code) + 1);
        if (!new_code) return 0;
        
        strcpy_s(new_code, strlen(code) + 1, code);
        
        /* Replace old code */
        free(existing_func->code);
        existing_func->code = new_code;
        
        /* IMMEDIATE INJECTION: Inject into current Lua state */
        lua_single_state_t* state = lua_get_single_state();
        if (state && state->L && state->initialized) {
            lua_inject_function(state->L, existing_func);
        }
        
        return 1;
    }
    
    /* Add new function */
    
    /* Expand functions array if needed */
    if (g_lua_function_storage.function_count >= g_lua_function_storage.function_capacity) {
        uint32_t new_capacity = g_lua_function_storage.function_capacity * 2;
        lua_function_t* new_functions = (lua_function_t*)realloc(g_lua_function_storage.functions,
                                                                new_capacity * sizeof(lua_function_t));
        if (!new_functions) return 0;
        
        /* Initialize new memory */
        memset(&new_functions[g_lua_function_storage.function_capacity], 0,
               (new_capacity - g_lua_function_storage.function_capacity) * sizeof(lua_function_t));
        
        g_lua_function_storage.functions = new_functions;
        g_lua_function_storage.function_capacity = new_capacity;
    }
    
    /* Allocate all memory first */
    size_t name_len = strlen(name);
    size_t code_len = strlen(code);
    
    char* name_copy = (char*)malloc(name_len + 1);
    char* code_copy = (char*)malloc(code_len + 1);
    
    if (!name_copy || !code_copy) {
        free(name_copy);
        free(code_copy);
        return 0;
    }
    
    strcpy_s(name_copy, name_len + 1, name);
    strcpy_s(code_copy, code_len + 1, code);
    
    /* Rebuild hash table if needed BEFORE adding the function */
    if (kh_generic_hash_needs_rebuild(&g_lua_function_storage.hash_table, g_lua_function_storage.function_count)) {
        if (!lua_rebuild_function_hash_table()) {
            free(name_copy);
            free(code_copy);
            return 0;
        }
    }
    
    /* Add to functions array */
    uint32_t func_index = g_lua_function_storage.function_count;
    
    /* Add to hash table BEFORE updating the array to maintain consistency */
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    if (!kh_generic_hash_insert(g_lua_function_storage.hash_table.entries,
                                         g_lua_function_storage.hash_table.size, 
                                         name_hash, func_index)) {
        /* Hash table insertion failed - free allocated memory */
        free(name_copy);
        free(code_copy);
        return 0;
    }
    
    /* Now it's safe to update the array since hash table succeeded */
    g_lua_function_storage.functions[func_index].name = name_copy;
    g_lua_function_storage.functions[func_index].code = code_copy;
    g_lua_function_storage.function_count++;
    g_lua_function_storage.hash_table.used_count++;
    
    /* IMMEDIATE INJECTION: Inject new function into current Lua state */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized) {
        lua_inject_function(state->L, &g_lua_function_storage.functions[func_index]);
    }
    
    return 1;
}

/* Delete a persistent Lua function */
static int lua_delete_function(const char* name) {
    if (!name || !g_lua_function_storage.initialized) return 0;
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    int found = 0;
    uint32_t hash_index = kh_generic_hash_find(g_lua_function_storage.hash_table.entries,
                                                        g_lua_function_storage.hash_table.size, 
                                                        name_hash, &found);
    
    if (!found || g_lua_function_storage.hash_table.entries[hash_index].deleted) {
        return 0;
    }
    
    uint32_t func_index = g_lua_function_storage.hash_table.entries[hash_index].data_index;
    if (func_index >= g_lua_function_storage.function_count) {
        return 0;
    }
    
    lua_function_t* func = &g_lua_function_storage.functions[func_index];
    if (!func->name || strcmp(func->name, name) != 0) {
        return 0;
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
    free(func->name);
    free(func->code);
    
    /* Remove from hash table */
    kh_generic_hash_delete(g_lua_function_storage.hash_table.entries,
                         g_lua_function_storage.hash_table.size, name_hash);
    g_lua_function_storage.hash_table.used_count--;
    g_lua_function_storage.hash_table.deleted_count++;
    
    /* Use swap-with-last */
    uint32_t last_index = g_lua_function_storage.function_count - 1;
    
    if (func_index != last_index) {
        g_lua_function_storage.functions[func_index] = g_lua_function_storage.functions[last_index];
        
        /* Update hash table entry for moved function */
        if (g_lua_function_storage.functions[func_index].name) {
            uint32_t moved_name_hash = kh_hash_name_case_insensitive(g_lua_function_storage.functions[func_index].name);
            int moved_found = 0;
            uint32_t moved_hash_index = kh_generic_hash_find(g_lua_function_storage.hash_table.entries,
                                                                      g_lua_function_storage.hash_table.size, 
                                                                      moved_name_hash, &moved_found);
            if (moved_found && !g_lua_function_storage.hash_table.entries[moved_hash_index].deleted) {
                g_lua_function_storage.hash_table.entries[moved_hash_index].data_index = func_index;
            }
        }
    }
    
    /* Clear last element */
    memset(&g_lua_function_storage.functions[last_index], 0, sizeof(lua_function_t));
    g_lua_function_storage.function_count--;
    
    /* Check if rebuild needed */
    if (kh_generic_hash_needs_rebuild(&g_lua_function_storage.hash_table, g_lua_function_storage.function_count)) {
        g_lua_function_storage.hash_table.needs_rebuild = 1;
    }
    
    return 1;
}

/* Inject a single function into Lua state with JIT optimization */
static int lua_inject_function(lua_State* L, lua_function_t* func) {
    if (!L || !func || !func->name || !func->code) return 0;
    
    if (!lua_checkstack(L, 5)) return 0;
    
    int initial_stack_top = lua_gettop(L);
    
    /* Create function code with wrapper that accepts varargs */
    size_t code_len = strlen(func->code);
    size_t name_len = strlen(func->name);
    size_t wrapper_len = code_len + name_len + 64;
    
    char* wrapper_code = (char*)malloc(wrapper_len);
    if (!wrapper_code) return 0;
    
    /* Wrap user code in function definition with varargs */
    int result = _snprintf_s(wrapper_code, wrapper_len, _TRUNCATE, 
                            "function %s(...)\n%s\nend", func->name, func->code);
    
    if (result < 0) {
        free(wrapper_code);
        return 0;
    }
    
    /* Compile and execute the function definition */
    if (luaL_loadstring(L, wrapper_code) == LUA_OK) {
        if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
            /* Function is now defined, try to trigger JIT optimization */
            lua_getglobal(L, func->name);
            if (lua_isfunction(L, -1)) {
                /* Call function with no arguments to trigger JIT (ignore any errors) */
                if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                    /* Clear any error from the stack */
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1); /* Remove non-function value */
            }
            
            free(wrapper_code);
            lua_settop(L, initial_stack_top);
            return 1;
        }
    }
    
    /* Cleanup on failure */
    free(wrapper_code);
    lua_settop(L, initial_stack_top);
    return 0;
}

/* Inject all stored functions into Lua state */
static int lua_inject_all_functions(lua_State* L) {
    if (!L || !g_lua_function_storage.initialized) return 1;
    
    if (!lua_checkstack(L, 5)) return 0;
    
    /* Inject all stored functions */
    for (uint32_t i = 0; i < g_lua_function_storage.function_count; i++) {
        lua_function_t* func = &g_lua_function_storage.functions[i];
        if (func && func->name) {
            lua_inject_function(L, func);
        }
    }
    
    return 1;
}

/* Set a persistent Lua variable with proper memory leak prevention */
static int lua_set_variable(const char* name, const char* type_value_array) {
    if (!name || !type_value_array) return 0;
    
    /* Validate variable name */
    if (!kh_validate_variable_name(name)) return 0;
    
    /* Parse the [type, value] array */
    char* type_str = NULL;
    char* value_str = NULL;
    if (!kh_parse_type_value_array(type_value_array, &type_str, &value_str)) {
        return 0;
    }
    
    /* Validate type */
    int var_type = -1;
    if (strcmp(type_str, "ARRAY") == 0) var_type = 0;
    else if (strcmp(type_str, "STRING") == 0) var_type = 1;
    else if (strcmp(type_str, "SCALAR") == 0) var_type = 2;
    else if (strcmp(type_str, "HASHMAP") == 0) var_type = 3;
    else if (strcmp(type_str, "BOOL") == 0) var_type = 4;
    else {
        free(type_str);
        free(value_str);
        return 0; /* Invalid type */
    }
    
    /* Find existing variable */
    lua_variable_t* existing_var = lua_find_variable(name);
    if (existing_var) {
        /* Update existing variable */
        char* final_value = (char*)malloc(strlen(value_str) + 1);
        if (!final_value) {
            free(type_str);
            free(value_str);
            return 0;
        }
        strcpy_s(final_value, strlen(value_str) + 1, value_str);
        
        /* Update type */
        char* new_type = (char*)malloc(strlen(type_str) + 1);
        if (!new_type) {
            free(final_value);
            free(type_str);
            free(value_str);
            return 0;
        }
        strcpy_s(new_type, strlen(type_str) + 1, type_str);
        
        /* Replace old values */
        free(existing_var->original_value);
        free(existing_var->type);
        existing_var->original_value = final_value;
        existing_var->type = new_type;
        existing_var->lua_type = var_type;
        
        /* IMMEDIATE INJECTION: Inject into current Lua state */
        lua_single_state_t* state = lua_get_single_state();
        if (state && state->L && state->initialized) {
            lua_inject_variable(state->L, existing_var);
        }
        
        free(type_str);
        free(value_str);
        return 1;
    }
    
    /* Add new variable */
    
    /* Expand variables array if needed */
    if (g_lua_variable_storage.variable_count >= g_lua_variable_storage.variable_capacity) {
        uint32_t new_capacity = g_lua_variable_storage.variable_capacity * 2;
        lua_variable_t* new_variables = (lua_variable_t*)realloc(g_lua_variable_storage.variables,
                                                                new_capacity * sizeof(lua_variable_t));
        if (!new_variables) {
            free(type_str);
            free(value_str);
            return 0;
        }
        
        /* Initialize new memory */
        memset(&new_variables[g_lua_variable_storage.variable_capacity], 0,
               (new_capacity - g_lua_variable_storage.variable_capacity) * sizeof(lua_variable_t));
        
        g_lua_variable_storage.variables = new_variables;
        g_lua_variable_storage.variable_capacity = new_capacity;
    }
    
    /* Allocate all memory first with comprehensive error handling */
    size_t name_len = strlen(name);
    size_t value_len = strlen(value_str);
    size_t type_len = strlen(type_str);
    
    char* name_copy = (char*)malloc(name_len + 1);
    char* value_copy = (char*)malloc(value_len + 1);
    char* type_copy = (char*)malloc(type_len + 1);
    
    /* Check all allocations before proceeding */
    if (!name_copy || !value_copy || !type_copy) {
        /* Free any successful allocations */
        free(name_copy);
        free(value_copy);
        free(type_copy);
        free(type_str);
        free(value_str);
        return 0;
    }
    
    /* Copy strings */
    strcpy_s(name_copy, name_len + 1, name);
    strcpy_s(value_copy, value_len + 1, value_str);
    strcpy_s(type_copy, type_len + 1, type_str);
    
    /* Rebuild hash table if needed BEFORE adding the variable */
    if (kh_generic_hash_needs_rebuild(&g_lua_variable_storage.hash_table, g_lua_variable_storage.variable_count)) {
        if (!lua_rebuild_variable_hash_table()) {
            /* Free allocated memory on hash table failure */
            free(name_copy);
            free(value_copy);
            free(type_copy);
            free(type_str);
            free(value_str);
            return 0;
        }
    }
    
    /* Add to variables array */
    uint32_t data_index = g_lua_variable_storage.variable_count;
    
    /* Add to hash table BEFORE updating the array to maintain consistency */
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    if (!kh_generic_hash_insert(g_lua_variable_storage.hash_table.entries,
                                         g_lua_variable_storage.hash_table.size, 
                                         name_hash, data_index)) {
        /* Hash table insertion failed - free allocated memory */
        free(name_copy);
        free(value_copy);
        free(type_copy);
        free(type_str);
        free(value_str);
        return 0;
    }
    
    /* Now it's safe to update the array since hash table succeeded */
    g_lua_variable_storage.variables[data_index].name = name_copy;
    g_lua_variable_storage.variables[data_index].original_value = value_copy;
    g_lua_variable_storage.variables[data_index].type = type_copy;
    g_lua_variable_storage.variables[data_index].lua_type = var_type;
    g_lua_variable_storage.variable_count++;
    g_lua_variable_storage.hash_table.used_count++;
    
    /* IMMEDIATE INJECTION: Inject new variable into current Lua state */
    lua_single_state_t* state = lua_get_single_state();
    if (state && state->L && state->initialized) {
        lua_inject_variable(state->L, &g_lua_variable_storage.variables[data_index]);
    }
    
    free(type_str);
    free(value_str);
    return 1;
}

/* Delete a persistent Lua variable using hash table */
static int lua_delete_variable(const char* name) {
    if (!name || !g_lua_variable_storage.initialized) return 0;
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    int found = 0;
    uint32_t hash_index = kh_generic_hash_find(g_lua_variable_storage.hash_table.entries,
                                                        g_lua_variable_storage.hash_table.size, 
                                                        name_hash, &found);
    
    if (!found || g_lua_variable_storage.hash_table.entries[hash_index].deleted) {
        return 0; /* Not found */
    }
    
    uint32_t data_index = g_lua_variable_storage.hash_table.entries[hash_index].data_index;
    if (data_index >= g_lua_variable_storage.variable_count) {
        return 0; /* Invalid index */
    }
    
    lua_variable_t* var = &g_lua_variable_storage.variables[data_index];
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
    kh_generic_hash_delete(g_lua_variable_storage.hash_table.entries,
                         g_lua_variable_storage.hash_table.size, name_hash);
    g_lua_variable_storage.hash_table.used_count--;
    g_lua_variable_storage.hash_table.deleted_count++;
    
    /* Use swap-with-last instead of shifting to avoid O(n) operations */
    uint32_t last_index = g_lua_variable_storage.variable_count - 1;
    
    if (data_index != last_index) {
        /* Move last element to deleted position */
        g_lua_variable_storage.variables[data_index] = g_lua_variable_storage.variables[last_index];
        
        /* Update hash table entry for the moved variable */
        if (g_lua_variable_storage.variables[data_index].name) {
            uint32_t moved_name_hash = kh_hash_name_case_insensitive(g_lua_variable_storage.variables[data_index].name);
            int moved_found = 0;
            uint32_t moved_hash_index = kh_generic_hash_find(g_lua_variable_storage.hash_table.entries,
                                                                      g_lua_variable_storage.hash_table.size, 
                                                                      moved_name_hash, &moved_found);
            if (moved_found && !g_lua_variable_storage.hash_table.entries[moved_hash_index].deleted) {
                g_lua_variable_storage.hash_table.entries[moved_hash_index].data_index = data_index;
            }
        }
    }
    
    /* Clear last element */
    memset(&g_lua_variable_storage.variables[last_index], 0, sizeof(lua_variable_t));
    g_lua_variable_storage.variable_count--;
    
    /* Check if rebuild needed */
    if (kh_generic_hash_needs_rebuild(&g_lua_variable_storage.hash_table, g_lua_variable_storage.variable_count)) {
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
                    
                    /* Validate UTF-8 before pushing to Lua */
                    if (kh_validate_utf8_string(clean_value, (int)strlen(clean_value))) {
                        lua_pushstring(L, clean_value);
                    } else {
                        lua_pushstring(L, ""); /* Push empty string for invalid UTF-8 */
                    }

                    free(clean_value);
                    lua_setglobal(L, var->name);
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
                int bool_val = kh_validate_bool_value(var->original_value);
                lua_pushboolean(L, bool_val);
                lua_setglobal(L, var->name);
                return 1;
            }
            break;
    }
    
    return 0;
}

/* Immediate variable injection - replaces lazy loading */
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

/* Helper function to format a single Lua value at a specific stack position */
static char* lua_format_single_value(lua_State* L, int stack_index) {
    if (!L) return NULL;
    
    if (!lua_checkstack(L, 3)) return NULL;
    
    /* Normalize stack index to absolute */
    int abs_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    
    /* Determine type */
    int lua_type_val = lua_type(L, abs_index);
    
    char* output = NULL;
    size_t output_size = 256;  /* Start with reasonable size */
    size_t pos = 0;
    
    output = (char*)malloc(output_size);
    if (!output) return NULL;
    
    switch (lua_type_val) {
        case LUA_TNIL:
            strcpy_s(output, output_size, "nil");
            return output;
            
        case LUA_TBOOLEAN: {
            const char* bool_str = lua_toboolean(L, abs_index) ? "true" : "false";
            strcpy_s(output, output_size, bool_str);
            return output;
        }
            
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, abs_index);
            if (isnan(num) || isinf(num)) {
                strcpy_s(output, output_size, "0");
            } else {
                char num_buffer[64];
                if (num == floor(num) && num >= -2147483648.0 && num <= 2147483647.0) {
                    _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.0f", num);
                } else {
                    _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.15g", num);
                }
                strcpy_s(output, output_size, num_buffer);
            }
            return output;
        }
            
        case LUA_TSTRING: {
            const char* str = lua_tostring(L, abs_index);
            if (str) {
                size_t str_len = strlen(str);
                
                /* Validate UTF-8 */
                if (!kh_validate_utf8_string(str, (int)str_len)) {
                    /* Invalid UTF-8 - return empty string or sanitize */
                    strcpy_s(output, output_size, "\"\"");
                    return output;
                }
                
                size_t needed = str_len * 4 + 3;
                
                if (needed > output_size) {
                    free(output);
                    output_size = needed;
                    output = (char*)malloc(output_size);
                    if (!output) return NULL;
                }
                
                output[pos++] = '"';
                
                /* Escape quotes in string */
                for (size_t j = 0; j < str_len; j++) {
                    if (str[j] == '"') {
                        if (pos + 2 < output_size) {
                            output[pos++] = '"';
                            output[pos++] = '"';
                        }
                    } else {
                        if (pos < output_size - 1) {
                            output[pos++] = str[j];
                        }
                    }
                }
                
                output[pos++] = '"';
                output[pos] = '\0';
                return output;
            }
            break;
        }
            
        case LUA_TTABLE: {
            /* Convert table to string representation using dynamic allocation */
            if (lua_table_to_arma_recursive(L, abs_index, &output, &output_size, &pos, 0)) {
                output[pos] = '\0';
                return output;
            }
            break;
        }
            
        default:
            strcpy_s(output, output_size, "\"unknown\"");
            return output;
    }
    
    free(output);
    return NULL;
}

/* Get variable value and type in Arma format [type, value] */
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
    
    if (!lua_isnil(L, -1)) {
        /* Format the value using dynamic allocation */
        char* formatted_value = lua_format_single_value(L, -1);
        
        if (formatted_value) {
            /* Calculate size needed for [type, value] format */
            size_t type_len = strlen(var->type);
            size_t value_len = strlen(formatted_value);
            size_t total_needed = type_len + value_len + 10; /* ["type",value] plus some extra */
            
            if (total_needed < (size_t)output_size) {
                /* Format as [type, value] */
                if (_snprintf_s(output, output_size, _TRUNCATE, "[\"%s\",%s]", 
                              var->type, formatted_value) >= 0) {
                    result = 1;
                } else {
                    kh_set_error(output, output_size, "OUTPUT FORMATTING FAILED");
                }
            } else {
                /* Try to fit what we can in the output buffer */
                /* This is a fallback for when the dynamic result is too large */
                kh_set_error(output, output_size, "OUTPUT BUFFER TOO SMALL");
            }
            
            free(formatted_value);
        } else {
            kh_set_error(output, output_size, "RESULT FORMAT FAILED");
        }
    } else {
        /* Variable not in Lua state, return stored value with type */
        /* Calculate size needed */
        size_t type_len = strlen(var->type);
        size_t value_len = strlen(var->original_value);
        size_t total_needed = type_len + value_len + 10;
        
        if (total_needed < (size_t)output_size) {
            if (_snprintf_s(output, output_size, _TRUNCATE, "[\"%s\",\"%s\"]", 
                          var->type, var->original_value) >= 0) {
                result = 1;
            } else {
                kh_set_error(output, output_size, "OUTPUT FORMATTING FAILED");
            }
        } else {
            kh_set_error(output, output_size, "OUTPUT BUFFER TOO SMALL");
        }
    }
    
    /* Clean up */
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

/* Lua-callable function: SetPersistentVariable(name, value, type) */
static int lua_c_set_persistent_variable(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 2) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
        }
        return 1;
    }
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    const char* name = lua_tostring(L, 1);
    const char* type_value_array = lua_tostring(L, 2);
    
    if (!name || !type_value_array) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    
    if (!clean_name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
    int success = 0;
    if (strlen(clean_name) > 0 && kh_validate_variable_name(clean_name)) {
        success = lua_set_variable(clean_name, type_value_array);
    }
    
    free(clean_name);
    lua_pushboolean(L, success);
    return 1;
}

/* Lua-callable function: PromoteGlobalToPersistent(name) */
static int lua_c_promote_global_to_persistent(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 1) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
        }
        return 1;
    }
    
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Get the global variable */
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 0);
        return 1;
    }
    
    /* Determine type and create type-value array */
    int lua_type_val = lua_type(L, -1);
    const char* type_str = NULL;
    char* value_buffer = NULL;
    size_t value_buffer_size = 256;  /* Start with reasonable size */
    size_t pos = 0;
    int success = 0;
    
    switch (lua_type_val) {
        case LUA_TBOOLEAN:
            type_str = "BOOL";
            value_buffer = (char*)malloc(16);
            if (value_buffer) {
                _snprintf_s(value_buffer, 16, _TRUNCATE, 
                           "%s", lua_toboolean(L, -1) ? "true" : "false");
                success = 1;
            }
            break;
            
        case LUA_TNUMBER:
            type_str = "SCALAR";
            value_buffer = (char*)malloc(64);
            if (value_buffer) {
                _snprintf_s(value_buffer, 64, _TRUNCATE, 
                           "%.15g", lua_tonumber(L, -1));
                success = 1;
            }
            break;
            
        case LUA_TSTRING: {
            type_str = "STRING";
            const char* str = lua_tostring(L, -1);
            if (str) {
                size_t str_len = strlen(str);
                value_buffer = (char*)malloc(str_len + 1);
                if (value_buffer) {
                    strcpy_s(value_buffer, str_len + 1, str);
                    success = 1;
                }
            }
            break;
        }
            
        case LUA_TTABLE:
            if (lua_is_array(L, -1)) {
                type_str = "ARRAY";
            } else {
                type_str = "HASHMAP";
            }
            value_buffer = (char*)malloc(value_buffer_size);
            if (value_buffer) {
                success = lua_table_to_arma_recursive(L, -1, &value_buffer, 
                                                     &value_buffer_size, &pos, 0);
                if (success) {
                    value_buffer[pos] = '\0';
                }
            }
            break;
            
        default:
            success = 0;
            break;
    }
    
    lua_pop(L, 1); /* Remove the global value from stack */
    
    if (success && type_str && value_buffer) {
        /* Create type-value array */
        size_t type_value_len = strlen(type_str) + strlen(value_buffer) + 10;
        char* type_value_array = (char*)malloc(type_value_len);
        
        if (type_value_array) {
            _snprintf_s(type_value_array, type_value_len, _TRUNCATE,
                       "[\"%s\",\"%s\"]", type_str, value_buffer);
            
            /* Set as persistent variable */
            success = lua_set_variable(name, type_value_array);
            free(type_value_array);
        } else {
            success = 0;
        }
    }
    
    free(value_buffer);
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

/* Lua-callable function: WriteKHData(filename, variable_name, type_value_array) */
static int lua_c_write_khdata(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 3) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "REQUIRES 3 ARGUMENTS");
        }
        return 2;
    }
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    const char* filename = lua_tostring(L, 1);
    const char* variable_name = lua_tostring(L, 2);
    const char* type_value_array = lua_tostring(L, 3);
    
    if (!filename || !variable_name || !type_value_array) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "INVALID PARAMETERS");
        return 2;
    }
    
    /* Allocate output buffer for error messages */
    char output[1024];
    output[0] = '\0';
    
    /* Call the existing KHData write function */
    int result = kh_write_khdata_variable(filename, variable_name, type_value_array, output, sizeof(output));
    
    if (result == 0) {
        /* Success */
        lua_pushboolean(L, 1);
        lua_pushnil(L);
    } else {
        /* Error */
        lua_pushboolean(L, 0);
        lua_pushstring(L, output);
    }
    
    return 2;
}

/* Lua-callable function: ReadKHData(filename, variable_name) */
static int lua_c_read_khdata(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 2) {
        if (lua_checkstack(L, 1)) {
            lua_pushnil(L);
            lua_pushstring(L, "REQUIRES 2 ARGUMENTS");
        }
        return 2;
    }
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    const char* filename = lua_tostring(L, 1);
    const char* variable_name = lua_tostring(L, 2);
    
    if (!filename || !variable_name) {
        lua_pushnil(L);
        lua_pushstring(L, "INVALID PARAMETERS");
        return 2;
    }
    
    /* Clean inputs */
    char* clean_filename = (char*)malloc(strlen(filename) + 1);
    char* clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    
    if (!clean_filename || !clean_var_name) {
        free(clean_filename);
        free(clean_var_name);
        lua_pushnil(L);
        lua_pushstring(L, "MEMORY ALLOCATION FAILED");
        return 2;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        free(clean_filename);
        free(clean_var_name);
        lua_pushnil(L);
        lua_pushstring(L, "FILE NOT FOUND OR LOAD FAILED");
        return 2;
    }
    
    /* Find file in memory */
    khdata_file_t* file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        free(clean_filename);
        free(clean_var_name);
        lua_pushnil(L);
        lua_pushstring(L, "FILE NOT FOUND IN MEMORY");
        return 2;
    }
    
    /* Special case: return all variable names if var_name == filename */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        lua_newtable(L);
        for (int i = 0; i < file->variable_count; i++) {
            lua_pushinteger(L, i + 1);
            lua_pushstring(L, file->variables[i].name);
            lua_settable(L, -3);
        }
        free(clean_filename);
        free(clean_var_name);
        lua_pushnil(L); /* No error */
        return 2;
    }
    
    /* Find specific variable */
    int var_index = kh_find_variable_indexed(file->variables, file->variable_count, clean_var_name, file->hash_info.entries, file->hash_info.size);
    if (var_index == -1) {
        free(clean_filename);
        free(clean_var_name);
        lua_pushnil(L);
        lua_pushstring(L, "VARIABLE NOT FOUND");
        return 2;
    }
    
    /* Parse the value and push it onto the Lua stack */
    const char* value = file->variables[var_index].value;
    kh_type_t type = file->variables[var_index].type;
    
    switch (type) {
        case KH_TYPE_BOOL: {
            int bool_val = kh_validate_bool_value(value);
            lua_pushboolean(L, bool_val);
            break;
        }
        
        case KH_TYPE_SCALAR: {
            char* endptr;
            double num = strtod(value, &endptr);
            if (*endptr == '\0' && !isnan(num) && !isinf(num)) {
                lua_pushnumber(L, num);
            } else {
                lua_pushnil(L);
            }
            break;
        }
        
        case KH_TYPE_ARRAY:
        case KH_TYPE_HASHMAP:
        case KH_TYPE_TYPED_ARRAY:
        case KH_TYPE_TYPED_HASHMAP: {
            /* Parse complex types */
            const char* end = value + strlen(value);
            const char* result = lua_parse_value_recursive(L, value, end, 0);
            if (!result) {
                lua_pushnil(L);
            }
            break;
        }
        
        case KH_TYPE_NIL:
            lua_pushnil(L);
            break;
            
        default:
            /* String types */
            lua_pushstring(L, value);
            break;
    }
    
    free(clean_filename);
    free(clean_var_name);
    lua_pushnil(L); /* No error */
    return 2;
}

/* Lua-callable function: FlushKHData */
static int lua_c_flush_khdata(lua_State* L) {
    char output[1024];
    output[0] = '\0';
    
    int result = kh_flush_khdata(output, sizeof(output));
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    if (result == 0) {
        /* Success */
        lua_pushboolean(L, 1);
        lua_pushstring(L, output);
    } else {
        /* Error */
        lua_pushboolean(L, 0);
        lua_pushstring(L, output);
    }
    
    return 2;
}

/* Lua-callable function: DeleteKHDataVariable(filename, variable_name) */
static int lua_c_delete_khdata_variable(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 2) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "REQUIRES 2 ARGUMENTS");
        }
        return 2;
    }
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    const char* filename = lua_tostring(L, 1);
    const char* variable_name = lua_tostring(L, 2);
    
    if (!filename || !variable_name) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "INVALID PARAMETERS");
        return 2;
    }
    
    /* Allocate output buffer for error messages */
    char output[1024];
    output[0] = '\0';
    
    /* Call the existing delete function */
    int result = kh_delete_khdata_variable(filename, variable_name, output, sizeof(output));
    
    if (result == 0) {
        /* Success */
        lua_pushboolean(L, 1);
        lua_pushnil(L);
    } else {
        /* Error */
        lua_pushboolean(L, 0);
        lua_pushstring(L, output);
    }
    
    return 2;
}

/* Lua-callable function: DeleteKHDataFile(filename) */
static int lua_c_delete_khdata_file(lua_State* L) {
    int argc = lua_gettop(L);
    if (argc != 1) {
        if (lua_checkstack(L, 1)) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "REQUIRES 1 ARGUMENT");
        }
        return 2;
    }
    
    if (!lua_checkstack(L, 2)) {
        return 0;
    }
    
    const char* filename = lua_tostring(L, 1);
    
    if (!filename) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "INVALID PARAMETER");
        return 2;
    }
    
    /* Allocate output buffer for error messages */
    char output[1024];
    output[0] = '\0';
    
    /* Call the existing delete file function */
    int result = kh_delete_khdata_file(filename, output, sizeof(output));
    
    if (result == 0) {
        /* Success */
        lua_pushboolean(L, 1);
        lua_pushnil(L);
    } else {
        /* Error */
        lua_pushboolean(L, 0);
        lua_pushstring(L, output);
    }
    
    return 2;
}

/* LuaClearVariables operation - Clear all persistent Lua variables */
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
                   g_lua_variable_storage.hash_table.size * sizeof(kh_generic_hash_entry_t));
        }
        g_lua_variable_storage.hash_table.used_count = 0;
        g_lua_variable_storage.hash_table.deleted_count = 0;
        g_lua_variable_storage.hash_table.needs_rebuild = 0;
    }
    
    _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\",%u,%zu]", cleared_count, memory_freed);
    return 0;
}

/* Set persistent Lua variable */
static int kh_process_lua_set_variable_operation(char* output, int output_size, 
                                                const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc != 2 || !argv || !argv[0] || !argv[1]) {
        kh_set_error(output, output_size, "REQUIRES 2 ARGUMENTS: [NAME, TYPE_VALUE_ARRAY]");
        return 1;
    }
    
    const char* name = argv[0];
    const char* type_value_array = argv[1];
    
    size_t name_len = strlen(name);
    char* clean_name = (char*)malloc(name_len + 1);
    if (!clean_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(name, clean_name, (int)name_len + 1);
    
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
    
    /* Set the variable */
    if (lua_set_variable(clean_name, type_value_array)) {
        _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\"]");
        free(clean_name);
        return 0;
    } else {
        free(clean_name);
        kh_set_error(output, output_size, "FAILED TO SET VARIABLE");
        return 1;
    }
}

/* Get persistent Lua variable with type */
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

/* Delete persistent Lua variable */
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

/* Print function for security */
static int lua_secure_print_fast(lua_State* L) {
    /* Consume all arguments but produce no output for security */
    int n = lua_gettop(L);
    lua_pop(L, n);
    return 0;
}

/* Lua state initialization with Lua-callable functions registration */
static int lua_init_turbo_state(lua_State* L) {
    if (!L) return 0;
    
    /* Only reserve what we actually need for this function */
    if (!lua_checkstack(L, 8)) return 0;
    
    /* Load essential libraries */
    luaopen_base(L);
    luaopen_math(L);     
    luaopen_string(L);   
    luaopen_table(L);
    luaopen_bit(L);
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
    
    /* Use single stack operation for all nil assignments */
    lua_pushnil(L);
    for (int i = 0; dangerous[i]; i++) {
        lua_pushvalue(L, -1);  /* Duplicate nil */
        lua_setglobal(L, dangerous[i]);
    }
    lua_pop(L, 1);  /* Remove original nil */
    
    /* Register functions in batch */
    const struct luaL_Reg kh_functions[] = {
        {"GetPersistentVariable", lua_c_get_persistent_variable},
        {"SetPersistentVariable", lua_c_set_persistent_variable},
        {"DeletePersistentVariable", lua_c_delete_persistent_variable},
        {"PromoteGlobalToPersistent", lua_c_promote_global_to_persistent},
        {"WriteKHData", lua_c_write_khdata},
        {"ReadKHData", lua_c_read_khdata},
        {"DeleteKHDataVariable", lua_c_delete_khdata_variable},
        {"DeleteKHDataFile", lua_c_delete_khdata_file},
        {"FlushKHData", lua_c_flush_khdata},
        {"print", lua_secure_print_fast},
        {NULL, NULL}
    };
    
    /* Register all functions at once */
    for (const struct luaL_Reg* func = kh_functions; func->name; func++) {
        lua_pushcfunction(L, func->func);
        lua_setglobal(L, func->name);
    }
    
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
        
        /* IMPORTANT: Re-inject all persistent functions and variables */
        lua_inject_all_functions(g_lua_state.L);
        lua_inject_all_variables(g_lua_state.L);
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
    
    /* Get the new state - this will automatically re-inject persistent data */
    lua_single_state_t* state = lua_get_single_state();
    
    if (!state || !state->L) {
        kh_set_error(output, output_size, "FAILED TO RESET LUA STATE");
        return 1;
    }
    
    /* Report success with info about restored data */
    _snprintf_s(output, output_size, _TRUNCATE, 
                "[\"SUCCESS\",%u,%u]", 
                g_lua_function_storage.function_count, 
                g_lua_variable_storage.variable_count);
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
    
    /* Clean up persistent Lua functions */
    if (g_lua_function_storage.initialized) {
        for (uint32_t i = 0; i < g_lua_function_storage.function_count; i++) {
            free(g_lua_function_storage.functions[i].name);
            free(g_lua_function_storage.functions[i].code);
        }
        free(g_lua_function_storage.functions);
        free(g_lua_function_storage.hash_table.entries);
        memset(&g_lua_function_storage, 0, sizeof(g_lua_function_storage));
    }
    
    /* Clean up single Lua state */
    if (g_lua_state.L) {
        lua_close(g_lua_state.L);
    }
    memset(&g_lua_state, 0, sizeof(g_lua_state));
    g_lua_initialized = 0;
}

/* Depth-limited array validation */
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

/* Hashmap conversion with error checking */
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

/* Arma array parsing with optimized stack reservations */
static const char* lua_parse_arma_array(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr) return NULL;
    
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
    while (ptr < end && *ptr != ']') {
        /* Skip whitespace and commas */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) {
            ptr++;
            parse_operations++;
        }
        if (ptr >= end || *ptr == ']') break;
        
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

/* Recursive value parsing with optimized memory usage for small strings */
static const char* lua_parse_value_recursive(lua_State* L, const char* ptr, const char* end, int depth) {
    if (!L || !ptr || ptr >= end) return NULL;
    
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
    
    /* Always use dynamic allocation */
    char* value_buffer = (char*)malloc(value_len + 1);
    if (!value_buffer) return NULL;
    
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
        free(value_buffer);
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
        /* String literal - remove quotes and validate UTF-8 */
        trimmed[trimmed_len-1] = '\0';
        const char* str_content = trimmed + 1;
        
        if (kh_validate_utf8_string(str_content, (int)strlen(str_content))) {
            lua_pushstring(L, str_content);
        } else {
            /* Invalid UTF-8 - push empty string */
            lua_pushstring(L, "");
        }
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
    free(value_buffer);
    
    return result_ptr;
}

/* Parse arguments and push them directly onto the Lua stack */
static int lua_parse_args_to_stack(lua_State* L, const char* args_str) {
    if (!args_str || !L) return 0;
    
    size_t args_len = strlen(args_str);
    if (args_len == 0) return 0;
    
    /* Find array start */
    const char* ptr = args_str;
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    if (!*ptr || *ptr != '[') return 0;
    
    /* Reserve stack space */
    if (!lua_checkstack(L, 10)) return -1;
    int stack_base = lua_gettop(L);
    
    const char* end = args_str + args_len;
    int arg_count = 0;
    
    /* Skip opening bracket */
    ptr++;
    
    /* Skip whitespace */
    while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    
    /* Handle empty array */
    if (ptr < end && *ptr == ']') return 0;
    
    /* Parse each [type, value] element and push directly to stack */
    while (ptr < end && *ptr != ']') {
        /* Skip whitespace and commas */
        while (ptr < end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
        if (ptr >= end || *ptr == ']') break;

        /* Dynamically ensure we have stack space for next argument */
        if (arg_count % 10 == 0) {
            if (!lua_checkstack(L, 10)) {
                lua_settop(L, stack_base);
                return -1;
            }
        }
                
        /* Each argument must be a [type, value] array */
        if (*ptr != '[') {
            lua_settop(L, stack_base);
            return -1;
        }
        
        /* Find the end of this [type, value] pair */
        const char* pair_start = ptr;
        int bracket_depth = 0;
        int in_string = 0;
        char string_quote = 0;
        
        while (ptr < end) {
            if (!in_string) {
                if (*ptr == '"' || *ptr == '\'') {
                    in_string = 1;
                    string_quote = *ptr;
                } else if (*ptr == '[') {
                    bracket_depth++;
                } else if (*ptr == ']') {
                    bracket_depth--;
                    if (bracket_depth == 0) {
                        ptr++;
                        break;
                    }
                }
            } else {
                /* Only handle doubled quotes */
                if (*ptr == string_quote) {
                    /* Check for doubled quote */
                    if (ptr + 1 < end && *(ptr + 1) == string_quote) {
                        ptr++; /* Skip second quote, stay in string */
                    } else {
                        in_string = 0;
                    }
                }
            }
            ptr++;
        }
        
        /* Extract and parse the [type, value] pair */
        size_t pair_len = ptr - pair_start;
        char* pair_str = (char*)malloc(pair_len + 1);
        if (!pair_str) {
            lua_settop(L, stack_base);
            return -1;
        }
        
        memcpy(pair_str, pair_start, pair_len);
        pair_str[pair_len] = '\0';
        
        /* Parse the [type, value] array */
        char* type_str = NULL;
        char* value_str = NULL;
        
        if (!kh_parse_type_value_array(pair_str, &type_str, &value_str)) {
            free(pair_str);
            free(type_str);
            free(value_str);
            lua_settop(L, stack_base);
            return -1;
        }
        
        free(pair_str);
        
        /* Get the type enum */
        kh_type_t type = kh_get_type_from_string(type_str);
        
        /* Destringify the value first (like in process_kh_data) */
        char* destringified_value = NULL;
        if (!kh_destringify_value(type, value_str, &destringified_value)) {
            free(type_str);
            free(value_str);
            lua_settop(L, stack_base);
            return -1;
        }
        
        /* Push value onto Lua stack based on type */
        switch (type) {
            case KH_TYPE_NIL:
                lua_pushnil(L);
                break;
                
            case KH_TYPE_BOOL: {
                /* Use the same validation as in kh_validate_bool_value */
                int bool_val = 0;
                if (destringified_value) {
                    /* Clean and lowercase for comparison */
                    size_t val_len = strlen(destringified_value);
                    char* clean_val = (char*)malloc(val_len + 1);
                    if (!clean_val) {
                        free(type_str);
                        free(value_str);
                        free(destringified_value);
                        lua_settop(L, stack_base);
                        return -1;
                    }
                    kh_clean_string(destringified_value, clean_val, (int)val_len + 1);
                    
                    /* Convert to lowercase */
                    for (char* p = clean_val; *p; p++) {
                        if (*p >= 'A' && *p <= 'Z') *p += 32;
                    }
                    
                    bool_val = (strcmp(clean_val, "true") == 0 || strcmp(clean_val, "1") == 0) ? 1 : 0;
                    free(clean_val);
                }
                lua_pushboolean(L, bool_val);
                break;
            }
                
            case KH_TYPE_SCALAR: {
                char* endptr;
                double num = strtod(destringified_value, &endptr);
                if (*endptr == '\0' && !isnan(num) && !isinf(num)) {
                    lua_pushnumber(L, num);
                } else {
                    lua_pushnumber(L, 0);
                }
                break;
            }
                
            case KH_TYPE_NAMESPACE: {
                /* Normalize namespace value like in process_kh_data */
                char normalized[256];
                if (kh_normalize_namespace_value(destringified_value, normalized, sizeof(normalized))) {
                    lua_pushstring(L, normalized);
                } else {
                    lua_pushstring(L, destringified_value);
                }
                break;
            }
            
            case KH_TYPE_SIDE: {
                /* Normalize side value like in process_kh_data */
                char normalized[256];
                if (kh_normalize_side_value(destringified_value, normalized, sizeof(normalized))) {
                    lua_pushstring(L, normalized);
                } else {
                    lua_pushstring(L, destringified_value);
                }
                break;
            }
                
            case KH_TYPE_STRING:
            case KH_TYPE_TEXT:
            case KH_TYPE_CODE:
            case KH_TYPE_MARKER:
            case KH_TYPE_CONFIG:
            case KH_TYPE_MISSION_CONFIG:
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
            case KH_TYPE_LUA_NAMESPACE_REFERENCE: {
                lua_pushstring(L, destringified_value);
                break;
            }
                
            case KH_TYPE_ARRAY:
            case KH_TYPE_HASHMAP:
            case KH_TYPE_TYPED_ARRAY:
            case KH_TYPE_TYPED_HASHMAP: {
                const char* nested_end = destringified_value + strlen(destringified_value);
                const char* result = lua_parse_value_recursive(L, destringified_value, nested_end, 0);
                if (!result) {
                    lua_pushnil(L);
                }
                break;
            }
                
            default:
                lua_pushstring(L, destringified_value);
                break;
        }
        
        free(type_str);
        free(value_str);
        free(destringified_value);
        arg_count++;
    }
    
    return arg_count;
}

/* Array detection with error handling */
static int lua_is_array(lua_State* L, int stack_index) {
    if (!lua_istable(L, stack_index) || !lua_checkstack(L, 2)) return 0;
    
    lua_pushnil(L);
    int max_index = 0;
    int key_count = 0;
    int has_non_integer_key = 0;
    
    while (lua_next(L, stack_index) != 0) {
        if (lua_type(L, -2) == LUA_TNUMBER) {
            double key = lua_tonumber(L, -2);
            if (key == floor(key) && key >= 1) { /* Reasonable limits */
                int int_key = (int)key;
                if (int_key > max_index) max_index = int_key;
            } else {
                has_non_integer_key = 1;
            }
        } else {
            has_non_integer_key = 1;
        }
        key_count++;
        
        lua_pop(L, 1);
    }
    
    return (!has_non_integer_key && max_index == key_count && key_count > 0);
}

/* Value to string conversion with bounds checking */
static int lua_value_to_arma_string(lua_State* L, int stack_index, char** buffer, 
                                           size_t* buffer_size, size_t* pos, int depth) {
    if (!buffer || !*buffer || !buffer_size || !pos) return 0;
    
    if (!lua_checkstack(L, 3)) return 0;
    
    int abs_stack_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    int type = lua_type(L, abs_stack_index);
    
    /* Pre-check if we need more space (estimate based on type) */
    size_t estimated_needed = 100; /* Default estimate */
    if (type == LUA_TSTRING) {
        size_t str_len = lua_rawlen(L, abs_stack_index);
        estimated_needed = str_len * 2 + 10; /* Account for escaping */
    }
    
    if (*pos + estimated_needed >= *buffer_size) {
        size_t new_size = *buffer_size;
        while (new_size < *pos + estimated_needed) {
            new_size *= 2;
        }
        char* new_buffer = (char*)realloc(*buffer, new_size);
        if (!new_buffer) return 0;
        *buffer = new_buffer;
        *buffer_size = new_size;
    }
    
    switch (type) {
        case LUA_TNIL:
            memcpy(*buffer + *pos, "nil", 3);
            *pos += 3;
            return 1;
            
        case LUA_TBOOLEAN: {
            const char* bool_str = lua_toboolean(L, abs_stack_index) ? "true" : "false";
            size_t bool_len = strlen(bool_str);
            memcpy(*buffer + *pos, bool_str, bool_len);
            *pos += bool_len;
            return 1;
        }
        
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, abs_stack_index);
            
            if (isnan(num) || isinf(num)) {
                memcpy(*buffer + *pos, "nil", 3);
                *pos += 3;
                return 1;
            }
            
            char num_buffer[64];
            int num_len;
            
            if (num == floor(num) && num >= -2147483648.0 && num <= 2147483647.0) {
                num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.0f", num);
            } else {
                num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.15g", num);
            }
            
            if (num_len > 0) {
                memcpy(*buffer + *pos, num_buffer, num_len);
                *pos += num_len;
                return 1;
            }
            break;
        }
        
        case LUA_TSTRING: {
            const char* str = lua_tostring(L, abs_stack_index);
            if (!str) return 0;
            
            size_t str_len = strlen(str);
            
            /* Validate UTF-8 */
            if (!kh_validate_utf8_string(str, (int)str_len)) {
                /* Skip invalid UTF-8 string */
                (*buffer)[(*pos)++] = '"';
                (*buffer)[(*pos)++] = '"';
                return 1;
            }
            
            /* UTF-8 aware: max 4 bytes per char + escaping */
            size_t max_needed = str_len * 4 + 3;
            if (*pos + max_needed >= *buffer_size) {
                size_t new_size = *buffer_size;
                while (new_size < *pos + max_needed) {
                    new_size *= 2;
                }
                char* new_buffer = (char*)realloc(*buffer, new_size);
                if (!new_buffer) return 0;
                *buffer = new_buffer;
                *buffer_size = new_size;
            }
            
            (*buffer)[(*pos)++] = '"';
            
            for (size_t i = 0; i < str_len; i++) {
                if (str[i] == '"') {
                    (*buffer)[(*pos)++] = '"';
                    (*buffer)[(*pos)++] = '"';
                } else {
                    (*buffer)[(*pos)++] = str[i];
                }
            }
            
            (*buffer)[(*pos)++] = '"';
            return 1;
        }
        
        case LUA_TTABLE:
            return lua_table_to_arma_recursive(L, abs_stack_index, buffer, buffer_size, pos, depth + 1);
            
        default: {
            const char* type_str = "[unknown]";
            size_t type_len = strlen(type_str);
            memcpy(*buffer + *pos, type_str, type_len);
            *pos += type_len;
            return 1;
        }
    }
    
    return 0;
}

/* Table to Arma conversion with comprehensive error handling and proper stack management */
static int lua_table_to_arma_recursive(lua_State* L, int stack_index, char** buffer, 
                                              size_t* buffer_size, size_t* pos, int depth) {
    if (!buffer || !*buffer || !buffer_size || !pos) return 0;
    
    if (!lua_checkstack(L, 5)) return 0;
    
    /* Ensure we have space for opening bracket */
    if (*pos + 10 >= *buffer_size) {
        size_t new_size = *buffer_size * 2;
        char* new_buffer = (char*)realloc(*buffer, new_size);
        if (!new_buffer) return 0;
        *buffer = new_buffer;
        *buffer_size = new_size;
    }
    
    /* Normalize stack index to absolute index */
    int abs_stack_index = (stack_index > 0) ? stack_index : lua_gettop(L) + 1 + stack_index;
    
    (*buffer)[(*pos)++] = '[';
    
    if (lua_is_array(L, abs_stack_index)) {
        /* Convert as array */
        int array_length = (int)lua_rawlen(L, abs_stack_index);
        
        for (int i = 1; i <= array_length; i++) {
            if (i > 1) {
                /* Ensure space for comma */
                if (*pos + 10 >= *buffer_size) {
                    size_t new_size = *buffer_size * 2;
                    char* new_buffer = (char*)realloc(*buffer, new_size);
                    if (!new_buffer) return 0;
                    *buffer = new_buffer;
                    *buffer_size = new_size;
                }
                (*buffer)[(*pos)++] = ',';
            }
            
            int stack_before = lua_gettop(L);
            lua_rawgeti(L, abs_stack_index, i);
            int success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            lua_settop(L, stack_before);
            
            if (!success) return 0;
        }
    } else {
        /* Convert as hashmap */
        int initial_stack_top = lua_gettop(L);
        lua_pushnil(L);
        int first_pair = 1;
        int pair_count = 0;
        
        while (lua_next(L, abs_stack_index) != 0) {
            int iteration_stack_top = lua_gettop(L);
            
            if (!first_pair) {
                /* Ensure space for comma */
                if (*pos + 10 >= *buffer_size) {
                    size_t new_size = *buffer_size * 2;
                    char* new_buffer = (char*)realloc(*buffer, new_size);
                    if (!new_buffer) {
                        lua_settop(L, initial_stack_top);
                        return 0;
                    }
                    *buffer = new_buffer;
                    *buffer_size = new_size;
                }
                (*buffer)[(*pos)++] = ',';
            }
            first_pair = 0;
            pair_count++;
            
            /* Start key-value pair array */
            if (*pos + 10 >= *buffer_size) {
                size_t new_size = *buffer_size * 2;
                char* new_buffer = (char*)realloc(*buffer, new_size);
                if (!new_buffer) {
                    lua_settop(L, initial_stack_top);
                    return 0;
                }
                *buffer = new_buffer;
                *buffer_size = new_size;
            }
            (*buffer)[(*pos)++] = '[';
            
            /* Add key */
            int key_success = lua_value_to_arma_string(L, -2, buffer, buffer_size, pos, depth);
            if (!key_success) {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* Add comma */
            if (*pos + 10 >= *buffer_size) {
                size_t new_size = *buffer_size * 2;
                char* new_buffer = (char*)realloc(*buffer, new_size);
                if (!new_buffer) {
                    lua_settop(L, initial_stack_top);
                    return 0;
                }
                *buffer = new_buffer;
                *buffer_size = new_size;
            }
            (*buffer)[(*pos)++] = ',';
            
            /* Add value */
            int value_success = lua_value_to_arma_string(L, -1, buffer, buffer_size, pos, depth);
            if (!value_success) {
                lua_settop(L, initial_stack_top);
                return 0;
            }
            
            /* End key-value pair array */
            if (*pos + 10 >= *buffer_size) {
                size_t new_size = *buffer_size * 2;
                char* new_buffer = (char*)realloc(*buffer, new_size);
                if (!new_buffer) {
                    lua_settop(L, initial_stack_top);
                    return 0;
                }
                *buffer = new_buffer;
                *buffer_size = new_size;
            }
            (*buffer)[(*pos)++] = ']';
            
            /* Remove value, keep key for next iteration */
            lua_pop(L, 1);
        }
        
        /* Restore stack to initial state */
        lua_settop(L, initial_stack_top);
    }
    
    /* Close the array */
    if (*pos + 10 >= *buffer_size) {
        size_t new_size = *buffer_size * 2;
        char* new_buffer = (char*)realloc(*buffer, new_size);
        if (!new_buffer) return 0;
        *buffer = new_buffer;
        *buffer_size = new_size;
    }
    (*buffer)[(*pos)++] = ']';
    
    return 1;
}

/* New function to format only return values from a specific stack position */
static char* lua_format_result_from_position(lua_State* L, int start_pos, int count) {
    if (!L || count < 0) return NULL;
    
    if (!lua_checkstack(L, 3)) return NULL;
    
    char* output = NULL;
    size_t output_size = 256;  /* Start small */
    size_t pos = 0;
    
    output = (char*)malloc(output_size);
    if (!output) return NULL;
    
    /* If no return values, return empty array */
    if (count == 0) {
        strcpy_s(output, output_size, "[]");
        return output;
    }
    
    /* Ensure space for opening bracket */
    if (output_size < 2) {
        output_size = 256;
        char* new_output = (char*)realloc(output, output_size);
        if (!new_output) {
            free(output);
            return NULL;
        }
        output = new_output;
    }
    
    /* Start with opening bracket */
    output[pos++] = '[';
    
    /* Process only the return values from start_pos */
    for (int i = 0; i < count; i++) {
        int stack_index = start_pos + i;
        
        if (i > 0) {
            /* Ensure space for comma */
            if (pos + 10 >= output_size) {
                output_size *= 2;
                char* new_output = (char*)realloc(output, output_size);
                if (!new_output) {
                    free(output);
                    return NULL;
                }
                output = new_output;
            }
            output[pos++] = ',';
        }
        
        /* Ensure space for [type, value] structure */
        if (pos + 20 >= output_size) {
            output_size *= 2;
            char* new_output = (char*)realloc(output, output_size);
            if (!new_output) {
                free(output);
                return NULL;
            }
            output = new_output;
        }
        
        /* Start [type, value] pair */
        output[pos++] = '[';
        
        /* Determine type and add type string */
        int lua_type_val = lua_type(L, stack_index);
        const char* type_str = NULL;
        
        switch (lua_type_val) {
            case LUA_TNIL:
                type_str = "\"NIL\"";
                break;
            case LUA_TBOOLEAN:
                type_str = "\"BOOL\"";
                break;
            case LUA_TNUMBER:
                type_str = "\"SCALAR\"";
                break;
            case LUA_TSTRING:
                type_str = "\"STRING\"";
                break;
            case LUA_TTABLE:
                if (lua_is_array(L, stack_index)) {
                    type_str = "\"ARRAY\"";
                } else {
                    type_str = "\"HASHMAP\"";
                }
                break;
            default:
                type_str = "\"UNKNOWN\"";
                break;
        }
        
        /* Add type string */
        size_t type_len = strlen(type_str);
        
        /* Ensure space for type string and comma */
        if (pos + type_len + 2 >= output_size) {
            output_size = output_size * 2 + type_len + 100;
            char* new_output = (char*)realloc(output, output_size);
            if (!new_output) {
                free(output);
                return NULL;
            }
            output = new_output;
        }
        
        memcpy(output + pos, type_str, type_len);
        pos += type_len;
        output[pos++] = ',';
        
        /* Add value based on type */
        switch (lua_type_val) {
            case LUA_TNIL:
                if (pos + 4 >= output_size) {
                    output_size *= 2;
                    char* new_output = (char*)realloc(output, output_size);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }
                memcpy(output + pos, "nil", 3);
                pos += 3;
                break;
                
            case LUA_TBOOLEAN: {
                const char* bool_str = lua_toboolean(L, stack_index) ? "true" : "false";
                size_t bool_len = strlen(bool_str);
                if (pos + bool_len + 1 >= output_size) {
                    output_size = output_size * 2 + bool_len + 10;
                    char* new_output = (char*)realloc(output, output_size);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }
                memcpy(output + pos, bool_str, bool_len);
                pos += bool_len;
                break;
            }
                
            case LUA_TNUMBER: {
                double num = lua_tonumber(L, stack_index);
                char num_buffer[64];
                int num_len;
                
                if (isnan(num) || isinf(num)) {
                    num_buffer[0] = '0';
                    num_len = 1;
                } else if (num == floor(num) && num >= -2147483648.0 && num <= 2147483647.0) {
                    num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.0f", num);
                } else {
                    num_len = _snprintf_s(num_buffer, sizeof(num_buffer), _TRUNCATE, "%.15g", num);
                }
                
                if (num_len > 0) {
                    if (pos + num_len + 1 >= output_size) {
                        output_size = output_size * 2 + num_len + 10;
                        char* new_output = (char*)realloc(output, output_size);
                        if (!new_output) {
                            free(output);
                            return NULL;
                        }
                        output = new_output;
                    }
                    memcpy(output + pos, num_buffer, num_len);
                    pos += num_len;
                }
                break;
            }
                
            case LUA_TSTRING: {
                const char* str = lua_tostring(L, stack_index);
                if (str) {
                    size_t str_len = strlen(str);
                    /* Need space for quotes and potential doubling of all quotes */
                    size_t max_needed = str_len * 2 + 3;
                    
                    if (pos + max_needed >= output_size) {
                        output_size = output_size * 2 + max_needed;
                        char* new_output = (char*)realloc(output, output_size);
                        if (!new_output) {
                            free(output);
                            return NULL;
                        }
                        output = new_output;
                    }
                    
                    output[pos++] = '"';
                    
                    /* Escape quotes in string */
                    for (size_t j = 0; j < str_len; j++) {
                        if (str[j] == '"') {
                            output[pos++] = '"';
                            output[pos++] = '"';
                        } else {
                            output[pos++] = str[j];
                        }
                    }
                    
                    output[pos++] = '"';
                }
                break;
            }
                
            case LUA_TTABLE: {
                /* Convert table to string representation using dynamic allocation */
                if (!lua_table_to_arma_recursive(L, stack_index, &output, &output_size, &pos, 0)) {
                    free(output);
                    return NULL;
                }
                break;
            }
                
            default:
                /* Add "unknown" */
                const char* unknown_str = "\"unknown\"";
                size_t unknown_len = strlen(unknown_str);
                if (pos + unknown_len + 1 >= output_size) {
                    output_size = output_size * 2 + unknown_len + 10;
                    char* new_output = (char*)realloc(output, output_size);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }
                memcpy(output + pos, unknown_str, unknown_len);
                pos += unknown_len;
                break;
        }
        
        /* Close [type, value] pair */
        if (pos + 2 >= output_size) {
            output_size *= 2;
            char* new_output = (char*)realloc(output, output_size);
            if (!new_output) {
                free(output);
                return NULL;
            }
            output = new_output;
        }
        output[pos++] = ']';
    }
    
    /* Close main array */
    if (pos + 2 >= output_size) {
        output_size *= 2;
        char* new_output = (char*)realloc(output, output_size);
        if (!new_output) {
            free(output);
            return NULL;
        }
        output = new_output;
    }
    output[pos++] = ']';
    output[pos] = '\0';
    
    return output;
}

/* Execution function with proper error handling for argument parsing */
static int kh_execute_lua(const char* arguments, const char* code, 
                          char* output, int output_size) {
    if (!arguments || !code || !output || output_size <= 0) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    /* Process 3 string format: remove outer quotes and un-escape inner quotes */
    char* actual_code = NULL;
    size_t code_len = strlen(code);
    
    if (code_len >= 2 && 
        ((code[0] == '"' && code[code_len - 1] == '"') || 
         (code[0] == '\'' && code[code_len - 1] == '\''))) {
        
        char quote_char = code[0];
        
        /* Allocate buffer for processed code (worst case: same size) */
        actual_code = (char*)malloc(code_len);
        if (!actual_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        
        /* Process the string, skipping outer quotes and un-escaping doubled quotes */
        size_t out_pos = 0;
        for (size_t i = 1; i < code_len - 1; i++) {
            if (code[i] == quote_char && i + 1 < code_len - 1 && code[i + 1] == quote_char) {
                /* Found doubled quote - convert to single */
                actual_code[out_pos++] = quote_char;
                i++; /* Skip the second quote */
            } else {
                /* Regular character */
                actual_code[out_pos++] = code[i];
            }
        }
        actual_code[out_pos] = '\0';
        code_len = out_pos;
    } else {
        /* No outer quotes, use as-is */
        actual_code = (char*)malloc(code_len + 1);
        if (!actual_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        strcpy(actual_code, code);
    }
    
    if (code_len == 0) {
        free(actual_code);
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }

    /* Check if this is a function name (no spaces) */
    int is_function_name = 1;
    for (size_t i = 0; i < code_len; i++) {
        if (actual_code[i] == ' ' || actual_code[i] == '\t' || 
            actual_code[i] == '\n' || actual_code[i] == '\r') {
            is_function_name = 0;
            break;
        }
    }

    lua_single_state_t* state = lua_get_single_state();
    if (!state || !state->L) {
        free(actual_code);
        kh_set_error(output, output_size, "FAILED TO INITIALIZE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    int initial_stack_top = lua_gettop(L);
    
    /* Parse arguments directly onto stack */
    int arg_count = 0;
    size_t args_len = strlen(arguments);
    if (args_len > 0) {
        const char* args_check = arguments;
        while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
        
        if (*args_check == '[') {
            args_check++;
            while (*args_check && (*args_check == ' ' || *args_check == '\t' || *args_check == '\n')) args_check++;
            if (*args_check != ']') {
                arg_count = lua_parse_args_to_stack(L, arguments);
                if (arg_count < 0) {
                    lua_settop(L, initial_stack_top);
                    free(actual_code);
                    kh_set_error(output, output_size, "ARGUMENT PARSING FAILED");
                    return 1;
                }
            }
        }
    }
    
    /* OPTIMIZED PATH: Direct function call without compilation */
    if (is_function_name) {
        /* Validate function name */
        if (!kh_validate_variable_name(actual_code)) {
            lua_settop(L, initial_stack_top);
            free(actual_code);
            kh_set_error(output, output_size, "INVALID FUNCTION NAME");
            return 1;
        }
        
        /* Get the function directly from Lua globals */
        lua_getglobal(L, actual_code);
        if (!lua_isfunction(L, -1)) {
            lua_settop(L, initial_stack_top);
            free(actual_code);
            kh_set_error(output, output_size, "FUNCTION NOT FOUND OR NOT A FUNCTION");
            return 1;
        }
        
        /* Insert function before arguments on stack */
        if (arg_count > 0) {
            lua_insert(L, -(arg_count + 1));
        }
        
        /* Call function with actual arguments */
        if (lua_pcall(L, arg_count, LUA_MULTRET, 0) == LUA_OK) {
            int post_call_top = lua_gettop(L);
            int num_returns = post_call_top - initial_stack_top;
            
            /* Format directly to output buffer */
            char* formatted_result = lua_format_result_from_position(L, initial_stack_top + 1, num_returns);
            if (!formatted_result) {
                kh_set_error(output, output_size, "RESULT FORMAT FAILED");
                /* result remains 1 for error */
            } else {
                strncpy_s(output, output_size, formatted_result, _TRUNCATE);
                free(formatted_result);
                result = 0;  /* Set result to 0 for success */
            }
        } else {
            const char* error_msg = lua_tostring(L, -1);
            char formatted_error[1024];
            _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                        "EXECUTION: %.900s", error_msg ? error_msg : "UNKNOWN ERROR");
            kh_set_error(output, output_size, formatted_error);
            /* result remains 1 for error */
        }
        
        lua_settop(L, initial_stack_top);
        free(actual_code);
        return result;
    }
    
    /* STANDARD PATH: Execute arbitrary Lua code with function wrapping */
    /* Wrap code in anonymous function to receive arguments */
    size_t wrapper_len = code_len + 100;
    char* wrapped_code = (char*)malloc(wrapper_len);
    if (!wrapped_code) {
        lua_settop(L, initial_stack_top);
        free(actual_code);
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    /* Create anonymous function wrapper */
    _snprintf_s(wrapped_code, wrapper_len, _TRUNCATE, 
                "return (function(...) %.*s end)(...)", (int)code_len, actual_code);
    
    if (luaL_loadstring(L, wrapped_code) == LUA_OK) {
        /* Insert loaded chunk before arguments */
        if (arg_count > 0) {
            lua_insert(L, -(arg_count + 1));
        }
        
        if (lua_pcall(L, arg_count, LUA_MULTRET, 0) == LUA_OK) {
            int post_call_top = lua_gettop(L);
            int num_returns = post_call_top - initial_stack_top;
            
            /* Format directly to output buffer */
            char* formatted_result = lua_format_result_from_position(L, initial_stack_top + 1, num_returns);
            if (!formatted_result) {
                kh_set_error(output, output_size, "RESULT FORMAT FAILED");
                /* result remains 1 for error */
            } else {
                strncpy_s(output, output_size, formatted_result, _TRUNCATE);
                free(formatted_result);
                result = 0;  /* Set result to 0 for success */
            }
        } else {
            const char* error_msg = lua_tostring(L, -1);
            char formatted_error[1024];
            _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                        "EXECUTION: %.900s", error_msg ? error_msg : "UNKNOWN ERROR");
            kh_set_error(output, output_size, formatted_error);
        }
    } else {
        const char* error_msg = lua_tostring(L, -1);
        char formatted_error[1024];
        _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                   "COMPILATION: %.900s", error_msg ? error_msg : "SYNTAX ERROR");
        kh_set_error(output, output_size, formatted_error);
    }
    
    free(wrapped_code);
    lua_settop(L, initial_stack_top);
    free(actual_code);
    
    return result;
}

/* Main entry point */
static int kh_process_lua_operation(char* output, int output_size, 
                                   const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 2 || !argv || !argv[0] || !argv[1]) {
        kh_set_error(output, output_size, "REQUIRES 2 ARGUMENTS: [ARGUMENTS, CODE]");
        return 1;
    }
    
    return kh_execute_lua(argv[0], argv[1], output, output_size);
}

/* LuaCompile operation - Validate Lua code syntax without execution */
static int kh_process_lua_compile_operation(char* output, int output_size, 
                                           const char** argv, int argc) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    
    if (argc < 1 || argc > 2 || !argv || !argv[0]) {
        kh_set_error(output, output_size, "REQUIRES 1-2 ARGUMENTS: [CODE] or [CODE, FUNCTION_NAME]");
        return 1;
    }
    
    const char* code = argv[0];
    const char* function_name = (argc >= 2) ? argv[1] : NULL;
    
    /* Process string format: remove outer quotes and un-escape inner quotes */
    char* actual_code = NULL;
    size_t code_len = strlen(code);
    
    if (code_len >= 2 && 
        ((code[0] == '"' && code[code_len - 1] == '"') || 
         (code[0] == '\'' && code[code_len - 1] == '\''))) {
        
        char quote_char = code[0];
        
        /* Allocate buffer for processed code (worst case: same size) */
        actual_code = (char*)malloc(code_len);
        if (!actual_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        
        /* Process the string, skipping outer quotes and un-escaping doubled quotes */
        size_t out_pos = 0;
        for (size_t i = 1; i < code_len - 1; i++) {
            if (code[i] == quote_char && i + 1 < code_len - 1 && code[i + 1] == quote_char) {
                /* Found doubled quote - convert to single */
                actual_code[out_pos++] = quote_char;
                i++; /* Skip the second quote */
            } else {
                /* Regular character */
                actual_code[out_pos++] = code[i];
            }
        }
        actual_code[out_pos] = '\0';
        code_len = out_pos;
    } else {
        /* No outer quotes, use as-is */
        actual_code = (char*)malloc(code_len + 1);
        if (!actual_code) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        strcpy(actual_code, code);
    }
    
    if (code_len == 0) {
        free(actual_code);
        kh_set_error(output, output_size, "CODE CANNOT BE EMPTY");
        return 1;
    }
    
    /* Clean and validate function name if provided */
    char* clean_name = NULL;
    if (function_name && strlen(function_name) > 0) {
        size_t name_len = strlen(function_name);
        clean_name = (char*)malloc(name_len + 1);
        if (!clean_name) {
            free(actual_code);
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        
        kh_clean_string(function_name, clean_name, (int)name_len + 1);
        
        if (strlen(clean_name) == 0) {
            free(clean_name);
            free(actual_code);
            kh_set_error(output, output_size, "FUNCTION NAME CANNOT BE EMPTY AFTER CLEANING");
            return 1;
        }
        
        if (!kh_validate_variable_name(clean_name)) {
            free(clean_name);
            free(actual_code);
            kh_set_error(output, output_size, "INVALID FUNCTION NAME");
            return 1;
        }
    }
    
    lua_single_state_t* state = lua_get_single_state();
    if (!state || !state->L) {
        free(clean_name);
        free(actual_code);
        kh_set_error(output, output_size, "FAILED TO INITIALIZE LUA STATE");
        return 1;
    }
    
    lua_State* L = state->L;
    int result = 1;
    int initial_stack_top = lua_gettop(L);
    
    if (clean_name) {
        /* Test direct compilation first */
        int direct_compile = luaL_loadstring(L, actual_code);
        
        if (direct_compile == LUA_OK) {
            lua_pop(L, 1);
            
            /* Now wrap in function */
            size_t wrapper_len = code_len + strlen(clean_name) + 64;
            char* test_wrapper = (char*)malloc(wrapper_len);
            
            if (!test_wrapper) {
                free(clean_name);
                free(actual_code);
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                return 1;
            }
            
            int wrapper_result = _snprintf_s(test_wrapper, wrapper_len, _TRUNCATE, 
                                            "function %s()\n%s\nend", clean_name, actual_code);
            
            if (wrapper_result < 0) {
                free(clean_name);
                free(test_wrapper);
                free(actual_code);
                kh_set_error(output, output_size, "FUNCTION WRAPPER CREATION FAILED");
                return 1;
            }
            
            if (luaL_loadstring(L, test_wrapper) == LUA_OK) {
                if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
                    lua_getglobal(L, clean_name);
                    if (lua_isfunction(L, -1)) {
                        lua_pop(L, 1);
                        
                        /* Store the processed code */
                        if (lua_set_function(clean_name, actual_code)) {
                            _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\",\"STORED\"]");
                            result = 0;
                        } else {
                            kh_set_error(output, output_size, "FAILED TO STORE FUNCTION");
                        }
                    } else {
                        kh_set_error(output, output_size, "FUNCTION CREATION FAILED");
                        lua_pop(L, 1);
                    }
                } else {
                    const char* error_msg = lua_tostring(L, -1);
                    char formatted_error[1024];
                    _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                               "EXECUTION: %.900s", error_msg ? error_msg : "UNKNOWN ERROR");
                    kh_set_error(output, output_size, formatted_error);
                }
            } else {
                const char* error_msg = lua_tostring(L, -1);
                char formatted_error[1024];
                _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                           "WRAPPED COMPILATION: %.900s", error_msg ? error_msg : "SYNTAX ERROR");
                kh_set_error(output, output_size, formatted_error);
            }
            
            free(test_wrapper);
        } else {
            const char* error_msg = lua_tostring(L, -1);
            char formatted_error[1024];
            _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                       "DIRECT COMPILATION: %.900s", error_msg ? error_msg : "SYNTAX ERROR");
            kh_set_error(output, output_size, formatted_error);
        }
    } else {
        /* Anonymous - just validate syntax */
        if (luaL_loadstring(L, actual_code) == LUA_OK) {
            lua_pop(L, 1);
            _snprintf_s(output, output_size, _TRUNCATE, "[\"SUCCESS\",\"SYNTAX_VALID\"]");
            result = 0;
        } else {
            const char* error_msg = lua_tostring(L, -1);
            char formatted_error[1024];
            _snprintf_s(formatted_error, sizeof(formatted_error), _TRUNCATE, 
                       "COMPILATION: %.900s", error_msg ? error_msg : "SYNTAX ERROR");
            kh_set_error(output, output_size, formatted_error);
        }
    }
    
    lua_settop(L, initial_stack_top);
    free(clean_name);
    free(actual_code);
    
    return result;
}

#endif /* LUA_INTEGRATION_H */