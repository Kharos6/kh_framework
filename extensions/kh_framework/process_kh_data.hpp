#ifndef PROCESS_KH_DATA_HPP
#define PROCESS_KH_DATA_HPP

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PATH_LENGTH 512
#define MAX_TOTAL_KHDATA_SIZE_BYTES (1024LL * 1024LL * 1024LL)  /* 1GB total limit for all .khdata files */
#define SLICE_SIZE 8192  /* 8KB per slice to stay under Arma's byte limit */
#define KHDATA_MAGIC 0x5444484B  /* "KHDT" in little endian */
#define KHDATA_VERSION 1

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

/* Structure for storing variable data */
typedef struct {
    char* name;
    kh_data_type_t type;
    char* value;
} kh_variable_t;

/* Binary file header */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int variable_count;
} kh_file_header_t;

/* Helper function to convert type string to enum */
static kh_data_type_t get_type_from_string(const char* type_str) {
    char clean_type[32];
    int i, j = 0;
    
    /* Clean the type string (remove quotes, convert to uppercase) */
    for (i = 0; type_str[i] != '\0' && j < sizeof(clean_type) - 1; i++) {
        if (type_str[i] != '"') {
            if (type_str[i] >= 'a' && type_str[i] <= 'z') {
                clean_type[j++] = type_str[i] - 32; /* Convert to uppercase */
            } else {
                clean_type[j++] = type_str[i];
            }
        }
    }
    clean_type[j] = '\0';
    
    if (strcmp(clean_type, "ARRAY") == 0) return KH_TYPE_ARRAY;
    if (strcmp(clean_type, "STRING") == 0) return KH_TYPE_STRING;
    if (strcmp(clean_type, "SCALAR") == 0) return KH_TYPE_SCALAR;
    if (strcmp(clean_type, "HASHMAP") == 0) return KH_TYPE_HASHMAP;
    if (strcmp(clean_type, "BOOL") == 0) return KH_TYPE_BOOL;
    if (strcmp(clean_type, "CODE") == 0) return KH_TYPE_CODE;
    if (strcmp(clean_type, "TEXT") == 0) return KH_TYPE_TEXT;
    
    return KH_TYPE_UNKNOWN;
}

/* Helper function to convert type enum to string */
static const char* get_string_from_type(kh_data_type_t type) {
    switch (type) {
        case KH_TYPE_ARRAY: return "ARRAY";
        case KH_TYPE_STRING: return "STRING";
        case KH_TYPE_SCALAR: return "SCALAR";
        case KH_TYPE_HASHMAP: return "HASHMAP";
        case KH_TYPE_BOOL: return "BOOL";
        case KH_TYPE_CODE: return "CODE";
        case KH_TYPE_TEXT: return "TEXT";
        default: return "UNKNOWN";
    }
}

/* Get the user's Documents\Arma 3 folder path */
static int get_arma3_documents_path(char* path, int path_size) {
    char documents_path[MAX_PATH];
    
    /* Get the user's Documents folder */
    if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path) != S_OK) {
        return 0;
    }
    
    /* Append \Arma 3 to the path */
    if (sprintf_s(path, path_size, "%s\\Arma 3", documents_path) < 0) {
        return 0;
    }
    
    /* Create directory if it doesn't exist */
    CreateDirectoryA(path, NULL);
    
    return 1;
}

/* Get full path to a .khdata file */
static int get_khdata_file_path(const char* filename, char* full_path, int path_size) {
    char arma3_path[1024];
    char* clean_filename;
    int filename_len;
    int i, j;
    
    if (!get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return 0;
    }
    
    /* Allocate memory for clean filename */
    filename_len = strlen(filename);
    clean_filename = (char*)malloc(filename_len + 8); /* +8 for ".khdata\0" */
    if (!clean_filename) {
        return 0;
    }
    
    /* Clean filename - remove quotes */
    j = 0;
    for (i = 0; i < filename_len; i++) {
        if (filename[i] != '"') {
            clean_filename[j++] = filename[i];
        }
    }
    clean_filename[j] = '\0';
    
    /* Add .khdata extension if not present */
    if (j < 7 || strcmp(clean_filename + j - 7, ".khdata") != 0) {
        strcat_s(clean_filename, filename_len + 8, ".khdata");
    }
    
    /* Build full path */
    int result = (sprintf_s(full_path, path_size, "%s\\%s", arma3_path, clean_filename) >= 0) ? 1 : 0;
    
    free(clean_filename);
    return result;
}

/* Clean variable name - remove quotes */
static void clean_variable_name(const char* input, char* output, int output_size) {
    int i, j = 0;
    
    for (i = 0; input[i] != '\0' && j < output_size - 1; i++) {
        if (input[i] != '"') {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

/* Clean value - remove surrounding quotes */
static void clean_value_quotes(const char* input, char* output, int output_size) {
    const char* start = input;
    const char* end = input + strlen(input) - 1;
    int len;
    
    /* Skip opening quote if present */
    if (*start == '"') {
        start++;
    }
    
    /* Skip closing quote if present */
    if (end >= start && *end == '"') {
        end--;
    }
    
    /* Calculate length without quotes */
    len = (int)(end - start + 1);
    if (len >= output_size) {
        len = output_size - 1;
    }
    
    /* Copy the clean value */
    if (len > 0) {
        strncpy_s(output, output_size, start, len);
        output[len] = '\0';
    } else {
        output[0] = '\0';
    }
}

/* Get file size in bytes */
static long get_file_size(const char* file_path) {
    FILE* file;
    long size;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return -1; /* File doesn't exist or can't be opened */
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    
    return size;
}

/* Helper function for case-insensitive string comparison */
static int strcasecmp_custom(const char* str1, const char* str2) {
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
    
    return 0; /* Equal */
}

/* Parse boolean string (true/false) */
static int parse_boolean(const char* bool_str) {
    char clean_bool[16];
    int i, j = 0;
    
    /* Clean the boolean string (remove quotes, convert to lowercase) */
    for (i = 0; bool_str[i] != '\0' && j < sizeof(clean_bool) - 1; i++) {
        if (bool_str[i] != '"') {
            if (bool_str[i] >= 'A' && bool_str[i] <= 'Z') {
                clean_bool[j++] = bool_str[i] + 32; /* Convert to lowercase */
            } else {
                clean_bool[j++] = bool_str[i];
            }
        }
    }
    clean_bool[j] = '\0';
    
    /* Check for true values */
    if (strcmp(clean_bool, "true") == 0 || strcmp(clean_bool, "1") == 0) {
        return 1;
    }
    
    /* Everything else is false (including "false", "0", empty, etc.) */
    return 0;
}

/* Read binary file and load all variables */
static int read_binary_file(const char* file_path, kh_variable_t** variables, int* count) {
    FILE* file;
    kh_file_header_t header;
    int i;
    unsigned int name_len, value_len, type_len;
    char type_str[32];
    
    *variables = NULL;
    *count = 0;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0; /* File not found */
    }
    
    /* Read header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Verify magic number and version */
    if (header.magic != KHDATA_MAGIC || header.version != KHDATA_VERSION) {
        fclose(file);
        return 0;
    }
    
    if (header.variable_count == 0) {
        fclose(file);
        return 1; /* Empty file is valid */
    }
    
    /* Allocate array for variables */
    *variables = (kh_variable_t*)malloc(header.variable_count * sizeof(kh_variable_t));
    if (!*variables) {
        fclose(file);
        return 0;
    }
    
    /* Read variables */
    for (i = 0; i < header.variable_count; i++) {
        /* Read variable name length and name */
        if (fread(&name_len, sizeof(unsigned int), 1, file) != 1) {
            break;
        }
        
        (*variables)[i].name = (char*)malloc(name_len + 1);
        if (!(*variables)[i].name) {
            break;
        }
        
        if (fread((*variables)[i].name, name_len, 1, file) != 1) {
            free((*variables)[i].name);
            break;
        }
        (*variables)[i].name[name_len] = '\0';
        
        /* Read type length and type */
        if (fread(&type_len, sizeof(unsigned int), 1, file) != 1) {
            free((*variables)[i].name);
            break;
        }
        
        if (type_len >= sizeof(type_str)) {
            free((*variables)[i].name);
            break;
        }
        
        if (fread(type_str, type_len, 1, file) != 1) {
            free((*variables)[i].name);
            break;
        }
        type_str[type_len] = '\0';
        (*variables)[i].type = get_type_from_string(type_str);
        
        /* Read value length and value */
        if (fread(&value_len, sizeof(unsigned int), 1, file) != 1) {
            free((*variables)[i].name);
            break;
        }
        
        (*variables)[i].value = (char*)malloc(value_len + 1);
        if (!(*variables)[i].value) {
            free((*variables)[i].name);
            break;
        }
        
        if (fread((*variables)[i].value, value_len, 1, file) != 1) {
            free((*variables)[i].name);
            free((*variables)[i].value);
            break;
        }
        (*variables)[i].value[value_len] = '\0';
        
        (*count)++;
    }
    
    fclose(file);
    
    /* If we didn't read all variables successfully, clean up */
    if (*count != header.variable_count) {
        for (i = 0; i < *count; i++) {
            free((*variables)[i].name);
            free((*variables)[i].value);
        }
        free(*variables);
        *variables = NULL;
        *count = 0;
        return 0;
    }
    
    return 1;
}

/* Write binary file with all variables */
static int write_binary_file(const char* file_path, kh_variable_t* variables, int count) {
    FILE* file;
    kh_file_header_t header;
    int i;
    unsigned int name_len, value_len, type_len;
    const char* type_str;
    
    if (fopen_s(&file, file_path, "wb") != 0) {
        return 0;
    }
    
    /* Write header */
    header.magic = KHDATA_MAGIC;
    header.version = KHDATA_VERSION;
    header.variable_count = count;
    
    if (fwrite(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Write variables */
    for (i = 0; i < count; i++) {
        /* Write variable name */
        name_len = strlen(variables[i].name);
        if (fwrite(&name_len, sizeof(unsigned int), 1, file) != 1) {
            fclose(file);
            return 0;
        }
        if (fwrite(variables[i].name, name_len, 1, file) != 1) {
            fclose(file);
            return 0;
        }
        
        /* Write type */
        type_str = get_string_from_type(variables[i].type);
        type_len = strlen(type_str);
        if (fwrite(&type_len, sizeof(unsigned int), 1, file) != 1) {
            fclose(file);
            return 0;
        }
        if (fwrite(type_str, type_len, 1, file) != 1) {
            fclose(file);
            return 0;
        }
        
        /* Write value */
        value_len = strlen(variables[i].value);
        if (fwrite(&value_len, sizeof(unsigned int), 1, file) != 1) {
            fclose(file);
            return 0;
        }
        if (fwrite(variables[i].value, value_len, 1, file) != 1) {
            fclose(file);
            return 0;
        }
    }
    
    fclose(file);
    return 1;
}

/* Free variables array */
static void free_variables(kh_variable_t* variables, int count) {
    int i;
    if (variables) {
        for (i = 0; i < count; i++) {
            free(variables[i].name);
            free(variables[i].value);
        }
        free(variables);
    }
}

/* Find variable by name (case-insensitive) */
static int find_variable(kh_variable_t* variables, int count, const char* name) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcasecmp_custom(variables[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Calculate total size of all .khdata files in the Arma 3 documents folder */
static long long get_total_khdata_files_size(void) {
    char arma3_path[MAX_PATH_LENGTH];
    char search_pattern[MAX_PATH_LENGTH];
    char file_path[MAX_PATH_LENGTH];
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    long long total_size = 0;
    long file_size;
    
    /* Get Arma 3 documents path */
    if (!get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return -1; /* Error getting path */
    }
    
    /* Create search pattern for .khdata files */
    sprintf_s(search_pattern, sizeof(search_pattern), "%s\\*.khdata", arma3_path);
    
    /* Find first .khdata file */
    find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return 0; /* No .khdata files found */
    }
    
    /* Sum sizes of all .khdata files */
    do {
        /* Skip directories */
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            sprintf_s(file_path, sizeof(file_path), "%s\\%s", arma3_path, find_data.cFileName);
            file_size = get_file_size(file_path);
            if (file_size > 0) {
                total_size += file_size;
            }
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return total_size;
}

/* Check if a specific .khdata file exists */
static int khdata_file_exists(const char* filename) {
    char file_path[MAX_PATH_LENGTH];
    DWORD file_attributes;
    
    if (!get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        return 0; /* Error getting path, assume doesn't exist */
    }
    
    file_attributes = GetFileAttributesA(file_path);
    return (file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY));
}

/* Format variable for output (["TYPE", value]) */
static int format_variable_output(kh_data_type_t type, const char* value, char* output, int output_size) {
    const char* type_str = get_string_from_type(type);
    
    /* Format as ["TYPE", value] */
    if (snprintf(output, output_size, "[\"%s\", %s]", type_str, value) >= output_size) {
        return 0; /* Output buffer too small */
    }
    
    return 1;
}

/* Parse hashmap key-value pair from input */
static int parse_hashmap_input(const char* input, char** key, char** value) {
    char* clean_input = (char*)malloc(strlen(input) + 1);
    char* colon_pos;
    int key_len, value_len;
    
    if (!clean_input) {
        return 0;
    }
    
    /* Clean input (remove outer quotes if present) */
    clean_value_quotes(input, clean_input, strlen(input) + 1);
    
    /* Find the first colon to separate key and value */
    colon_pos = strchr(clean_input, ':');
    if (!colon_pos) {
        free(clean_input);
        return 0;
    }
    
    /* Extract key */
    key_len = (int)(colon_pos - clean_input);
    *key = (char*)malloc(key_len + 1);
    if (!*key) {
        free(clean_input);
        return 0;
    }
    strncpy_s(*key, key_len + 1, clean_input, key_len);
    (*key)[key_len] = '\0';
    
    /* Remove leading/trailing whitespace from key */
    while (key_len > 0 && ((*key)[key_len-1] == ' ' || (*key)[key_len-1] == '\t')) {
        (*key)[--key_len] = '\0';
    }
    
    /* Extract value */
    value_len = strlen(colon_pos + 1);
    *value = (char*)malloc(value_len + 1);
    if (!*value) {
        free(*key);
        free(clean_input);
        return 0;
    }
    strcpy_s(*value, value_len + 1, colon_pos + 1);
    
    /* Remove leading whitespace from value */
    while ((*value)[0] == ' ' || (*value)[0] == '\t') {
        memmove(*value, *value + 1, strlen(*value));
    }
    
    free(clean_input);
    return 1;
}

/* Merge hashmap values */
static int merge_hashmap(const char* existing_hashmap, const char* new_entry, char* result, int result_size) {
    char* key;
    char* value;
    char* result_copy;
    int success = 0;
    
    /* Parse the new key-value pair */
    if (!parse_hashmap_input(new_entry, &key, &value)) {
        return 0;
    }
    
    /* For simplicity, we'll treat this as a simple string replacement */
    /* In a real implementation, you'd want proper hashmap parsing */
    result_copy = (char*)malloc(strlen(existing_hashmap) + strlen(new_entry) + 100);
    if (result_copy) {
        sprintf_s(result_copy, strlen(existing_hashmap) + strlen(new_entry) + 100, 
                 "%s + [%s: %s]", existing_hashmap, key, value);
        
        if (strlen(result_copy) < result_size) {
            strcpy_s(result, result_size, result_copy);
            success = 1;
        }
        free(result_copy);
    }
    
    free(key);
    free(value);
    return success;
}

/* Toggle boolean value */
static int toggle_boolean(const char* current_bool, char* result, int result_size) {
    int current_value = parse_boolean(current_bool);
    strcpy_s(result, result_size, current_value ? "false" : "true");
    return 1;
}

/* Combine values based on type and overwrite flag */
static int combine_typed_values(kh_data_type_t type, const char* existing_value, const char* new_value, 
                               int overwrite_flag, char* result, int result_size) {
    char* clean_existing;
    char* clean_new;
    double existing_num, new_num, result_num;
    int success = 0;
    
    clean_existing = (char*)malloc(strlen(existing_value) + 1);
    clean_new = (char*)malloc(strlen(new_value) + 1);
    
    if (!clean_existing || !clean_new) {
        if (clean_existing) free(clean_existing);
        if (clean_new) free(clean_new);
        return 0;
    }
    
    clean_value_quotes(existing_value, clean_existing, strlen(existing_value) + 1);
    clean_value_quotes(new_value, clean_new, strlen(new_value) + 1);
    
    switch (type) {
        case KH_TYPE_BOOL:
            if (overwrite_flag) {
                /* Set to new boolean value */
                strcpy_s(result, result_size, clean_new);
                success = 1;
            } else {
                /* Flip the current boolean value */
                success = toggle_boolean(clean_existing, result, result_size);
            }
            break;
            
        case KH_TYPE_HASHMAP:
            if (overwrite_flag) {
                /* Replace entire hashmap */
                strcpy_s(result, result_size, clean_new);
                success = 1;
            } else {
                /* Merge with existing hashmap */
                success = merge_hashmap(clean_existing, clean_new, result, result_size);
            }
            break;
            
        case KH_TYPE_SCALAR:
            if (overwrite_flag) {
                /* Replace with new value */
                strcpy_s(result, result_size, clean_new);
                success = 1;
            } else {
                /* Add to existing value */
                existing_num = atof(clean_existing);
                new_num = atof(clean_new);
                result_num = existing_num + new_num;
                sprintf_s(result, result_size, "%.6g", result_num);
                success = 1;
            }
            break;
            
        case KH_TYPE_ARRAY:
            if (overwrite_flag) {
                /* Replace entire array */
                strcpy_s(result, result_size, clean_new);
                success = 1;
            } else {
                /* Combine arrays */
                int existing_len = strlen(clean_existing);
                int new_len = strlen(clean_new);
                
                if (existing_len >= 2 && new_len >= 2) {
                    /* Remove closing bracket from existing, opening bracket from new */
                    if (existing_len == 2 || (existing_len == 3 && clean_existing[1] == ' ')) {
                        /* Empty array */
                        strcpy_s(result, result_size, clean_new);
                    } else {
                        snprintf(result, result_size, "%.*s,%s", existing_len - 1, clean_existing, clean_new + 1);
                    }
                    success = 1;
                }
            }
            break;
            
        case KH_TYPE_STRING:
        case KH_TYPE_TEXT:
        case KH_TYPE_CODE:
        default:
            if (overwrite_flag) {
                /* Replace with new value */
                strcpy_s(result, result_size, clean_new);
                success = 1;
            } else {
                /* Concatenate strings */
                if (strlen(clean_existing) + strlen(clean_new) < result_size) {
                    sprintf_s(result, result_size, "%s%s", clean_existing, clean_new);
                    success = 1;
                }
            }
            break;
    }
    
    free(clean_existing);
    free(clean_new);
    return success;
}

/* Read a variable from a .khdata file with optional slicing */
static int read_khdata_variable_slice(const char* filename, const char* variable_name, const char* slice_index_str, char* output, int output_size) {
    char file_path[1024];
    char* clean_var_name;
    char* clean_filename;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    int var_index;
    int slice_index = -1;
    int i;
    char* formatted_output;
    long formatted_len;
    
    /* Parse slice index if provided */
    if (slice_index_str && strlen(slice_index_str) > 0) {
        slice_index = atoi(slice_index_str);
        if (slice_index < 0) {
            strcpy_s(output, output_size, "KH_ERROR: INVALID SLICE INDEX");
            return 1;
        }
    }
    
    /* Get full file path */
    if (!get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        strcpy_s(output, output_size, "KH_ERROR: INVALID PATH");
        return 1;
    }
    
    /* Clean variable name and filename for comparison */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_var_name || !clean_filename) {
        strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
        if (clean_var_name) free(clean_var_name);
        if (clean_filename) free(clean_filename);
        return 1;
    }
    
    clean_variable_name(variable_name, clean_var_name, strlen(variable_name) + 1);
    clean_variable_name(filename, clean_filename, strlen(filename) + 1);
    
    /* Read binary file */
    if (!read_binary_file(file_path, &variables, &variable_count)) {
        strcpy_s(output, output_size, "KH_ERROR: FILE NOT FOUND OR CORRUPTED");
        free(clean_var_name);
        free(clean_filename);
        return 1;
    }
    
    /* Check if variable name equals filename (special case: return all variable names) */
    if (strcasecmp_custom(clean_var_name, clean_filename) == 0) {
        /* Return array of all variable names */
        char* result = (char*)malloc(variable_count * 256);
        if (!result) {
            strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
            free_variables(variables, variable_count);
            free(clean_var_name);
            free(clean_filename);
            return 1;
        }
        
        strcpy_s(result, variable_count * 256, "[");
        for (i = 0; i < variable_count; i++) {
            if (i > 0) {
                strcat_s(result, variable_count * 256, ", ");
            }
            strcat_s(result, variable_count * 256, "\"");
            strcat_s(result, variable_count * 256, variables[i].name);
            strcat_s(result, variable_count * 256, "\"");
        }
        strcat_s(result, variable_count * 256, "]");
        
        /* Handle slicing for variable list */
        if (slice_index >= 0) {
            long data_size = strlen(result);
            long slice_start = (long)slice_index * SLICE_SIZE;
            long slice_end = slice_start + SLICE_SIZE;
            
            if (slice_start >= data_size) {
                strcpy_s(output, output_size, "KH_ERROR: SLICE INDEX OUT OF RANGE");
            } else {
                if (slice_end > data_size) {
                    slice_end = data_size;
                }
                
                long slice_length = slice_end - slice_start;
                if (slice_length >= output_size) {
                    slice_length = output_size - 1;
                }
                
                strncpy_s(output, output_size, result + slice_start, slice_length);
                output[slice_length] = '\0';
            }
        } else {
            /* No slicing - check size limit */
            if (strlen(result) >= SLICE_SIZE) {
                strcpy_s(output, output_size, "KH_ERROR: DATA TOO LARGE WITHOUT SLICING");
            } else {
                strcpy_s(output, output_size, result);
            }
        }
        
        free(result);
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return 0;
    }
    
    /* Find the specific variable */
    var_index = find_variable(variables, variable_count, clean_var_name);
    if (var_index == -1) {
        strcpy_s(output, output_size, "KH_ERROR: VARIABLE NOT FOUND");
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return 1;
    }
    
    /* Format output as ["TYPE", value] */
    formatted_len = strlen(variables[var_index].value) + 50;
    formatted_output = (char*)malloc(formatted_len);
    if (!formatted_output) {
        strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return 1;
    }
    
    if (!format_variable_output(variables[var_index].type, variables[var_index].value, formatted_output, formatted_len)) {
        strcpy_s(output, output_size, "KH_ERROR: OUTPUT FORMATTING FAILED");
        free(formatted_output);
        free_variables(variables, variable_count);
        free(clean_var_name);
        free(clean_filename);
        return 1;
    }
    
    /* Handle slicing if requested */
    if (slice_index >= 0) {
        long data_size = strlen(formatted_output);
        long slice_start = (long)slice_index * SLICE_SIZE;
        long slice_end = slice_start + SLICE_SIZE;
        
        if (slice_start >= data_size) {
            strcpy_s(output, output_size, "KH_ERROR: SLICE INDEX OUT OF RANGE");
        } else {
            if (slice_end > data_size) {
                slice_end = data_size;
            }
            
            long slice_length = slice_end - slice_start;
            if (slice_length >= output_size) {
                slice_length = output_size - 1;
            }
            
            strncpy_s(output, output_size, formatted_output + slice_start, slice_length);
            output[slice_length] = '\0';
        }
    } else {
        /* No slicing - check size limit */
        if (strlen(formatted_output) >= SLICE_SIZE) {
            strcpy_s(output, output_size, "KH_ERROR: DATA TOO LARGE WITHOUT SLICING");
        } else {
            strcpy_s(output, output_size, formatted_output);
        }
    }
    
    free(formatted_output);
    free_variables(variables, variable_count);
    free(clean_var_name);
    free(clean_filename);
    return 0;
}

/* Backward compatibility wrapper for read function without slicing */
static int read_khdata_variable(const char* filename, const char* variable_name, char* output, int output_size) {
    return read_khdata_variable_slice(filename, variable_name, NULL, output, output_size);
}

/* Write a variable to a .khdata file with type and overwrite option */
static int write_khdata_variable(const char* filename, const char* variable_name, const char* value, 
                                const char* type_str, const char* overwrite_str, char* output, int output_size) {
    char file_path[1024];
    char* clean_var_name;
    char* clean_value;
    char* combined_value;
    kh_data_type_t data_type;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    int var_index;
    int overwrite_flag;
    long long current_total_size;
    long current_file_size;
    
    /* Validate type */
    data_type = get_type_from_string(type_str);
    if (data_type == KH_TYPE_UNKNOWN) {
        strcpy_s(output, output_size, "KH_ERROR: UNKNOWN TYPE");
        return 1;
    }
    
    /* Parse overwrite flag (default is true if not provided) */
    if (overwrite_str && strlen(overwrite_str) > 0) {
        overwrite_flag = parse_boolean(overwrite_str);
    } else {
        overwrite_flag = 1; /* Default to overwrite */
    }
    
    /* Get full file path */
    if (!get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        strcpy_s(output, output_size, "KH_ERROR: INVALID PATH");
        return 1;
    }
    
    /* Allocate memory for processing */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_value = (char*)malloc(strlen(value) + 1);
    combined_value = (char*)malloc(strlen(value) * 2 + 1000);
    
    if (!clean_var_name || !clean_value || !combined_value) {
        strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
        if (clean_var_name) free(clean_var_name);
        if (clean_value) free(clean_value);
        if (combined_value) free(combined_value);
        return 1;
    }
    
    /* Clean inputs */
    clean_variable_name(variable_name, clean_var_name, strlen(variable_name) + 1);
    clean_value_quotes(value, clean_value, strlen(value) + 1);
    
    /* Check total file size limits */
    current_total_size = get_total_khdata_files_size();
    if (current_total_size < 0) {
        strcpy_s(output, output_size, "KH_ERROR: CANNOT CALCULATE TOTAL SIZE");
        goto cleanup;
    }
    
    if (current_total_size > MAX_TOTAL_KHDATA_SIZE_BYTES) {
        strcpy_s(output, output_size, "KH_ERROR: TOTAL SIZE LIMIT EXCEEDED");
        goto cleanup;
    }
    
    current_file_size = get_file_size(file_path);
    if (current_file_size < 0) {
        current_file_size = 0; /* File doesn't exist yet */
    }
    
    /* Read existing file */
    if (!read_binary_file(file_path, &variables, &variable_count)) {
        /* File doesn't exist or is corrupted, create new one */
        variable_count = 0;
        variables = NULL;
    }
    
    /* Find existing variable */
    var_index = find_variable(variables, variable_count, clean_var_name);
    
    if (var_index != -1 && !overwrite_flag) {
        /* Variable exists and we're not overwriting - combine values */
        if (!combine_typed_values(data_type, variables[var_index].value, clean_value, 
                                 overwrite_flag, combined_value, strlen(value) * 2 + 1000)) {
            strcpy_s(output, output_size, "KH_ERROR: VALUE COMBINATION FAILED");
            goto cleanup;
        }
        
        /* Update existing variable */
        free(variables[var_index].value);
        variables[var_index].value = (char*)malloc(strlen(combined_value) + 1);
        if (!variables[var_index].value) {
            strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(variables[var_index].value, strlen(combined_value) + 1, combined_value);
        variables[var_index].type = data_type;
    } else {
        /* Add new variable or replace existing one */
        if (var_index == -1) {
            /* Add new variable */
            variables = (kh_variable_t*)realloc(variables, (variable_count + 1) * sizeof(kh_variable_t));
            if (!variables) {
                strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            var_index = variable_count++;
            
            variables[var_index].name = (char*)malloc(strlen(clean_var_name) + 1);
            if (!variables[var_index].name) {
                strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            strcpy_s(variables[var_index].name, strlen(clean_var_name) + 1, clean_var_name);
        } else {
            /* Replace existing variable */
            free(variables[var_index].value);
        }
        
        variables[var_index].value = (char*)malloc(strlen(clean_value) + 1);
        if (!variables[var_index].value) {
            strcpy_s(output, output_size, "KH_ERROR: MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(variables[var_index].value, strlen(clean_value) + 1, clean_value);
        variables[var_index].type = data_type;
    }
    
    /* Write binary file */
    if (!write_binary_file(file_path, variables, variable_count)) {
        strcpy_s(output, output_size, "KH_ERROR: FAILED TO WRITE FILE");
        goto cleanup;
    }
    
    strcpy_s(output, output_size, "SUCCESS");
    
cleanup:
    if (clean_var_name) free(clean_var_name);
    if (clean_value) free(clean_value);
    if (combined_value) free(combined_value);
    free_variables(variables, variable_count);
    return (strcmp(output, "SUCCESS") == 0) ? 0 : 1;
}

#endif /* PROCESS_KH_DATA_HPP */