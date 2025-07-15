#ifndef PROCESS_KH_DATA_HPP
#define PROCESS_KH_DATA_HPP

#include "common_defines.hpp"

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

/* Binary file header structure */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int variable_count;
} kh_file_header_t;

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
void kh_free_variables(kh_variable_t* variables, int count) {
    int i;
    if (variables) {
        for (i = 0; i < count; i++) {
            free(variables[i].name);
            free(variables[i].value);
        }
        free(variables);
    }
}

/* Find variable by name (case-insensitive) - optimized with early exit */
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

/* Read binary file and load all variables - improved error handling */
int kh_read_binary_file(const char* file_path, kh_variable_t** variables, int* count) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int i;
    unsigned int name_len, value_len, type_len;
    char* type_str = NULL;
    int success = 1;
    
    *variables = NULL;
    *count = 0;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0;
    }
    
    /* Read and validate header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    if (header.magic != KHDATA_MAGIC || header.version != KHDATA_VERSION) {
        fclose(file);
        return 0;
    }
    
    if (header.variable_count == 0) {
        fclose(file);
        return 1; /* Empty file is valid */
    }
    
    /* Security check: prevent excessive memory allocation */
    if (header.variable_count > 10000) {
        fclose(file);
        return 0;
    }
    
    /* Allocate variables array */
    *variables = (kh_variable_t*)calloc(header.variable_count, sizeof(kh_variable_t));
    if (!*variables) {
        fclose(file);
        return 0;
    }
    
    /* Read variables */
    for (i = 0; i < (int)header.variable_count && success; i++) {
        /* Read name */
        if (fread(&name_len, sizeof(unsigned int), 1, file) != 1 || name_len > 1024) {
            success = 0;
            break;
        }
        
        (*variables)[i].name = (char*)malloc(name_len + 1);
        if (!(*variables)[i].name) {
            success = 0;
            break;
        }
        
        if (fread((*variables)[i].name, name_len, 1, file) != 1) {
            success = 0;
            break;
        }
        (*variables)[i].name[name_len] = '\0';
        
        /* Read type */
        if (fread(&type_len, sizeof(unsigned int), 1, file) != 1 || type_len >= 64) {
            success = 0;
            break;
        }
        
        type_str = (char*)malloc(type_len + 1);
        if (!type_str) {
            success = 0;
            break;
        }
        
        if (fread(type_str, type_len, 1, file) != 1) {
            success = 0;
            break;
        }
        type_str[type_len] = '\0';
        (*variables)[i].type = kh_get_type_from_string(type_str);
        free(type_str);
        type_str = NULL;
        
        /* Read value */
        if (fread(&value_len, sizeof(unsigned int), 1, file) != 1 || value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
            success = 0;
            break;
        }
        
        (*variables)[i].value = (char*)malloc(value_len + 1);
        if (!(*variables)[i].value) {
            success = 0;
            break;
        }
        
        if (fread((*variables)[i].value, value_len, 1, file) != 1) {
            success = 0;
            break;
        }
        (*variables)[i].value[value_len] = '\0';
        
        (*count)++;
    }
    
    fclose(file);
    
    /* Cleanup on failure */
    if (!success) {
        if (type_str) free(type_str);
        kh_free_variables(*variables, *count);
        *variables = NULL;
        *count = 0;
        return 0;
    }
    
    return 1;
}

/* Write binary file with all variables - improved error handling */
static int kh_write_binary_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0)) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int i;
    unsigned int name_len, value_len, type_len;
    const char* type_str;
    int success = 1;
    
    if (fopen_s(&file, file_path, "wb") != 0) {
        return 0;
    }
    
    /* Write header */
    header.magic = KHDATA_MAGIC;
    header.version = KHDATA_VERSION;
    header.variable_count = (unsigned int)count;
    
    if (fwrite(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Write variables */
    for (i = 0; i < count && success; i++) {
        if (!variables[i].name || !variables[i].value) {
            success = 0;
            break;
        }
        
        /* Write name */
        name_len = (unsigned int)strlen(variables[i].name);
        if (fwrite(&name_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(variables[i].name, name_len, 1, file) != 1) {
            success = 0;
            break;
        }
        
        /* Write type */
        type_str = kh_get_string_from_type(variables[i].type);
        type_len = (unsigned int)strlen(type_str);
        if (fwrite(&type_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(type_str, type_len, 1, file) != 1) {
            success = 0;
            break;
        }
        
        /* Write value */
        value_len = (unsigned int)strlen(variables[i].value);
        if (fwrite(&value_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(variables[i].value, value_len, 1, file) != 1) {
            success = 0;
            break;
        }
    }
    
    fclose(file);
    return success;
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
            
        default:
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, value) >= 0);
    }
}

/* Parse Arma 3 hashmap entry in format [key, value] - improved memory management */
static int kh_parse_hashmap_entry(const char* input, char** key, char** value) {
    if (!input || !key || !value) return 0;
    
    const char* ptr = input;
    const char* bracket_start;
    const char* comma_pos;
    const char* bracket_end;
    int key_len, value_len;
    
    *key = NULL;
    *value = NULL;
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    if (*ptr != '[') return 0;
    
    bracket_start = ptr + 1;
    comma_pos = strchr(bracket_start, ',');
    if (!comma_pos) return 0;
    
    bracket_end = strrchr(comma_pos, ']');
    if (!bracket_end) return 0;
    
    /* Extract and clean key */
    key_len = (int)(comma_pos - bracket_start);
    while (key_len > 0 && (bracket_start[key_len-1] == ' ' || bracket_start[key_len-1] == '\t')) {
        key_len--;
    }
    
    if (key_len <= 0) return 0;
    
    *key = (char*)malloc((size_t)key_len + 1);
    if (!*key) return 0;
    
    memcpy(*key, bracket_start, (size_t)key_len);
    (*key)[key_len] = '\0';
    
    /* Remove quotes from key if present */
    if (key_len >= 2 && (*key)[0] == '"' && (*key)[key_len-1] == '"') {
        memmove(*key, *key + 1, (size_t)(key_len - 2));
        (*key)[key_len - 2] = '\0';
    }
    
    /* Extract and clean value */
    const char* value_start = comma_pos + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) value_start++;
    
    value_len = (int)(bracket_end - value_start);
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t')) {
        value_len--;
    }
    
    if (value_len <= 0) {
        free(*key);
        *key = NULL;
        return 0;
    }
    
    *value = (char*)malloc((size_t)value_len + 1);
    if (!*value) {
        free(*key);
        *key = NULL;
        return 0;
    }
    
    memcpy(*value, value_start, (size_t)value_len);
    (*value)[value_len] = '\0';
    
    return 1;
}

/* Parse a full hashmap and extract all entries - improved with dynamic allocation */
static int kh_parse_full_hashmap(const char* input, char*** entries, int max_entries) {
    if (!input || !entries) return 0;
    
    const char* ptr = input;
    int entry_count = 0;
    int bracket_depth = 0;
    const char* entry_start = NULL;
    
    /* Allocate array of string pointers */
    *entries = (char**)malloc((size_t)max_entries * sizeof(char*));
    if (!*entries) return 0;
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    
    if (*ptr != '[') {
        free(*entries);
        *entries = NULL;
        return 0;
    }
    ptr++; /* Skip opening bracket */
    
    while (*ptr && entry_count < max_entries) {
        if (*ptr == '[') {
            if (bracket_depth == 0) {
                entry_start = ptr;
            }
            bracket_depth++;
        } else if (*ptr == ']') {
            bracket_depth--;
            if (bracket_depth == 0 && entry_start) {
                /* Found complete entry */
                int entry_len = (int)(ptr - entry_start + 1);
                (*entries)[entry_count] = (char*)malloc((size_t)entry_len + 1);
                if ((*entries)[entry_count]) {
                    memcpy((*entries)[entry_count], entry_start, (size_t)entry_len);
                    (*entries)[entry_count][entry_len] = '\0';
                    entry_count++;
                }
                entry_start = NULL;
            }
        }
        ptr++;
    }
    
    return entry_count;
}

/* Enhanced hashmap merging with better memory management */
static int kh_merge_hashmap(const char* existing_hashmap, const char* new_entry, char* result, int result_size) {
    if (!existing_hashmap || !new_entry || !result || result_size <= 0) return 0;
    
    char** entries = NULL;
    int entry_count;
    char* key = NULL;
    char* value = NULL;
    char* formatted_entry = NULL;
    int ret = 0;
    
    /* Try to parse as full hashmap first */
    entry_count = kh_parse_full_hashmap(new_entry, &entries, 16);
    
    if (entry_count > 0) {
        /* Process each entry */
        size_t temp_size = (size_t)result_size * 2; /* Allocate more space for temp processing */
        char* temp_result = (char*)malloc(temp_size);
        if (!temp_result) goto cleanup;
        
        strcpy_s(temp_result, temp_size, existing_hashmap);
        
        for (int i = 0; i < entry_count; i++) {
            if (kh_parse_hashmap_entry(entries[i], &key, &value)) {
                /* Format the new entry */
                size_t formatted_len = strlen(key) + strlen(value) + 10;
                formatted_entry = (char*)malloc(formatted_len);
                if (!formatted_entry) goto cleanup;
                
                _snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", key, value);
                
                /* Add to temp_result */
                const char* trimmed = temp_result;
                while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
                
                if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                    _snprintf_s(temp_result, temp_size, _TRUNCATE, "[%s]", formatted_entry);
                } else {
                    size_t existing_len = strlen(temp_result);
                    if (existing_len > 0 && temp_result[existing_len-1] == ']') {
                        _snprintf_s(temp_result, temp_size, _TRUNCATE, "%.*s, %s]", (int)(existing_len - 1), temp_result, formatted_entry);
                    }
                }
                
                free(key);
                free(value);
                free(formatted_entry);
                key = NULL;
                value = NULL;
                formatted_entry = NULL;
            }
        }
        
        /* Copy result back */
        if (strlen(temp_result) < (size_t)result_size) {
            strcpy_s(result, (size_t)result_size, temp_result);
            ret = 1;
        }
        
        free(temp_result);
    } else {
        /* Fall back to single entry parsing */
        if (kh_parse_hashmap_entry(new_entry, &key, &value)) {
            size_t formatted_len = strlen(key) + strlen(value) + 10;
            formatted_entry = (char*)malloc(formatted_len);
            if (!formatted_entry) goto cleanup;
            
            _snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", key, value);
            
            /* Check if existing hashmap is empty */
            const char* trimmed = existing_hashmap;
            while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
            
            if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                _snprintf_s(result, (size_t)result_size, _TRUNCATE, "[%s]", formatted_entry);
            } else {
                size_t existing_len = strlen(existing_hashmap);
                if (existing_len > 0 && existing_hashmap[existing_len-1] == ']') {
                    _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.*s, %s]", (int)(existing_len - 1), existing_hashmap, formatted_entry);
                }
            }
            
            ret = 1;
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
    free(formatted_entry);
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
                double existing_num = atof(clean_existing);
                double new_num = atof(clean_new);
                _snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.6g", existing_num + new_num);
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

/* Enhanced read function with better error handling and memory management */
static int kh_read_khdata_variable_slice(const char* filename, const char* variable_name, const char* slice_index_str, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_PATH_LENGTH];
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
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
        slice_index = atoi(slice_index_str);
        if (slice_index < 0) {
            kh_set_error(output, output_size, "INVALID SLICE INDEX");
            goto cleanup;
        }
    }
    
    if (!kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_read_binary_file(file_path, &variables, &variable_count)) {
        kh_set_error(output, output_size, "FILE NOT FOUND OR CORRUPTED");
        goto cleanup;
    }
    
    /* Special case: return all variable names if var_name == filename */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        size_t result_size = (size_t)variable_count * 256 + 100;
        char* result_str = (char*)malloc(result_size);
        if (!result_str) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        
        strcpy_s(result_str, result_size, "[");
        for (int i = 0; i < variable_count; i++) {
            if (i > 0) strcat_s(result_str, result_size, ", ");
            strcat_s(result_str, result_size, "\"");
            strcat_s(result_str, result_size, variables[i].name);
            strcat_s(result_str, result_size, "\"");
        }
        strcat_s(result_str, result_size, "]");
        
        /* Handle slicing */
        if (slice_index >= 0) {
            long data_size = (long)strlen(result_str);
            long slice_start = (long)slice_index * SLICE_SIZE;
            long slice_end = slice_start + SLICE_SIZE;
            
            if (slice_start >= data_size) {
                kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
            } else {
                if (slice_end > data_size) slice_end = data_size;
                long slice_length = slice_end - slice_start;
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
    var_index = kh_find_variable(variables, variable_count, clean_var_name);
    if (var_index == -1) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        goto cleanup;
    }
    
    /* Format output */
    size_t formatted_len = strlen(variables[var_index].value) + 50;
    formatted_output = (char*)malloc(formatted_len);
    if (!formatted_output) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    if (!kh_format_variable_output(variables[var_index].type, variables[var_index].value, formatted_output, (int)formatted_len)) {
        kh_set_error(output, output_size, "OUTPUT FORMATTING FAILED");
        goto cleanup;
    }
    
    /* Handle slicing */
    if (slice_index >= 0) {
        long data_size = (long)strlen(formatted_output);
        long slice_start = (long)slice_index * SLICE_SIZE;
        long slice_end = slice_start + SLICE_SIZE;
        
        if (slice_start >= data_size) {
            kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
        } else {
            if (slice_end > data_size) slice_end = data_size;
            long slice_length = slice_end - slice_start;
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
    kh_free_variables(variables, variable_count);
    return result;
}

/* Enhanced write function with better memory management */
static int kh_write_khdata_variable(const char* filename, const char* variable_name, const char* value, 
                                  const char* type_str, const char* overwrite_str, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_PATH_LENGTH];
    char* clean_var_name = NULL;
    char* clean_value = NULL;
    char* combined_value = NULL;
    kh_data_type_t data_type;
    kh_variable_t* variables = NULL;
    int variable_count = 0;
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
    
    if (!kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        return 1;
    }
    
    /* Allocate memory for cleaned strings */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_value = (char*)malloc(strlen(value) + 1);
    combined_value = (char*)malloc(strlen(value) * 2 + 1024); /* Extra space for combination */
    
    if (!clean_var_name || !clean_value || !combined_value) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(value, clean_value, (int)strlen(value) + 1);

    /* Allow empty strings for certain data types */
    if (strlen(clean_value) == 0 && data_type != KH_TYPE_STRING && data_type != KH_TYPE_TEXT && data_type != KH_TYPE_CODE) {
        kh_set_error(output, output_size, "EMPTY VALUE");
        goto cleanup;
    }
    
    /* Read existing file */
    if (!kh_read_binary_file(file_path, &variables, &variable_count)) {
        variable_count = 0;
        variables = NULL;
    }
    
    /* Find existing variable */
    var_index = kh_find_variable(variables, variable_count, clean_var_name);
    
    if (var_index != -1 && !overwrite_flag) {
        /* Combine with existing value */
        if (!kh_combine_values(data_type, variables[var_index].value, clean_value, 
                             overwrite_flag, combined_value, (int)strlen(value) * 2 + 1024)) {
            kh_set_error(output, output_size, "VALUE COMBINATION FAILED");
            goto cleanup;
        }
        
        /* Update existing variable */
        free(variables[var_index].value);
        size_t combined_len = strlen(combined_value) + 1;
        variables[var_index].value = (char*)malloc(combined_len);
        if (!variables[var_index].value) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(variables[var_index].value, combined_len, combined_value);
        variables[var_index].type = data_type;
    } else {
        /* Add new variable or replace existing */
        if (var_index == -1) {
            variables = (kh_variable_t*)realloc(variables, (size_t)(variable_count + 1) * sizeof(kh_variable_t));
            if (!variables) {
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            var_index = variable_count++;
            
            size_t name_len = strlen(clean_var_name) + 1;
            variables[var_index].name = (char*)malloc(name_len);
            if (!variables[var_index].name) {
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            strcpy_s(variables[var_index].name, name_len, clean_var_name);
        } else {
            free(variables[var_index].value);
        }
        
        size_t value_len = strlen(clean_value) + 1;
        variables[var_index].value = (char*)malloc(value_len);
        if (!variables[var_index].value) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(variables[var_index].value, value_len, clean_value);
        variables[var_index].type = data_type;
    }
    
    /* Write file */
    if (!kh_write_binary_file(file_path, variables, variable_count)) {
        kh_set_error(output, output_size, "FAILED TO WRITE FILE");
        goto cleanup;
    }
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_var_name);
    free(clean_value);
    free(combined_value);
    kh_free_variables(variables, variable_count);
    return result;
}

#endif /* PROCESS_KH_DATA_HPP */