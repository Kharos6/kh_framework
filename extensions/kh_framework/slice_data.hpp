#ifndef SLICE_DATA_HPP
#define SLICE_DATA_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLICE_SIZE 8192  /* 8KB per slice to stay under Arma's byte limit */

/* Get the total size of a variable's formatted output */
static long get_variable_formatted_size(const char* filename, const char* variable_name) {
    char file_path[1024];
    char* clean_var_name;
    char* clean_filename;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    int var_index;
    int i;
    long total_size = 0;
    
    /* Get full file path */
    if (!get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        return -1;
    }
    
    /* Clean variable name and filename for comparison */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_var_name || !clean_filename) {
        if (clean_var_name) free(clean_var_name);
        if (clean_filename) free(clean_filename);
        return -1;
    }
    
    clean_variable_name(variable_name, clean_var_name, strlen(variable_name) + 1);
    clean_variable_name(filename, clean_filename, strlen(filename) + 1);
    
    /* Read binary file */
    if (!read_binary_file(file_path, &variables, &variable_count)) {
        free(clean_var_name);
        free(clean_filename);
        return -1; /* File not found or corrupted */
    }
    
    /* Check if variable name equals filename (special case: return size of all variable names array) */
    if (strcasecmp_custom(clean_var_name, clean_filename) == 0) {
        /* Calculate size of variable names array */
        total_size = 2; /* For opening and closing brackets */
        
        for (i = 0; i < variable_count; i++) {
            if (i > 0) {
                total_size += 2; /* For ", " */
            }
            total_size += strlen(variables[i].name) + 2; /* For quotes around name */
        }
        
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return total_size;
    }
    
    /* Find the specific variable */
    var_index = find_variable(variables, variable_count, clean_var_name);
    if (var_index == -1) {
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return -1; /* Variable not found */
    }
    
    /* Calculate size of formatted output ["TYPE", value] */
    const char* type_str = get_string_from_type(variables[var_index].type);
    total_size = strlen(type_str) + strlen(variables[var_index].value) + 6; /* ["", ] */
    
    free_variables(variables, variable_count);
    free(clean_var_name);
    free(clean_filename);
    return total_size;
}

/* Calculate how many slices a variable would need */
static int calculate_slice_count(const char* filename, const char* variable_name, char* output, int output_size) {
    long data_size = get_variable_formatted_size(filename, variable_name);
    int slice_count;
    
    if (data_size < 0) {
        strcpy_s(output, output_size, "KH_ERROR: VARIABLE NOT FOUND");
        return 1;
    }
    
    if (data_size == 0) {
        strcpy_s(output, output_size, "1"); /* Empty data still needs 1 slice */
        return 0;
    }
    
    /* Calculate number of slices needed */
    slice_count = (int)((data_size + SLICE_SIZE - 1) / SLICE_SIZE); /* Ceiling division */
    
    sprintf_s(output, output_size, "%d", slice_count);
    return 0;
}

#endif /* SLICE_DATA_HPP */