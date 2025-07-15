#ifndef SLICE_DATA_HPP
#define SLICE_DATA_HPP

#include "common_defines.hpp"

/* Forward declaration to avoid circular dependency */
typedef struct kh_variable_s kh_variable_t;
extern int kh_read_binary_file(const char* file_path, kh_variable_t** variables, int* count);
extern void kh_free_variables(kh_variable_t* variables, int count);
extern int kh_find_variable(kh_variable_t* variables, int count, const char* name);
extern const char* kh_get_string_from_type(kh_data_type_t type);

/* Calculate the formatted size of a variable's output - optimized */
static long kh_get_variable_formatted_size(const char* filename, const char* variable_name) {
    if (!filename || !variable_name) return -1;
    
    char file_path[MAX_PATH_LENGTH];
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
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
    
    if (!kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_read_binary_file(file_path, &variables, &variable_count)) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Special case: variable name equals filename (return size of all variable names array) */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        total_size = 2; /* Opening and closing brackets */
        
        for (int i = 0; i < variable_count; i++) {
            if (i > 0) {
                total_size += 2; /* ", " separator */
            }
            if (variables[i].name) {
                total_size += (long)strlen(variables[i].name) + 2; /* Name with quotes */
            }
        }
        
        free(clean_var_name);
        free(clean_filename);
        kh_free_variables(variables, variable_count);
        return total_size;
    }
    
    /* Find specific variable using external function to avoid code duplication */
    var_index = kh_find_variable(variables, variable_count, clean_var_name);
    if (var_index == -1) {
        free(clean_var_name);
        free(clean_filename);
        kh_free_variables(variables, variable_count);
        return -1;
    }
    
    /* Calculate size of formatted output ["TYPE", value] */
    const char* type_str = kh_get_string_from_type(variables[var_index].type);
    if (type_str && variables[var_index].value) {
        total_size = (long)strlen(type_str) + (long)strlen(variables[var_index].value) + 6; /* ["", ] formatting */
    }
    
    free(clean_var_name);
    free(clean_filename);
    kh_free_variables(variables, variable_count);
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
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
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

#endif /* SLICE_DATA_HPP */