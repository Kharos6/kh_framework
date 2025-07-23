#ifndef ARRAY_OPERATIONS_HPP
#define ARRAY_OPERATIONS_HPP

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

/* Array element data structure */
typedef struct {
    int type;           /* KH_TYPE_* from common_defines.h */
    char* value;        /* String representation of the value */
    size_t value_len;   /* Length of value string */
    double number;      /* Cached numeric value for SCALAR types */
    int is_valid;       /* Whether this element is valid */
} array_element_t;

/* Dynamic array structure */
typedef struct {
    array_element_t* elements;
    int count;
    int capacity;
    int allocation_failed;
} dynamic_array_t;

/* Array function types for dispatch table */
typedef enum {
    ARRAY_FUNC_ARRAY_TO_SCALAR,     /* Array -> Scalar */
    ARRAY_FUNC_ARRAY_TO_BOOL,       /* Array -> Boolean */
    ARRAY_FUNC_ARRAY_TO_ARRAY,      /* Array -> Array */
    ARRAY_FUNC_ARRAY_TO_STRING,     /* Array -> String */
    ARRAY_FUNC_ARRAYARRAY_TO_ARRAY, /* Array, Array -> Array */
    ARRAY_FUNC_ARRAYARRAY_TO_BOOL,  /* Array, Array -> Boolean */
    ARRAY_FUNC_ARRAYVALUE_TO_ARRAY, /* Array, Value -> Array */
    ARRAY_FUNC_SPECIAL              /* Special handling required */
} array_func_type_t;

/* Array function definition structure */
typedef struct {
    const char* name;
    array_func_type_t type;
    int arg_count;
    void* func_ptr;
    const char* description;
} array_function_t;

/* Forward declarations for KH_TYPE constants from process_kh_data.h */
typedef enum {
    KH_TYPE_ARRAY_LOCAL = 0,
    KH_TYPE_STRING_LOCAL = 1,
    KH_TYPE_SCALAR_LOCAL = 2,
    KH_TYPE_HASHMAP_LOCAL = 3,
    KH_TYPE_BOOL_LOCAL = 4,
    KH_TYPE_UNKNOWN_LOCAL = -1
} kh_data_type_local_t;

/* Initialize dynamic array */
static inline int kh_init_dynamic_array(dynamic_array_t* arr, int initial_capacity) {
    if (!arr || initial_capacity <= 0) return 0;
    
    memset(arr, 0, sizeof(dynamic_array_t));
    
    arr->elements = (array_element_t*)calloc((size_t)initial_capacity, sizeof(array_element_t));
    if (!arr->elements) {
        arr->allocation_failed = 1;
        return 0;
    }
    
    arr->capacity = initial_capacity;
    arr->count = 0;
    arr->allocation_failed = 0;
    return 1;
}

/* Resize dynamic array if needed */
static inline int kh_resize_dynamic_array(dynamic_array_t* arr, int new_capacity) {
    if (!arr || new_capacity <= 0 || arr->allocation_failed) return 0;
    
    if (new_capacity <= arr->capacity) return 1;
    
    /* Prevent excessive memory allocation */
    if (new_capacity > KH_ARRAY_MAX_ELEMENTS) {
        arr->allocation_failed = 1;
        return 0;
    }
    
    array_element_t* new_elements = (array_element_t*)realloc(arr->elements, 
                                                              (size_t)new_capacity * sizeof(array_element_t));
    if (!new_elements) {
        arr->allocation_failed = 1;
        return 0;
    }
    
    /* Initialize new elements */
    for (int i = arr->capacity; i < new_capacity; i++) {
        memset(&new_elements[i], 0, sizeof(array_element_t));
    }
    
    arr->elements = new_elements;
    arr->capacity = new_capacity;
    return 1;
}

/* Free dynamic array */
static inline void kh_free_dynamic_array(dynamic_array_t* arr) {
    if (!arr) return;
    
    if (arr->elements) {
        for (int i = 0; i < arr->count; i++) {
            free(arr->elements[i].value);
        }
        free(arr->elements);
        arr->elements = NULL;
    }
    
    arr->count = 0;
    arr->capacity = 0;
    arr->allocation_failed = 0;
}

/* Add element to dynamic array */
static inline int kh_add_array_element(dynamic_array_t* arr, int type, const char* value) {
    if (!arr || !value || arr->allocation_failed) return 0;
    
    /* Resize if needed */
    if (arr->count >= arr->capacity) {
        int new_capacity = arr->capacity * 2;
        if (new_capacity > KH_ARRAY_MAX_ELEMENTS) new_capacity = KH_ARRAY_MAX_ELEMENTS;
        
        if (!kh_resize_dynamic_array(arr, new_capacity)) {
            return 0;
        }
    }
    
    if (arr->count >= arr->capacity) return 0;
    
    array_element_t* elem = &arr->elements[arr->count];
    
    /* Set type */
    elem->type = type;
    
    /* Copy value */
    elem->value_len = strlen(value);
    elem->value = (char*)malloc(elem->value_len + 1);
    if (!elem->value) {
        arr->allocation_failed = 1;
        return 0;
    }
    
    strcpy_s(elem->value, elem->value_len + 1, value);
    
    /* Cache numeric value for scalars */
    if (type == KH_TYPE_SCALAR_LOCAL) {
        char* endptr;
        elem->number = strtod(value, &endptr);
        
        // Validate conversion was successful
        if (endptr == value || *endptr != '\0') {
            elem->number = 0.0;  // Default for invalid numbers
        }
    }
    
    elem->is_valid = 1;
    arr->count++;
    
    return 1;
}

/* Parse a sub-array and count its elements */
static inline int kh_count_array_elements(const char* array_str) {
    if (!array_str) return -1;
    
    const char* ptr = array_str;
    int bracket_depth = 0;
    int in_quotes = 0;
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
        
        if (c == '"') {
            in_quotes = !in_quotes;
            has_content = 1;
        }
        
        if (!in_quotes) {
            if (c == '[') {
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
        }
        
        ptr++;
    }
    
    return (bracket_depth == 0) ? element_count : -1;
}

/* Check if string represents a valid hashmap (array of key-value pairs) */
static inline int kh_is_valid_hashmap(const char* input) {
    if (!input) return 0;
    
    const char* ptr = input;
    int bracket_depth = 0;
    int in_quotes = 0;
    int parse_operations = 0;
    
    /* Skip leading whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Must start with opening bracket */
    if (*ptr != '[') return 0;
    ptr++; /* Skip opening bracket */
    bracket_depth = 1;
    
    /* Skip whitespace after opening bracket */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Empty arrays are not hashmaps */
    if (*ptr == ']') return 0;
    
    const char* element_start = NULL;
    int element_bracket_depth = 0;
    
    while (*ptr && bracket_depth > 0 && parse_operations < KH_ARRAY_PARSE_OPERATIONS_LIMIT) {
        parse_operations++;
        char c = *ptr;
        
        if (c == '"') {
            in_quotes = !in_quotes;
        }
        
        if (!in_quotes) {
            if (c == '[') {
                if (element_start == NULL) {
                    element_start = ptr; /* Start of sub-array */
                }
                bracket_depth++;
                element_bracket_depth++;
            } else if (c == ']') {
                bracket_depth--;
                element_bracket_depth--;
                
                /* End of array or end of nested element */
                if (bracket_depth == 0 || (element_bracket_depth == 0 && element_start != NULL)) {
                    if (element_start != NULL) {
                        /* Extract and validate sub-array */
                        int element_len = (int)(ptr - element_start + 1);
                        
                        char* element_str = (char*)malloc((size_t)element_len + 1);
                        if (!element_str) return 0;
                        
                        memcpy(element_str, element_start, (size_t)element_len);
                        element_str[element_len] = '\0';
                        
                        /* Check if this sub-array has exactly 2 elements */
                        int sub_element_count = kh_count_array_elements(element_str);
                        free(element_str);
                        
                        if (sub_element_count != 2) {
                            return 0; /* Not a valid key-value pair */
                        }
                        
                        element_start = NULL;
                        element_bracket_depth = 0;
                    }
                }
            } else if (c == ',' && bracket_depth == 1) {
                /* Element separator at top level */
                if (element_start != NULL) {
                    /* Extract and validate sub-array */
                    int element_len = (int)(ptr - element_start);
                    
                    char* element_str = (char*)malloc((size_t)element_len + 1);
                    if (!element_str) return 0;
                    
                    memcpy(element_str, element_start, (size_t)element_len);
                    element_str[element_len] = '\0';
                    
                    /* Check if this sub-array has exactly 2 elements */
                    int sub_element_count = kh_count_array_elements(element_str);
                    free(element_str);
                    
                    if (sub_element_count != 2) {
                        return 0; /* Not a valid key-value pair */
                    }
                }
                
                /* Skip comma and whitespace to find next element */
                ptr++;
                while (*ptr && isspace(*ptr)) ptr++;
                element_start = (*ptr != '\0' && bracket_depth > 0) ? ptr : NULL;
                element_bracket_depth = 0;
                continue;
            } else if (element_start == NULL && !isspace(c)) {
                /* Found non-array element at top level */
                if (c != '[') {
                    return 0; /* Top-level element is not an array */
                }
                element_start = ptr;
                element_bracket_depth = 0;
            }
        }
        
        ptr++;
    }
    
    /* Must have proper bracket matching and at least one valid key-value pair */
    return (bracket_depth == 0 && parse_operations < KH_ARRAY_PARSE_OPERATIONS_LIMIT) ? 1 : 0;
}

/* Detect element type from string value */
static inline int kh_detect_element_type(const char* value) {
    if (!value) return KH_TYPE_UNKNOWN_LOCAL;
    
    const char* trimmed = value;
    
    /* Skip leading whitespace */
    while (*trimmed && isspace(*trimmed)) trimmed++;
    
    if (strlen(trimmed) == 0) return KH_TYPE_STRING_LOCAL;
    
    /* Check for boolean */
    if ((strncmp(trimmed, "true", 4) == 0 && (trimmed[4] == '\0' || isspace(trimmed[4]))) ||
        (strncmp(trimmed, "false", 5) == 0 && (trimmed[5] == '\0' || isspace(trimmed[5])))) {
        return KH_TYPE_BOOL_LOCAL;
    }
    
    /* Check for array */
    if (*trimmed == '[') {
        /* Use robust hashmap detection */
        if (kh_is_valid_hashmap(trimmed)) {
            return KH_TYPE_HASHMAP_LOCAL;
        } else {
            return KH_TYPE_ARRAY_LOCAL;
        }
    }
    
    /* Check for quoted string */
    if (*trimmed == '"') return KH_TYPE_STRING_LOCAL;
    
    /* Check for number */
    char* end_ptr;
    strtod(trimmed, &end_ptr);
    
    /* Skip trailing whitespace */
    while (*end_ptr && isspace(*end_ptr)) end_ptr++;
    
    if (*end_ptr == '\0' && end_ptr != trimmed) {
        return KH_TYPE_SCALAR_LOCAL;
    }
    
    /* Default to string */
    return KH_TYPE_STRING_LOCAL;
}

/* Parse array string into dynamic array structure */
static inline int kh_parse_array_string(const char* input, dynamic_array_t* result) {
    if (!input || !result) return 0;
    
    if (!kh_init_dynamic_array(result, 16)) return 0;
    
    const char* ptr = input;
    int input_len = (int)strlen(input);
    int bracket_depth = 0;
    int in_quotes = 0;
    int parse_operations = 0;
    
    /* Input size validation */
    if (input_len > KH_ARRAY_MAX_INPUT_SIZE) {
        kh_free_dynamic_array(result);
        return 0;
    }
    
    /* Skip leading whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Must start with opening bracket */
    if (*ptr != '[') {
        kh_free_dynamic_array(result);
        return 0;
    }
    ptr++; /* Skip opening bracket */
    bracket_depth = 1;
    
    const char* element_start = NULL;
    int element_bracket_depth = 0;
    
    while (*ptr && bracket_depth > 0 && parse_operations < KH_ARRAY_PARSE_OPERATIONS_LIMIT) {
        parse_operations++;
        char c = *ptr;
        
        if (c == '"') {
            in_quotes = !in_quotes;
        }
        
        if (!in_quotes) {
            if (c == '[') {
                if (element_start == NULL) {
                    /* Skip whitespace to find element start */
                    const char* temp = ptr;
                    while (*temp && isspace(*temp)) temp++;
                    if (*temp) element_start = ptr;
                }
                bracket_depth++;
                element_bracket_depth++;
            } else if (c == ']') {
                bracket_depth--;
                element_bracket_depth--;
                
                /* End of array or end of nested element */
                if (bracket_depth == 0 || (element_bracket_depth == 0 && element_start != NULL)) {
                    if (element_start != NULL) {
                        /* Extract element */
                        int element_len = (int)(ptr - element_start);
                        if (bracket_depth == 0) element_len++; /* Include closing bracket for nested arrays */
                        
                        char* element_str = (char*)malloc((size_t)element_len + 1);
                        if (!element_str) {
                            kh_free_dynamic_array(result);
                            return 0;
                        }
                        
                        memcpy(element_str, element_start, (size_t)element_len);
                        element_str[element_len] = '\0';
                        
                        /* Trim whitespace */
                        char* trimmed = element_str;
                        while (*trimmed && isspace(*trimmed)) trimmed++;
                        
                        char* end = trimmed + strlen(trimmed) - 1;
                        while (end > trimmed && isspace(*end)) *end-- = '\0';
                        
                        if (strlen(trimmed) > 0) {
                            int element_type = kh_detect_element_type(trimmed);
                            if (!kh_add_array_element(result, element_type, trimmed)) {
                                free(element_str);
                                kh_free_dynamic_array(result);
                                return 0;
                            }
                        }
                        
                        free(element_str);
                        element_start = NULL;
                        element_bracket_depth = 0;
                    }
                }
            } else if (c == ',' && bracket_depth == 1) {
                /* Element separator at top level */
                if (element_start != NULL) {
                    /* Extract element */
                    int element_len = (int)(ptr - element_start);
                    
                    char* element_str = (char*)malloc((size_t)element_len + 1);
                    if (!element_str) {
                        kh_free_dynamic_array(result);
                        return 0;
                    }
                    
                    memcpy(element_str, element_start, (size_t)element_len);
                    element_str[element_len] = '\0';
                    
                    /* Trim whitespace */
                    char* trimmed = element_str;
                    while (*trimmed && isspace(*trimmed)) trimmed++;
                    
                    char* end = trimmed + strlen(trimmed) - 1;
                    while (end > trimmed && isspace(*end)) *end-- = '\0';
                    
                    if (strlen(trimmed) > 0) {
                        int element_type = kh_detect_element_type(trimmed);
                        if (!kh_add_array_element(result, element_type, trimmed)) {
                            free(element_str);
                            kh_free_dynamic_array(result);
                            return 0;
                        }
                    }
                    
                    free(element_str);
                }
                
                /* Skip comma and whitespace to find next element */
                ptr++;
                while (*ptr && isspace(*ptr)) ptr++;
                element_start = (*ptr != '\0' && bracket_depth > 0) ? ptr : NULL;
                element_bracket_depth = 0;
                continue;
            } else if (element_start == NULL && !isspace(c)) {
                element_start = ptr;
                element_bracket_depth = 0;
            }
        }
        
        ptr++;
    }
    
    /* Check for parsing limits exceeded */
    if (parse_operations >= KH_ARRAY_PARSE_OPERATIONS_LIMIT) {
        kh_free_dynamic_array(result);
        return 0;
    }
    
    return (bracket_depth == 0) ? 1 : 0;
}

/* Format array back to string */
static inline int kh_format_array_to_string(const dynamic_array_t* arr, char* output, int output_size) {
    if (!arr || !output || output_size <= 0) return 0;
    
    int pos = 0;
    
    /* Opening bracket */
    if (pos >= output_size - 1) return 0;
    output[pos++] = '[';
    
    for (int i = 0; i < arr->count; i++) {
        if (i > 0) {
            /* Add comma separator */
            if (pos >= output_size - 2) return 0;
            output[pos++] = ',';
            output[pos++] = ' ';
        }
        
        const array_element_t* elem = &arr->elements[i];
        int value_len = (int)strlen(elem->value);
        
        /* Check if we have enough space */
        if (pos + value_len >= output_size - 1) return 0;
        
        /* Add element value */
        memcpy(output + pos, elem->value, (size_t)value_len);
        pos += value_len;
    }
    
    /* Closing bracket */
    if (pos >= output_size - 1) return 0;
    output[pos++] = ']';
    output[pos] = '\0';
    
    return 1;
}

/* Array operation implementations */

/* Get array length */
static inline double kh_array_length(const dynamic_array_t* arr) {
    return arr ? (double)arr->count : 0.0;
}

/* Check if array is empty */
static inline int kh_array_is_empty(const dynamic_array_t* arr) {
    return (arr == NULL || arr->count == 0) ? 1 : 0;
}

/* Sum all numeric elements in array */
static inline double kh_array_sum(const dynamic_array_t* arr) {
    if (!arr) return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == KH_TYPE_SCALAR_LOCAL) {
            sum += arr->elements[i].number;
        }
    }
    return sum;
}

/* Get average of numeric elements in array */
static inline double kh_array_average(const dynamic_array_t* arr) {
    if (!arr || arr->count == 0) return 0.0;
    
    double sum = 0.0;
    int numeric_count = 0;
    
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == KH_TYPE_SCALAR_LOCAL) {
            sum += arr->elements[i].number;
            numeric_count++;
        }
    }
    
    return (numeric_count > 0) ? (sum / numeric_count) : 0.0;
}

/* Get minimum value from numeric elements */
static inline double kh_array_min(const dynamic_array_t* arr) {
    if (!arr || arr->count == 0) return 0.0;
    
    double min_val = INFINITY;
    int found_numeric = 0;
    
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == KH_TYPE_SCALAR_LOCAL) {
            if (!found_numeric || arr->elements[i].number < min_val) {
                min_val = arr->elements[i].number;
                found_numeric = 1;
            }
        }
    }
    
    return found_numeric ? min_val : 0.0;
}

/* Get maximum value from numeric elements */
static inline double kh_array_max(const dynamic_array_t* arr) {
    if (!arr || arr->count == 0) return 0.0;
    
    double max_val = -INFINITY;
    int found_numeric = 0;
    
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == KH_TYPE_SCALAR_LOCAL) {
            if (!found_numeric || arr->elements[i].number > max_val) {
                max_val = arr->elements[i].number;
                found_numeric = 1;
            }
        }
    }
    
    return found_numeric ? max_val : 0.0;
}

/* Count elements of specific type */
static inline double kh_array_count_type(const dynamic_array_t* arr, int target_type) {
    if (!arr) return 0.0;
    
    int count = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == target_type) {
            count++;
        }
    }
    return (double)count;
}

/* Check if arrays are equal */
static inline int kh_array_equals(const dynamic_array_t* arr1, const dynamic_array_t* arr2) {
    if (!arr1 && !arr2) return 1;
    if (!arr1 || !arr2) return 0;
    if (arr1->count != arr2->count) return 0;
    
    for (int i = 0; i < arr1->count; i++) {
        if (arr1->elements[i].type != arr2->elements[i].type) return 0;
        if (strcmp(arr1->elements[i].value, arr2->elements[i].value) != 0) return 0;
    }
    
    return 1;
}

/* Check if element exists in array */
static inline int kh_array_contains(const dynamic_array_t* arr, const char* value, int value_type) {
    if (!arr || !value) return 0;
    
    for (int i = 0; i < arr->count; i++) {
        if (arr->elements[i].type == value_type && 
            strcmp(arr->elements[i].value, value) == 0) {
            return 1;
        }
    }
    
    return 0;
}

/* Create intersection of two arrays */
static inline int kh_array_intersect(const dynamic_array_t* arr1, const dynamic_array_t* arr2, 
                                     dynamic_array_t* result) {
    if (!arr1 || !arr2 || !result) return 0;
    
    if (!kh_init_dynamic_array(result, 16)) return 0;
    
    for (int i = 0; i < arr1->count; i++) {
        if (kh_array_contains(arr2, arr1->elements[i].value, arr1->elements[i].type)) {
            /* Check if already added to result */
            if (!kh_array_contains(result, arr1->elements[i].value, arr1->elements[i].type)) {
                if (!kh_add_array_element(result, arr1->elements[i].type, arr1->elements[i].value)) {
                    kh_free_dynamic_array(result);
                    return 0;
                }
            }
        }
    }
    
    return 1;
}

/* Create union of two arrays */
static inline int kh_array_union(const dynamic_array_t* arr1, const dynamic_array_t* arr2, 
                                 dynamic_array_t* result) {
    if (!arr1 || !arr2 || !result) return 0;
    
    if (!kh_init_dynamic_array(result, arr1->count + arr2->count)) return 0;
    
    /* Add all elements from first array */
    for (int i = 0; i < arr1->count; i++) {
        if (!kh_add_array_element(result, arr1->elements[i].type, arr1->elements[i].value)) {
            kh_free_dynamic_array(result);
            return 0;
        }
    }
    
    /* Add unique elements from second array */
    for (int i = 0; i < arr2->count; i++) {
        if (!kh_array_contains(result, arr2->elements[i].value, arr2->elements[i].type)) {
            if (!kh_add_array_element(result, arr2->elements[i].type, arr2->elements[i].value)) {
                kh_free_dynamic_array(result);
                return 0;
            }
        }
    }
    
    return 1;
}

/* Get types summary of array elements */
static inline int kh_array_get_types(const dynamic_array_t* arr, char* output, int output_size) {
    if (!arr || !output || output_size <= 0) return 0;
    
    int type_counts[5] = {0}; /* Count for each KH_TYPE_* (excluding CODE and TEXT) */
    
    for (int i = 0; i < arr->count; i++) {
        int type = arr->elements[i].type;
        if (type >= 0 && type < 5) {
            type_counts[type]++;
        }
    }
    
    const char* type_names[] = {"ARRAY", "STRING", "SCALAR", "HASHMAP", "BOOL"};
    
    int pos = 0;
    output[pos++] = '[';
    
    int first = 1;
    for (int type = 0; type < 5; type++) {
        if (type_counts[type] > 0) {
            if (!first) {
                if (pos + 2 >= output_size) return 0;
                output[pos++] = ',';
                output[pos++] = ' ';
            }
            
            int needed = _snprintf_s(output + pos, (size_t)(output_size - pos), _TRUNCATE, 
                                     "[\"%s\", %d]", type_names[type], type_counts[type]);
            if (needed < 0 || pos + needed >= output_size) return 0;
            pos += needed;
            first = 0;
        }
    }
    
    if (pos >= output_size - 1) return 0;
    output[pos++] = ']';
    output[pos] = '\0';
    
    return 1;
}

/* Reverse array elements */
static inline int kh_array_reverse(const dynamic_array_t* arr, dynamic_array_t* result) {
    if (!arr || !result) return 0;
    
    if (!kh_init_dynamic_array(result, arr->count)) return 0;
    
    for (int i = arr->count - 1; i >= 0; i--) {
        if (!kh_add_array_element(result, arr->elements[i].type, arr->elements[i].value)) {
            kh_free_dynamic_array(result);
            return 0;
        }
    }
    
    return 1;
}

/* Implementation functions for dispatch table */
static inline double kh_array_length_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0.0;
    
    double result = kh_array_length(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline double kh_array_sum_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0.0;
    
    double result = kh_array_sum(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline double kh_array_average_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0.0;
    
    double result = kh_array_average(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline double kh_array_min_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0.0;
    
    double result = kh_array_min(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline double kh_array_max_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0.0;
    
    double result = kh_array_max(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline int kh_array_is_empty_impl(const char* input) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 1;
    
    int result = kh_array_is_empty(&arr);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline int kh_array_equals_impl(const char* input1, const char* input2) {
    dynamic_array_t arr1, arr2;
    int result = 0;
    
    if (kh_parse_array_string(input1, &arr1) && kh_parse_array_string(input2, &arr2)) {
        result = kh_array_equals(&arr1, &arr2);
    }
    
    kh_free_dynamic_array(&arr1);
    kh_free_dynamic_array(&arr2);
    return result;
}

static inline int kh_array_intersect_impl(const char* input1, const char* input2, char* output, int output_size) {
    dynamic_array_t arr1, arr2, result;
    int success = 0;
    
    if (kh_parse_array_string(input1, &arr1) && kh_parse_array_string(input2, &arr2)) {
        if (kh_array_intersect(&arr1, &arr2, &result)) {
            success = kh_format_array_to_string(&result, output, output_size);
            kh_free_dynamic_array(&result);
        }
    }
    
    kh_free_dynamic_array(&arr1);
    kh_free_dynamic_array(&arr2);
    return success;
}

static inline int kh_array_union_impl(const char* input1, const char* input2, char* output, int output_size) {
    dynamic_array_t arr1, arr2, result;
    int success = 0;
    
    if (kh_parse_array_string(input1, &arr1) && kh_parse_array_string(input2, &arr2)) {
        if (kh_array_union(&arr1, &arr2, &result)) {
            success = kh_format_array_to_string(&result, output, output_size);
            kh_free_dynamic_array(&result);
        }
    }
    
    kh_free_dynamic_array(&arr1);
    kh_free_dynamic_array(&arr2);
    return success;
}

static inline int kh_array_types_impl(const char* input, char* output, int output_size) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0;
    
    int result = kh_array_get_types(&arr, output, output_size);
    kh_free_dynamic_array(&arr);
    return result;
}

static inline int kh_array_reverse_impl(const char* input, char* output, int output_size) {
    dynamic_array_t arr, result;
    int success = 0;
    
    if (kh_parse_array_string(input, &arr)) {
        if (kh_array_reverse(&arr, &result)) {
            success = kh_format_array_to_string(&result, output, output_size);
            kh_free_dynamic_array(&result);
        }
    }
    
    kh_free_dynamic_array(&arr);
    return success;
}

static inline int kh_array_contains_impl(const char* input, const char* value, char* output, int output_size) {
    dynamic_array_t arr;
    if (!kh_parse_array_string(input, &arr)) return 0;
    
    int value_type = kh_detect_element_type(value);
    int contains = kh_array_contains(&arr, value, value_type);
    
    strcpy_s(output, (size_t)output_size, contains ? "true" : "false");
    
    kh_free_dynamic_array(&arr);
    return 1;
}

/* Wrapper functions for scalar return types */
static inline int kh_array_length_wrapper(const char* input, char* output, int output_size) {
    double result = kh_array_length_impl(input);
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.0f", result);
    return 1;
}

static inline int kh_array_sum_wrapper(const char* input, char* output, int output_size) {
    double result = kh_array_sum_impl(input);
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", result);
    return 1;
}

static inline int kh_array_average_wrapper(const char* input, char* output, int output_size) {
    double result = kh_array_average_impl(input);
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", result);
    return 1;
}

static inline int kh_array_min_wrapper(const char* input, char* output, int output_size) {
    double result = kh_array_min_impl(input);
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", result);
    return 1;
}

static inline int kh_array_max_wrapper(const char* input, char* output, int output_size) {
    double result = kh_array_max_impl(input);
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%.6g", result);
    return 1;
}

static inline int kh_array_is_empty_wrapper(const char* input, char* output, int output_size) {
    int result = kh_array_is_empty_impl(input);
    strcpy_s(output, (size_t)output_size, result ? "true" : "false");
    return 1;
}

static inline int kh_array_equals_wrapper(const char* input1, const char* input2, char* output, int output_size) {
    int result = kh_array_equals_impl(input1, input2);
    strcpy_s(output, (size_t)output_size, result ? "true" : "false");
    return 1;
}

/* Whitelist of allowed array functions - SECURITY CRITICAL */
static const array_function_t KH_ALLOWED_ARRAY_FUNCTIONS[] = {
    /* Array to Scalar operations */
    {"alength", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_length_wrapper, "Get array length"},
    {"asize", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_length_wrapper, "Get array size (alias)"},
    {"asum", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_sum_wrapper, "Sum all numeric elements"},
    {"aaverage", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_average_wrapper, "Average of numeric elements"},
    {"amean", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_average_wrapper, "Mean of numeric elements (alias)"},
    {"amin", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_min_wrapper, "Minimum of numeric elements"},
    {"amax", ARRAY_FUNC_ARRAY_TO_SCALAR, 1, (void*)kh_array_max_wrapper, "Maximum of numeric elements"},
    
    /* Array to Boolean operations */
    {"aisempty", ARRAY_FUNC_ARRAY_TO_BOOL, 1, (void*)kh_array_is_empty_wrapper, "Check if array is empty"},
    {"aequals", ARRAY_FUNC_ARRAYARRAY_TO_BOOL, 2, (void*)kh_array_equals_wrapper, "Check if arrays are equal"},
    {"acontains", ARRAY_FUNC_ARRAYVALUE_TO_ARRAY, 2, (void*)kh_array_contains_impl, "Check if array contains value"},
    
    /* Array to String operations */
    {"atypes", ARRAY_FUNC_ARRAY_TO_STRING, 1, (void*)kh_array_types_impl, "Get types summary of array elements"},
    
    /* Array to Array operations */
    {"areverse", ARRAY_FUNC_ARRAY_TO_ARRAY, 1, (void*)kh_array_reverse_impl, "Reverse array elements"},
    
    /* Array, Array to Array operations */
    {"aintersect", ARRAY_FUNC_ARRAYARRAY_TO_ARRAY, 2, (void*)kh_array_intersect_impl, "Get intersection of two arrays"},
    {"aunion", ARRAY_FUNC_ARRAYARRAY_TO_ARRAY, 2, (void*)kh_array_union_impl, "Get union of two arrays"}
};

static const int KH_ARRAY_FUNCTION_COUNT = sizeof(KH_ALLOWED_ARRAY_FUNCTIONS) / sizeof(array_function_t);

/* Find array function in whitelist - SECURITY CRITICAL */
static inline const array_function_t* kh_find_array_function(const char* name) {
    if (!name) return NULL;
    
    int i;
    for (i = 0; i < KH_ARRAY_FUNCTION_COUNT; i++) {
        if (kh_strcasecmp(KH_ALLOWED_ARRAY_FUNCTIONS[i].name, name) == 0) {
            return &KH_ALLOWED_ARRAY_FUNCTIONS[i];
        }
    }
    return NULL;
}

/* Main array operation processing function */
static int kh_process_array_operation(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc < 1) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    char* clean_operation = NULL;
    char** clean_args = NULL;
    const array_function_t* func;
    int result = 1;
    int allocated_args = 0;
    
    clean_operation = (char*)malloc(strlen(argv[0]) + 1);
    if (!clean_operation) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(argv[0], clean_operation, (int)strlen(argv[0]) + 1);
    
    if (strlen(clean_operation) == 0) {
        kh_set_error(output, output_size, "EMPTY FUNCTION NAME");
        goto cleanup;
    }
    
    func = kh_find_array_function(clean_operation);
    if (!func) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "UNKNOWN ARRAY FUNCTION '%s'", clean_operation);
        goto cleanup;
    }
    
    if (argc - 1 != func->arg_count) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "FUNCTION '%s' EXPECTS %d ARGUMENTS, GOT %d", 
                 clean_operation, func->arg_count, argc - 1);
        goto cleanup;
    }
    
    /* Clean arguments */
    if (argc > 1) {
        clean_args = (char**)calloc((size_t)(argc - 1), sizeof(char*));
        if (!clean_args) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        
        for (int i = 1; i < argc; i++) {
            clean_args[i - 1] = (char*)malloc(strlen(argv[i]) + 1);
            if (!clean_args[i - 1]) {
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            allocated_args++;
            kh_clean_string(argv[i], clean_args[i - 1], (int)strlen(argv[i]) + 1);
        }
    }
    
    /* Execute function based on type */
    switch (func->type) {
        case ARRAY_FUNC_ARRAY_TO_SCALAR:
        case ARRAY_FUNC_ARRAY_TO_BOOL: {
            int (*f)(const char*, char*, int) = (int (*)(const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case ARRAY_FUNC_ARRAY_TO_STRING:
        case ARRAY_FUNC_ARRAY_TO_ARRAY: {
            int (*f)(const char*, char*, int) = (int (*)(const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case ARRAY_FUNC_ARRAYARRAY_TO_ARRAY: {
            int (*f)(const char*, const char*, char*, int) = (int (*)(const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case ARRAY_FUNC_ARRAYARRAY_TO_BOOL: {
            int (*f)(const char*, const char*, char*, int) = (int (*)(const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case ARRAY_FUNC_ARRAYVALUE_TO_ARRAY: {
            int (*f)(const char*, const char*, char*, int) = (int (*)(const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        default:
            kh_set_error(output, output_size, "UNKNOWN FUNCTION TYPE");
            break;
    }

cleanup:
    free(clean_operation);
    if (clean_args) {
        for (int j = 0; j < allocated_args; j++) {
            free(clean_args[j]);
        }
        free(clean_args);
    }
    return result;
}

#endif /* ARRAY_OPERATIONS_HPP */