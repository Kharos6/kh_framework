#ifndef PROCESS_KH_DATA_H
#define PROCESS_KH_DATA_H

/* Variable data structure */
struct kh_variable_s {
    char* name;
    kh_type_t type;
    char* value;
};

/* Binary file header structure */
#pragma pack(push, 4)
typedef struct {
    unsigned int magic;              /* Magic number */
    unsigned int version;            /* Format version */
    unsigned int variable_count;     /* Number of variables */
    unsigned int hash_table_size;    /* Size of hash table (0 for old format) */
    unsigned int data_section_offset; /* Offset to data section */
} kh_file_header_t;
#pragma pack(pop)

/* In-memory file representation */
typedef struct {
    char* filename;  /* Just the filename without path */
    char* full_path; /* Full path to the file */
    kh_variable_t* variables;
    int variable_count;
    kh_generic_hash_table_t hash_info;
    int dirty;  /* Flag to track if file needs saving */
    int capacity; /* Current allocated capacity for variables */
} khdata_file_t;

/* Memory manager for all .khdata files */
typedef struct {
    khdata_file_t* files;
    int file_count;
    int file_capacity;
    int initialized;
    char base_path[MAX_FILE_PATH_LENGTH]; /* Base path to kh_data folder */
} khdata_memory_manager_t;

/* Global memory manager instance */
static khdata_memory_manager_t g_memory_manager = {0};

/* Free variables array */
static inline void kh_free_variables(kh_variable_t* variables, int count) {
    int i;
    if (variables) {
        for (i = 0; i < count; i++) {
            free(variables[i].name);
            free(variables[i].value);
        }
        free(variables);
    }
}

/* Free hash table info structure */
static inline void kh_free_hash_table_info(kh_generic_hash_table_t* hash_info) {
    if (hash_info) {
        free(hash_info->entries);
        hash_info->entries = NULL;
        hash_info->size = 0;
        hash_info->used_count = 0;
        hash_info->deleted_count = 0;
        hash_info->needs_rebuild = 0;
    }
}

/* Free a single khdata file structure */
static inline void kh_free_khdata_file(khdata_file_t* file) {
    if (!file) return;
    
    free(file->filename);
    free(file->full_path);
    kh_free_variables(file->variables, file->variable_count);
    kh_free_hash_table_info(&file->hash_info);
    
    memset(file, 0, sizeof(khdata_file_t));
}

/* Optimized hash table rebuild - only when necessary */
static inline int kh_rebuild_hash_table(khdata_file_t* file) {
    if (!file || file->variable_count <= 0) return 1;
    
    if (!kh_generic_hash_needs_rebuild(&file->hash_info, file->variable_count)) {
        return 1;
    }
    
    kh_free_hash_table_info(&file->hash_info);
    
    uint32_t hash_table_size = kh_calculate_optimal_hash_size(file->variable_count);
    
    file->hash_info.entries = (kh_generic_hash_entry_t*)calloc(hash_table_size, sizeof(kh_generic_hash_entry_t));
    if (!file->hash_info.entries) return 0;
    
    file->hash_info.size = hash_table_size;
    file->hash_info.used_count = 0;
    file->hash_info.deleted_count = 0;
    file->hash_info.needs_rebuild = 0;
    
    for (int i = 0; i < file->variable_count; i++) {
        if (file->variables[i].name) {
            uint32_t name_hash = kh_hash_name_case_insensitive(file->variables[i].name);
            if (!kh_generic_hash_insert(file->hash_info.entries, hash_table_size, name_hash, i)) {
                kh_free_hash_table_info(&file->hash_info);
                return 0;
            }
            file->hash_info.used_count++;
        }
    }
    
    return 1;
}

/* Smart hash table update - add single entry without full rebuild */
static inline int kh_hash_table_add_entry(kh_generic_hash_table_t* hash_info, const char* name, int var_index) {
    if (!hash_info || !name || var_index < 0) return 0;
    
    /* Check if we need to rebuild first */
    if (kh_generic_hash_needs_rebuild(hash_info, hash_info->used_count + 1)) {
        hash_info->needs_rebuild = 1;
        return 0; /* Signal that rebuild is needed */
    }
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    if (kh_generic_hash_insert(hash_info->entries, hash_info->size, name_hash, var_index)) {
        hash_info->used_count++;
        return 1;
    }
    
    /* Insertion failed, mark for rebuild */
    hash_info->needs_rebuild = 1;
    return 0;
}

/* Smart hash table removal - mark deleted without full rebuild */
static inline int kh_hash_table_remove_entry(kh_generic_hash_table_t* hash_info, const char* name) {
    if (!hash_info || !name) return 0;
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    if (kh_generic_hash_delete(hash_info->entries, hash_info->size, name_hash)) {
        hash_info->used_count--;
        hash_info->deleted_count++;
        
        /* Check if we have too many deletions */
        if (hash_info->deleted_count > hash_info->used_count / 2) {
            hash_info->needs_rebuild = 1;
        }
        
        return 1;
    }
    
    return 0;
}

/* Find file in memory by filename */
static inline khdata_file_t* kh_find_file_in_memory(const char* filename) {
    if (!filename) return NULL;
    
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (g_memory_manager.files[i].filename && 
            kh_strcasecmp(g_memory_manager.files[i].filename, filename) == 0) {
            return &g_memory_manager.files[i];
        }
    }
    
    return NULL;
}

/* Check if file is in binary format and get version */
static int kh_get_file_format_version(const char* file_path) {
    if (!file_path) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int result = 0;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0; /* File doesn't exist or can't be opened */
    }
    
    /* Try to read header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) == 1) {
        /* Check magic number */
        if (header.magic == KHDATA_MAGIC) {
            result = 1;
        }
    }
    
    fclose(file);
    return result;
}

/* Check if file is already in binary format */
static int kh_is_file_binary(const char* file_path) {
    return kh_get_file_format_version(file_path);
}

/* Parse a single line from text format: variable_name=TYPE:value */
static int kh_parse_text_line(const char* line, char** name, kh_type_t* type, char** value) {
    if (!line || !name || !type || !value) return 0;
    
    *name = NULL;
    *value = NULL;
    *type = KH_TYPE_UNKNOWN;
    
    /* Skip empty lines and comments */
    const char* ptr = line;
    while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
    if (*ptr == '\0' || *ptr == '#' || *ptr == '\n' || *ptr == '\r') {
        return 0;
    }
    
    /* Find the equals sign (not inside quotes) */
    const char* equals_pos = NULL;
    int in_quotes = 0;
    const char* scan = ptr;
    char string_quote = 0;
    
    while (*scan) {
        if (!in_quotes) {
            if (*scan == '=') {
                equals_pos = scan;
                break;
            } else if (*scan == '"' || *scan == '\'') {
                string_quote = *scan;
                in_quotes = 1;
            }
        } else {
            if (*scan == string_quote) {
                in_quotes = 0;
            }
        }
        scan++;
    }
    
    if (!equals_pos) return 0;
    
    /* Find the colon that separates type from value (not inside quotes) */
    const char* colon_pos = NULL;
    scan = equals_pos + 1;
    in_quotes = 0;
    
    while (*scan) {
        if (!in_quotes) {
            if (*scan == ':') {
                colon_pos = scan;
                break;
            } else if (*scan == '"' || *scan == '\'') {
                string_quote = *scan;
                in_quotes = 1;
            }
        } else {
            if (*scan == string_quote) {
                in_quotes = 0;
            }
        }
        scan++;
    }
    
    if (!colon_pos) return 0;
    
    /* Extract variable name */
    int name_len = (int)(equals_pos - ptr);
    while (name_len > 0 && (ptr[name_len-1] == ' ' || ptr[name_len-1] == '\t')) {
        name_len--;
    }
    
    if (name_len <= 0) return 0;
    
    *name = (char*)malloc((size_t)name_len + 1);
    if (!*name) return 0;
    
    memcpy(*name, ptr, (size_t)name_len);
    (*name)[name_len] = '\0';
    
    /* Extract type */
    const char* type_start = equals_pos + 1;
    while (*type_start && (*type_start == ' ' || *type_start == '\t')) type_start++;
    
    int type_len = (int)(colon_pos - type_start);
    while (type_len > 0 && (type_start[type_len-1] == ' ' || type_start[type_len-1] == '\t')) {
        type_len--;
    }
    
    if (type_len <= 0) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    char* type_str = (char*)malloc((size_t)type_len + 1);
    if (!type_str) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    memcpy(type_str, type_start, (size_t)type_len);
    type_str[type_len] = '\0';
    
    *type = kh_get_type_from_string(type_str);
    free(type_str);
    
    if (*type == KH_TYPE_UNKNOWN) {
        free(*name);
        *name = NULL;
        return 0;
    }
    
    /* Extract value (rest of line after colon) */
    const char* value_start = colon_pos + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) value_start++;
    
    /* Calculate value length, trimming trailing whitespace and newlines */
    int value_len = (int)strlen(value_start);
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t' || 
                            value_start[value_len-1] == '\n' || value_start[value_len-1] == '\r')) {
        value_len--;
    }
    
    *value = (char*)malloc((size_t)value_len + 1);
    if (!*value) {
        free(*name);
        *name = NULL;
        *type = KH_TYPE_UNKNOWN;
        return 0;
    }
    
    if (value_len > 0) {
        memcpy(*value, value_start, (size_t)value_len);
    }
    (*value)[value_len] = '\0';
    
    return 1;
}

/* Read text format file and convert to variables array */
static int kh_read_text_file(const char* file_path, kh_variable_t** variables, int* count) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file = NULL;
    char* line_buffer = NULL;
    kh_variable_t* temp_variables = NULL;
    int temp_count = 0;
    int temp_capacity = 16;
    int success = 0;
    
    *variables = NULL;
    *count = 0;
    
    if (fopen_s(&file, file_path, "r") != 0) {
        return 0;
    }
    
    /* Allocate initial arrays */
    int buffer_size = 1024;
    line_buffer = (char*)malloc((size_t)buffer_size);
    temp_variables = (kh_variable_t*)calloc((size_t)temp_capacity, sizeof(kh_variable_t));
    
    if (!line_buffer || !temp_variables) {
        goto cleanup;
    }
    
    /* Read line by line */
    while (fgets(line_buffer, buffer_size, file)) {
        /* Resize line buffer if needed */
        while (strlen(line_buffer) == (size_t)buffer_size - 1 && line_buffer[buffer_size - 2] != '\n') {
            buffer_size *= 2;
            char* new_buffer = (char*)realloc(line_buffer, (size_t)buffer_size);
            if (!new_buffer) {
                goto cleanup;
            }
            line_buffer = new_buffer;
            
            /* Continue reading the rest of the line */
            if (!fgets(line_buffer + strlen(line_buffer), buffer_size - (int)strlen(line_buffer), file)) {
                break;
            }
        }
        
        char* name = NULL;
        kh_type_t type;
        char* value = NULL;
        
        if (kh_parse_text_line(line_buffer, &name, &type, &value)) {
            /* Resize variables array if needed */
            if (temp_count >= temp_capacity) {
                temp_capacity *= 2;
                kh_variable_t* new_variables = (kh_variable_t*)realloc(temp_variables, 
                                                (size_t)temp_capacity * sizeof(kh_variable_t));
                if (!new_variables) {
                    free(name);
                    free(value);
                    goto cleanup;
                }
                temp_variables = new_variables;
                
                /* Initialize new memory to zero */
                memset(&temp_variables[temp_count], 0, 
                       (size_t)(temp_capacity - temp_count) * sizeof(kh_variable_t));
            }
            
            /* Store the variable */
            temp_variables[temp_count].name = name;
            temp_variables[temp_count].type = type;
            temp_variables[temp_count].value = value;
            temp_count++;
        }
    }
    
    /* Success - transfer ownership */
    *variables = temp_variables;
    *count = temp_count;
    temp_variables = NULL; /* Prevent cleanup */
    success = 1;

cleanup:
    if (file) {
        fclose(file);
    }
    free(line_buffer);
    
    if (!success && temp_variables) {
        /* Free all allocated variables on failure */
        for (int i = 0; i < temp_count; i++) {
            free(temp_variables[i].name);
            free(temp_variables[i].value);
        }
        free(temp_variables);
    }
    
    return success;
}

/* Write variables array to text format file */
static int kh_write_text_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0)) return 0;
    
    FILE* file;
    int success = 1;
    
    if (fopen_s(&file, file_path, "w") != 0) {
        return 0;
    }
    
    /* Write header comment */
    fprintf(file, "# Unbinarized KHDATA\n");
    fprintf(file, "# Format: variable_name=type:value\n");
    
    /* Write variables */
    for (int i = 0; i < count; i++) {
        if (!variables[i].name || !variables[i].value) {
            success = 0;
            break;
        }
        
        const char* type_str = kh_get_string_from_type(variables[i].type);
        
        if (fprintf(file, "%s=%s:%s\n", 
                   variables[i].name, type_str, variables[i].value) < 0) {
            success = 0;
            break;
        }
    }
    
    fclose(file);
    return success;
}

/* Validate file integrity before reading */
static int kh_validate_file_integrity(const char* file_path, long file_size) {
    if (!file_path || file_size < 0) return 0;
    
    /* File must be at least large enough for header */
    if (file_size < (long)sizeof(kh_file_header_t)) return 0;
    
    /* File shouldn't be larger than our maximum limit */
    if (file_size > MAX_TOTAL_KHDATA_SIZE_BYTES) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    int result = 1;
    
    if (fopen_s(&file, file_path, "rb") != 0) return 0;
    
    /* Read header for basic validation */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Validate magic number and version */
    if (header.magic != KHDATA_MAGIC) {
        result = 0;
    }
    
    if (result && header.hash_table_size > 0 && header.hash_table_size < KH_HASH_TABLE_MIN_SIZE) {
        result = 0;
    }
    
    if (result && header.data_section_offset >= (uint32_t)file_size) {
        result = 0;
    }
    
    fclose(file);
    return result;
}

/* Get file size in bytes */
static inline long kh_get_file_size(const char* file_path) {
    if (!file_path) return -1;
    
    FILE* file;
    long size;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return -1;
    }
    
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    
    return size;
}

/* Read binary file with hash table support - returns hash table info */
static int kh_read_binary_file_with_hash(const char* file_path, kh_variable_t** variables, int* count, kh_generic_hash_table_t* hash_info) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_variable_t* temp_variables = NULL;
    int success = 0;
    long file_size = 0;
    long bytes_read = 0;
    
    *variables = NULL;
    *count = 0;
    if (hash_info) {
        hash_info->entries = NULL;
        hash_info->size = 0;
        hash_info->used_count = 0;
        hash_info->deleted_count = 0;
        hash_info->needs_rebuild = 0;
    }
    
    /* Get and validate file size */
    file_size = kh_get_file_size(file_path);
    if (file_size <= 0 || file_size > MAX_TOTAL_KHDATA_SIZE_BYTES) {
        return 0;
    }
    
    /* Validate file integrity */
    if (!kh_validate_file_integrity(file_path, file_size)) {
        return 0;
    }
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0;
    }
    
    /* Read and validate header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    bytes_read += sizeof(kh_file_header_t);
    
    if (header.magic != KHDATA_MAGIC) {
        fclose(file);
        return 0;
    }
    
    /* Validate variable count */
    if (header.variable_count > 0 && bytes_read >= file_size) {
        fclose(file);
        return 0;
    }
    
    if (header.variable_count == 0) {
        fclose(file);
        return 1; /* Empty file is valid */
    }
    
    /* Read hash table */
    if (header.hash_table_size > 0 && hash_info) {
        /* Upper bound validation for hash_table_size */
        
        if (header.hash_table_size < KH_HASH_TABLE_MIN_SIZE ||
            header.hash_table_size > 65535) {  /* Also check uint16_t distance limit */
            fclose(file);
            return 0;
        }
        
        /* Additional check: ensure hash table doesn't exceed file size */
        size_t hash_table_bytes = header.hash_table_size * sizeof(kh_generic_hash_entry_t);
        if (hash_table_bytes > (size_t)(file_size - sizeof(kh_file_header_t))) {
            fclose(file);
            return 0;
        }
        
        hash_info->entries = (kh_generic_hash_entry_t*)calloc(header.hash_table_size, sizeof(kh_generic_hash_entry_t));
        if (!hash_info->entries) {
            fclose(file);
            return 0;
        }
        hash_info->size = header.hash_table_size;
        
        if (fread(hash_info->entries, sizeof(kh_generic_hash_entry_t), header.hash_table_size, file) != header.hash_table_size) {
            kh_free_hash_table_info(hash_info);
            fclose(file);
            return 0;
        }
        bytes_read += header.hash_table_size * sizeof(kh_generic_hash_entry_t);
        
        /* Count used and deleted entries */
        for (uint32_t i = 0; i < header.hash_table_size; i++) {
            if (hash_info->entries[i].name_hash != KH_HASH_EMPTY) {
                if (hash_info->entries[i].deleted) {
                    hash_info->deleted_count++;
                } else {
                    hash_info->used_count++;
                }
            }
        }
        
        /* Seek to data section */
        if (fseek(file, header.data_section_offset, SEEK_SET) != 0) {
            kh_free_hash_table_info(hash_info);
            fclose(file);
            return 0;
        }
        bytes_read = header.data_section_offset;
    }
    
    /* Allocate variables array */
    temp_variables = (kh_variable_t*)calloc(header.variable_count, sizeof(kh_variable_t));
    if (!temp_variables) {
        if (hash_info) kh_free_hash_table_info(hash_info);
        fclose(file);
        return 0;
    }
    
    /* Read variables with comprehensive bounds checking */
    for (unsigned int i = 0; i < header.variable_count; i++) {
        unsigned int name_len, value_len, type_len;
        char* type_str = NULL;
        
        /* Read name length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&name_len, sizeof(unsigned int), 1, file) != 1 || 
            name_len == 0) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Check bounds for name data */
        if (bytes_read + name_len > file_size) {
            goto cleanup_failure;
        }
        
        /* Allocate and read name */
        temp_variables[i].name = (char*)malloc(name_len + 1);
        if (!temp_variables[i].name || 
            fread(temp_variables[i].name, name_len, 1, file) != 1) {
            goto cleanup_failure;
        }
        temp_variables[i].name[name_len] = '\0';
        bytes_read += name_len;
        
        /* Validate name contains only printable characters */
        for (unsigned int j = 0; j < name_len; j++) {
            if (temp_variables[i].name[j] < 32 || temp_variables[i].name[j] == 127) {
                goto cleanup_failure;
            }
        }
        
        /* Read type length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&type_len, sizeof(unsigned int), 1, file) != 1 || 
            type_len == 0 || type_len >= 64) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Check bounds for type data */
        if (bytes_read + type_len > file_size) {
            goto cleanup_failure;
        }
        
        /* Allocate and read type */
        type_str = (char*)malloc(type_len + 1);
        if (!type_str || fread(type_str, type_len, 1, file) != 1) {
            free(type_str);
            goto cleanup_failure;
        }
        type_str[type_len] = '\0';
        bytes_read += type_len;
        
        /* Validate and convert type */
        temp_variables[i].type = kh_get_type_from_string(type_str);
        free(type_str);
        type_str = NULL;
        
        if (temp_variables[i].type == KH_TYPE_UNKNOWN) {
            goto cleanup_failure;
        }
        
        /* Read value length with bounds checking */
        if (bytes_read + sizeof(unsigned int) > file_size || 
            fread(&value_len, sizeof(unsigned int), 1, file) != 1 || 
            value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
            goto cleanup_failure;
        }
        bytes_read += sizeof(unsigned int);
        
        /* Allocate value buffer */
        temp_variables[i].value = (char*)malloc(value_len + 1);
        if (!temp_variables[i].value) {
            goto cleanup_failure;
        }
        
        /* Read value data if present */
        if (value_len > 0) {
            if (bytes_read + value_len > file_size || 
                fread(temp_variables[i].value, value_len, 1, file) != 1) {
                goto cleanup_failure;
            }
            bytes_read += value_len;
        }
        
        temp_variables[i].value[value_len] = '\0';
        (*count)++;
    }
    
    /* Success */
    *variables = temp_variables;
    success = 1;
    temp_variables = NULL;
    
cleanup_failure:
    fclose(file);
    
    if (!success) {
        if (temp_variables) {
            kh_free_variables(temp_variables, *count);
        }
        if (hash_info) {
            kh_free_hash_table_info(hash_info);
        }
        *variables = NULL;
        *count = 0;
    }
    
    return success;
}

/* Check if filename is a Windows reserved device name */
static inline int kh_is_reserved_device_name(const char* name) {
    if (!name) return 0;
    
    /* Extract base name without extension */
    char base_name[32];
    const char* dot_pos = strchr(name, '.');
    size_t base_len = dot_pos ? (size_t)(dot_pos - name) : strlen(name);
    
    if (base_len == 0 || base_len >= sizeof(base_name)) {
        return 0;
    }
    
    /* Copy and uppercase for comparison */
    size_t i;
    for (i = 0; i < base_len; i++) {
        base_name[i] = (name[i] >= 'a' && name[i] <= 'z') ? 
                       (name[i] - 32) : name[i];
    }
    base_name[base_len] = '\0';
    
    /* Check 3-letter reserved names */
    if (base_len == 3) {
        if (strcmp(base_name, "CON") == 0 ||
            strcmp(base_name, "PRN") == 0 ||
            strcmp(base_name, "AUX") == 0 ||
            strcmp(base_name, "NUL") == 0) {
            return 1;
        }
    }
    
    /* Check COM1-COM9 */
    if (base_len == 4 && base_name[0] == 'C' && 
        base_name[1] == 'O' && base_name[2] == 'M' &&
        base_name[3] >= '1' && base_name[3] <= '9') {
        return 1;
    }
    
    /* Check LPT1-LPT9 */
    if (base_len == 4 && base_name[0] == 'L' && 
        base_name[1] == 'P' && base_name[2] == 'T' &&
        base_name[3] >= '1' && base_name[3] <= '9') {
        return 1;
    }
    
    return 0;
}

/* Get the user's Documents\Arma 3 folder path */
static inline int kh_get_arma3_documents_path(char* path, int path_size) {
    if (!path || path_size <= 0) return 0;
    
    char documents_path[MAX_PATH];
    char arma3_path[MAX_PATH];
    char framework_path[MAX_PATH];
    
    if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path) != S_OK) {
        return 0;
    }
    
    /* Build Documents\Arma 3 path */
    if (_snprintf_s(arma3_path, sizeof(arma3_path), _TRUNCATE, "%s\\Arma 3", documents_path) < 0) {
        return 0;
    }
    
    /* Build Documents\Arma 3\kh_framework path */
    if (_snprintf_s(framework_path, sizeof(framework_path), _TRUNCATE, "%s\\kh_framework", arma3_path) < 0) {
        return 0;
    }
    
    /* Build final Documents\Arma 3\kh_framework\kh_data path */
    if (_snprintf_s(path, (size_t)path_size, _TRUNCATE, "%s\\kh_data", framework_path) < 0) {
        return 0;
    }
    
    /* Create directories if they don't exist (nested creation) */
    CreateDirectoryA(arma3_path, NULL);      /* Create Arma 3 folder */
    CreateDirectoryA(framework_path, NULL);   /* Create kh_framework folder */
    CreateDirectoryA(path, NULL);             /* Create kh_data folder */
    
    return 1;
}

/* Validate filename for security - prevent path traversal and enforce .khdata extension */
static inline int kh_validate_filename(const char* filename) {
    if (!filename) return 0;
    
    int filename_len = (int)strlen(filename);
    
    /* Check for empty filename */
    if (filename_len == 0 || filename_len > MAX_FILE_PATH_LENGTH) return 0;
    
    /* First, check for simple dangerous patterns */
    if (strchr(filename, '/') || strchr(filename, '\\') || 
        strchr(filename, ':') || strstr(filename, "..")) {
        return 0;
    }
    
    /* Check for reserved device names */
    if (kh_is_reserved_device_name(filename)) {
        return 0;
    }
    
    /* Get the base path for validation */
    char base_path[MAX_FILE_PATH_LENGTH];
    if (!kh_get_arma3_documents_path(base_path, sizeof(base_path))) {
        return 0;
    }
    
    /* Build the full path for validation */
    char test_path[MAX_FILE_PATH_LENGTH];
    if (_snprintf_s(test_path, sizeof(test_path), _TRUNCATE, 
                    "%s\\%s", base_path, filename) < 0) {
        return 0;
    }
    
    /* Use GetFullPathName to resolve the path */
    char resolved_path[MAX_FILE_PATH_LENGTH];
    char* file_part = NULL;
    DWORD result = GetFullPathNameA(test_path, sizeof(resolved_path), 
                                    resolved_path, &file_part);
    
    if (result == 0 || result >= sizeof(resolved_path)) {
        return 0; /* Path resolution failed or path too long */
    }
    
    /* Convert paths to lowercase for comparison */
    char resolved_lower[MAX_FILE_PATH_LENGTH];
    char base_lower[MAX_FILE_PATH_LENGTH];
    
    strcpy_s(resolved_lower, sizeof(resolved_lower), resolved_path);
    strcpy_s(base_lower, sizeof(base_lower), base_path);
    
    CharLowerA(resolved_lower);
    CharLowerA(base_lower);
    
    /* Ensure the resolved path starts with our base path */
    if (strncmp(resolved_lower, base_lower, strlen(base_lower)) != 0) {
        return 0; /* Path traversal detected */
    }
    
    /* Additional validation for the filename itself */
    for (int i = 0; i < filename_len; i++) {
        char c = filename[i];
        
        /* Reject dangerous characters */
        if (c == '*' || c == '?' || c == '"' || c == '<' || 
            c == '>' || c == '|' || c == '\0') {
            return 0;
        }
        
        /* Reject control characters */
        if (c < 32 || c == 127) {
            return 0;
        }
        
        /* Reject non-ASCII for security */
        if ((unsigned char)c > 127) {
            return 0;
        }
    }
    
    /* Check for leading/trailing dots or spaces */
    if (filename[0] == '.' || filename[0] == ' ' || 
        filename[filename_len-1] == '.' || filename[filename_len-1] == ' ') {
        return 0;
    }
    
    return 1; /* Filename is valid */
}

/* Count existing .khdata files in Arma 3 documents folder */
static inline int kh_count_khdata_files(void) {
    char arma3_path[MAX_FILE_PATH_LENGTH];
    char search_pattern[MAX_FILE_PATH_LENGTH];
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    int file_count = 0;
    
    if (!kh_get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return -1;
    }
    
    _snprintf_s(search_pattern, sizeof(search_pattern), _TRUNCATE, "%s\\*.khdata", arma3_path);
    
    find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return 0; /* No files found */
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            file_count++;
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return file_count;
}

/* Get full path to a .khdata file */
static inline int kh_get_khdata_file_path(const char* filename, char* full_path, int path_size) {
    if (!filename || !full_path || path_size <= 0) return 0;
    
    char arma3_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int filename_len = (int)strlen(filename);
    int result = 0;
    
    if (!kh_get_arma3_documents_path(arma3_path, sizeof(arma3_path))) {
        return 0;
    }
    
    if (filename_len >= MAX_FILE_PATH_LENGTH - 8) {
        return 0; /* Filename too long */
    }
    
    /* Allocate space for clean filename */
    clean_filename = (char*)malloc((size_t)filename_len + 8); /* +8 for .khdata extension */
    if (!clean_filename) return 0;
    
    /* Clean filename - remove quotes */
    kh_clean_string(filename, clean_filename, filename_len + 1);
    int clean_len = (int)strlen(clean_filename);
    
    /* Validate filename for security - prevent path traversal */
    if (!kh_validate_filename(clean_filename)) {
        free(clean_filename);
        return 0;
    }
    
    /* Add .khdata extension if not present */
    if (clean_len < 7 || strcmp(clean_filename + clean_len - 7, ".khdata") != 0) {
        strcat_s(clean_filename, (size_t)filename_len + 8, ".khdata");
    }
    
    /* Build full path */
    result = (_snprintf_s(full_path, (size_t)path_size, _TRUNCATE, "%s\\%s", arma3_path, clean_filename) >= 0) ? 1 : 0;
    
    free(clean_filename);
    return result;
}

/* Check if creating a new file would exceed the limit */
static inline int kh_check_file_limit(const char* filename, char* error_output, int error_size) {
    if (!filename || !error_output || error_size <= 0) return 0;
    
    char file_path[MAX_FILE_PATH_LENGTH];
    int current_count = kh_count_khdata_files();
    
    if (current_count < 0) {
        kh_set_error(error_output, error_size, "FAILED TO COUNT EXISTING FILES");
        return 0;
    }
    
    /* Check if file already exists (if so, we're not creating a new one) */
    if (kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        DWORD file_attributes = GetFileAttributesA(file_path);
        if (file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return 1; /* File exists, no new file creation */
        }
    }
    
    /* Check if we would exceed the limit */
    if (current_count >= MAX_KHDATA_FILES) {
        _snprintf_s(error_output, (size_t)error_size, _TRUNCATE, KH_ERROR_PREFIX "MAXIMUM %d KHDATA FILES EXCEEDED", MAX_KHDATA_FILES);
        return 0;
    }
    
    return 1;
}

/* Write binary file with hash table indexing */
static int kh_write_binary_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0) || count < 0) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_generic_hash_entry_t* hash_table = NULL;
    char* temp_file_path = NULL;
    int success = 0;
    uint32_t hash_table_size = 0;
    uint32_t data_section_offset = 0;
    
    /* Validate input data before writing */
    for (int i = 0; i < count; i++) {
        if (!variables[i].name || !variables[i].value) {
            return 0;
        }
        
        /* Validate lengths */
        size_t name_len = strlen(variables[i].name);
        size_t value_len = strlen(variables[i].value);
        
        if (name_len == 0 || value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
            return 0;
        }
        
        /* Validate type */
        if (variables[i].type == KH_TYPE_UNKNOWN) {
            return 0;
        }
        
        /* Validate name contains only safe characters */
        for (size_t j = 0; j < name_len; j++) {
            if (variables[i].name[j] < 32 || variables[i].name[j] == 127) {
                return 0;
            }
        }
    }
    
    /* Create hash table for indexed access */
    if (count > 0) {
        hash_table_size = kh_calculate_optimal_hash_size(count);
        hash_table = (kh_generic_hash_entry_t*)calloc(hash_table_size, sizeof(kh_generic_hash_entry_t));
        if (!hash_table) {
            return 0;
        }
        
        /* Build hash table using Robin Hood hashing */
        for (int i = 0; i < count; i++) {
            uint32_t name_hash = kh_hash_name_case_insensitive(variables[i].name);
            if (!kh_generic_hash_insert(hash_table, hash_table_size, name_hash, i)) {
                /* Hash table insertion failed - should not happen with proper sizing */
                free(hash_table);
                return 0;
            }
        }
    }
    
    /* Calculate data section offset with proper alignment */
    data_section_offset = sizeof(kh_file_header_t) + (hash_table_size * sizeof(kh_generic_hash_entry_t));
    /* Align to 8-byte boundary for better performance */
    data_section_offset = (data_section_offset + 7) & ~7;
    
    /* Create temporary file path for atomic write */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10);
    if (!temp_file_path) {
        free(hash_table);
        return 0;
    }
    
    if (strcpy_s(temp_file_path, path_len + 10, file_path) != 0 ||
        strcat_s(temp_file_path, path_len + 10, ".tmp") != 0) {
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Open temporary file for writing */
    if (fopen_s(&file, temp_file_path, "wb") != 0) {
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Write header (indexed format) */
    memset(&header, 0, sizeof(header)); /* Clear reserved fields */
    header.magic = KHDATA_MAGIC;
    header.variable_count = (unsigned int)count;
    header.hash_table_size = hash_table_size;
    header.data_section_offset = data_section_offset;
    
    if (fwrite(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        goto cleanup;
    }
    
    /* Write hash table */
    if (hash_table_size > 0) {
        if (fwrite(hash_table, sizeof(kh_generic_hash_entry_t), hash_table_size, file) != hash_table_size) {
            goto cleanup;
        }
    }
    
    /* Pad to data section alignment */
    long current_pos = ftell(file);
    if (current_pos < 0) {
        goto cleanup;
    }
    
    while ((uint32_t)current_pos < data_section_offset) {
        if (fputc(0, file) == EOF) {
            goto cleanup;
        }
        current_pos++;
    }
    
    /* Write variables */
    for (int i = 0; i < count; i++) {
        unsigned int name_len = (unsigned int)strlen(variables[i].name);
        unsigned int value_len = (unsigned int)strlen(variables[i].value);
        const char* type_str = kh_get_string_from_type(variables[i].type);
        unsigned int type_len = (unsigned int)strlen(type_str);
        
        /* Write name */
        if (fwrite(&name_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(variables[i].name, name_len, 1, file) != 1) {
            goto cleanup;
        }
        
        /* Write type */
        if (fwrite(&type_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(type_str, type_len, 1, file) != 1) {
            goto cleanup;
        }
        
        /* Write value */
        if (fwrite(&value_len, sizeof(unsigned int), 1, file) != 1) {
            goto cleanup;
        }
        
        if (value_len > 0) {
            if (fwrite(variables[i].value, value_len, 1, file) != 1) {
                goto cleanup;
            }
        }
    }
    
    /* Flush and close file */
    if (fflush(file) != 0) {
        goto cleanup;
    }
    
    /* Mark success before closing */
    success = 1;

cleanup:
    if (file) {
        fclose(file);
        file = NULL;
    }
    
    if (success) {
        /* Atomic replace: move temp file to final location */
        if (DeleteFileA(file_path) || GetLastError() == ERROR_FILE_NOT_FOUND) {
            if (!MoveFileA(temp_file_path, file_path)) {
                success = 0;
            }
        } else {
            success = 0;
        }
    }
    
    /* Clean up temp file if operation failed */
    if (!success && temp_file_path) {
        DeleteFileA(temp_file_path);
    }
    
    free(hash_table);
    free(temp_file_path);
    return success;
}

/* Load a single file from disk into memory */
static int kh_load_file_into_memory(const char* filename) {
    if (!filename) return 0;
    
    char full_path[MAX_FILE_PATH_LENGTH];
    khdata_file_t* file_slot = NULL;
    
    /* Get full path */
    if (!kh_get_khdata_file_path(filename, full_path, sizeof(full_path))) {
        return 0;
    }
    
    /* Check if file exists */
    if (GetFileAttributesA(full_path) == INVALID_FILE_ATTRIBUTES) {
        return 0; /* File doesn't exist */
    }
    
    /* Check if already loaded */
    file_slot = kh_find_file_in_memory(filename);
    if (file_slot) {
        return 1; /* Already loaded */
    }
    
    /* Find empty slot or resize array */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (!g_memory_manager.files[i].filename) {
            file_slot = &g_memory_manager.files[i];
            break;
        }
    }
    
    if (!file_slot) {
        /* Need to resize array */
        if (g_memory_manager.file_count >= g_memory_manager.file_capacity) {
            int new_capacity = g_memory_manager.file_capacity * 2;
            if (new_capacity < 16) new_capacity = 16;
            
            khdata_file_t* new_files = (khdata_file_t*)realloc(g_memory_manager.files, 
                                                               new_capacity * sizeof(khdata_file_t));
            if (!new_files) return 0;
            
            /* Initialize new slots */
            memset(&new_files[g_memory_manager.file_capacity], 0, 
                   (new_capacity - g_memory_manager.file_capacity) * sizeof(khdata_file_t));
            
            g_memory_manager.files = new_files;
            g_memory_manager.file_capacity = new_capacity;
        }
        
        file_slot = &g_memory_manager.files[g_memory_manager.file_count++];
    }
    
    /* Initialize file slot */
    memset(file_slot, 0, sizeof(khdata_file_t));
    
    /* Convert to binary format if needed */
    if (!kh_is_file_binary(full_path)) {
        /* Read as text and convert */
        kh_variable_t* temp_variables = NULL;
        int temp_count = 0;
        
        if (!kh_read_text_file(full_path, &temp_variables, &temp_count)) {
            return 0;
        }
        
        /* Write as binary */
        if (!kh_write_binary_file(full_path, temp_variables, temp_count)) {
            kh_free_variables(temp_variables, temp_count);
            return 0;
        }
        
        kh_free_variables(temp_variables, temp_count);
    }
    
    /* Read binary file */
    if (!kh_read_binary_file_with_hash(full_path, &file_slot->variables, 
                                       &file_slot->variable_count, &file_slot->hash_info)) {
        return 0;
    }
    
    /* Check if hash table needs to be built/rebuilt */
    if (file_slot->variable_count > 0 && 
        (file_slot->hash_info.size == 0 || file_slot->hash_info.entries == NULL)) {
        /* File was loaded without hash table (emergency save format) */
        /* Build hash table now */
        uint32_t hash_table_size = kh_calculate_optimal_hash_size(file_slot->variable_count);
        
        file_slot->hash_info.entries = (kh_generic_hash_entry_t*)calloc(hash_table_size, 
                                                                        sizeof(kh_generic_hash_entry_t));
        if (!file_slot->hash_info.entries) {
            kh_free_khdata_file(file_slot);
            return 0;
        }
        
        file_slot->hash_info.size = hash_table_size;
        file_slot->hash_info.used_count = 0;
        file_slot->hash_info.deleted_count = 0;
        file_slot->hash_info.needs_rebuild = 0;
        
        /* Insert all variables into hash table */
        for (int i = 0; i < file_slot->variable_count; i++) {
            if (file_slot->variables[i].name) {
                uint32_t name_hash = kh_hash_name_case_insensitive(file_slot->variables[i].name);
                if (!kh_generic_hash_insert(file_slot->hash_info.entries, hash_table_size, name_hash, i)) {
                    /* Hash insertion failed - shouldn't happen with proper sizing */
                    kh_free_khdata_file(file_slot);
                    return 0;
                }
                file_slot->hash_info.used_count++;
            }
        }
        
        /* Mark file as dirty to ensure it gets saved with proper hash table next time */
        file_slot->dirty = 1;
    }
    
    /* Store filename and path */
    file_slot->filename = (char*)malloc(strlen(filename) + 1);
    file_slot->full_path = (char*)malloc(strlen(full_path) + 1);
    
    if (!file_slot->filename || !file_slot->full_path) {
        kh_free_khdata_file(file_slot);
        return 0;
    }
    
    strcpy_s(file_slot->filename, strlen(filename) + 1, filename);
    strcpy_s(file_slot->full_path, strlen(full_path) + 1, full_path);
    
    file_slot->capacity = file_slot->variable_count;
    
    /* Dirty flag is already set if hash table was rebuilt */
    
    return 1;
}

/* Save a file from memory to disk */
static int kh_save_file_from_memory(khdata_file_t* file) {
    if (!file || !file->filename || !file->full_path) {
        return 1; /* Success if nothing to save */
    }
    
    /* Always save if marked dirty, but also check if file still needs saving */
    if (!file->dirty) {
        return 1; /* Nothing to save */
    }
    
    /* Ensure directory exists before saving (in case it was deleted) */
    char dir_path[MAX_FILE_PATH_LENGTH];
    if (!kh_get_arma3_documents_path(dir_path, sizeof(dir_path))) {
        return 0; /* Failed to get directory path */
    }
    
    /* Create directory if it doesn't exist */
    CreateDirectoryA(dir_path, NULL);
    
    if (!kh_write_binary_file(file->full_path, file->variables, file->variable_count)) {
        return 0;
    }
    
    file->dirty = 0;
    return 1;
}

/* Add new file to memory */
static khdata_file_t* kh_add_file_to_memory(const char* filename) {
    if (!filename) return NULL;
    
    char full_path[MAX_FILE_PATH_LENGTH];
    khdata_file_t* file_slot = NULL;
    
    /* Get full path */
    if (!kh_get_khdata_file_path(filename, full_path, sizeof(full_path))) {
        return NULL;
    }
    
    /* Check if already exists */
    file_slot = kh_find_file_in_memory(filename);
    if (file_slot) {
        return file_slot;
    }
    
    /* Find empty slot or resize array */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (!g_memory_manager.files[i].filename) {
            file_slot = &g_memory_manager.files[i];
            break;
        }
    }
    
    if (!file_slot) {
        /* Need to resize array */
        if (g_memory_manager.file_count >= g_memory_manager.file_capacity) {
            int new_capacity = g_memory_manager.file_capacity * 2;
            if (new_capacity < 16) new_capacity = 16;
            
            khdata_file_t* new_files = (khdata_file_t*)realloc(g_memory_manager.files, 
                                                               new_capacity * sizeof(khdata_file_t));
            if (!new_files) return NULL;
            
            /* Initialize new slots */
            memset(&new_files[g_memory_manager.file_capacity], 0, 
                   (new_capacity - g_memory_manager.file_capacity) * sizeof(khdata_file_t));
            
            g_memory_manager.files = new_files;
            g_memory_manager.file_capacity = new_capacity;
        }
        
        file_slot = &g_memory_manager.files[g_memory_manager.file_count++];
    }
    
    /* Initialize new file */
    memset(file_slot, 0, sizeof(khdata_file_t));
    
    file_slot->filename = (char*)malloc(strlen(filename) + 1);
    file_slot->full_path = (char*)malloc(strlen(full_path) + 1);
    
    if (!file_slot->filename || !file_slot->full_path) {
        kh_free_khdata_file(file_slot);
        return NULL;
    }
    
    strcpy_s(file_slot->filename, strlen(filename) + 1, filename);
    strcpy_s(file_slot->full_path, strlen(full_path) + 1, full_path);
    
    file_slot->capacity = 16; /* Initial capacity */
    file_slot->variables = (kh_variable_t*)calloc(file_slot->capacity, sizeof(kh_variable_t));
    if (!file_slot->variables) {
        kh_free_khdata_file(file_slot);
        return NULL;
    }
    
    file_slot->dirty = 1;
    
    return file_slot;
}

/* Remove file from memory */
static int kh_remove_file_from_memory(const char* filename) {
    if (!filename) return 0;
    
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        if (g_memory_manager.files[i].filename && 
            kh_strcasecmp(g_memory_manager.files[i].filename, filename) == 0) {
            
            /* Save if dirty before removing */
            if (g_memory_manager.files[i].dirty) {
                kh_save_file_from_memory(&g_memory_manager.files[i]);
            }
            
            kh_free_khdata_file(&g_memory_manager.files[i]);
            return 1;
        }
    }
    
    return 0;
}

/* Initialize memory manager and load all existing files */
static int kh_init_memory_manager(void) {
    /* Simple check for single-threaded environment */
    if (g_memory_manager.initialized) {
        return 1;
    }
    
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_pattern[MAX_FILE_PATH_LENGTH];
    
    /* Get base path */
    if (!kh_get_arma3_documents_path(g_memory_manager.base_path, sizeof(g_memory_manager.base_path))) {
        return 0;
    }
    
    /* Initialize memory manager */
    g_memory_manager.file_capacity = 16;
    g_memory_manager.files = (khdata_file_t*)calloc(g_memory_manager.file_capacity, sizeof(khdata_file_t));
    if (!g_memory_manager.files) {
        return 0;
    }
    
    g_memory_manager.file_count = 0;
    g_memory_manager.initialized = 1;
    
    /* Find all .khdata files */
    _snprintf_s(search_pattern, sizeof(search_pattern), _TRUNCATE, "%s\\*.khdata", g_memory_manager.base_path);
    
    find_handle = FindFirstFileA(search_pattern, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        return 1; /* No files found, but initialization successful */
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            /* Load this file into memory */
            char* filename = find_data.cFileName;
            
            /* Remove .khdata extension for internal storage */
            int filename_len = (int)strlen(filename);
            if (filename_len > 7 && strcmp(filename + filename_len - 7, ".khdata") == 0) {
                filename[filename_len - 7] = '\0';
            }
            
            kh_load_file_into_memory(filename);
            
            /* Restore extension */
            if (filename_len > 7) {
                filename[filename_len - 7] = '.';
            }
        }
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return 1;
}

/* Cleanup memory manager and save all dirty files */
static void kh_cleanup_memory_manager(void) {
    if (!g_memory_manager.initialized) {
        return;
    }
    
    /* Check if we're in a normal shutdown scenario */
    /* If GetModuleHandle returns NULL for our own module, we're likely in process termination */
    HMODULE hSelf = NULL;
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCTSTR)kh_cleanup_memory_manager, &hSelf) || !hSelf) {
        /* Process is likely terminating - skip file I/O */
        memset(&g_memory_manager, 0, sizeof(g_memory_manager));
        return;
    }
    
    /* First pass: attempt quick saves for dirty files */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        khdata_file_t* file = &g_memory_manager.files[i];
        
        if (file && file->filename && file->dirty && file->full_path && 
            file->variables && file->variable_count > 0) {
            
            /* Use low-level file operations for reliability during shutdown */
            HANDLE hFile = CreateFileA(file->full_path, 
                                      GENERIC_WRITE, 
                                      0, 
                                      NULL, 
                                      CREATE_ALWAYS, 
                                      FILE_ATTRIBUTE_NORMAL, 
                                      NULL);
            
            if (hFile != INVALID_HANDLE_VALUE) {
                /* Write minimal header */
                kh_file_header_t header = {0};
                header.magic = KHDATA_MAGIC;
                header.variable_count = file->variable_count;
                header.hash_table_size = 0; /* Skip hash table for emergency save */
                header.data_section_offset = sizeof(kh_file_header_t);
                
                DWORD written;
                WriteFile(hFile, &header, sizeof(header), &written, NULL);
                
                /* Write variables in simplified format */
                for (int j = 0; j < file->variable_count && file->variables[j].name; j++) {
                    kh_variable_t* var = &file->variables[j];
                    
                    unsigned int name_len = (unsigned int)strlen(var->name);
                    const char* type_str = kh_get_string_from_type(var->type);
                    unsigned int type_len = (unsigned int)strlen(type_str);
                    unsigned int value_len = var->value ? (unsigned int)strlen(var->value) : 0;
                    
                    /* Write with minimal error checking */
                    WriteFile(hFile, &name_len, sizeof(unsigned int), &written, NULL);
                    WriteFile(hFile, var->name, name_len, &written, NULL);
                    WriteFile(hFile, &type_len, sizeof(unsigned int), &written, NULL);
                    WriteFile(hFile, type_str, type_len, &written, NULL);
                    WriteFile(hFile, &value_len, sizeof(unsigned int), &written, NULL);
                    if (value_len > 0 && var->value) {
                        WriteFile(hFile, var->value, value_len, &written, NULL);
                    }
                }
                
                CloseHandle(hFile);
            }
        }
    }
    
    /* Now free all memory - this is always safe */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        khdata_file_t* file = &g_memory_manager.files[i];
        if (file) {
            /* Free all allocated memory */
            free(file->filename);
            free(file->full_path);
            
            if (file->variables) {
                for (int j = 0; j < file->variable_count; j++) {
                    free(file->variables[j].name);
                    free(file->variables[j].value);
                }
                free(file->variables);
            }
            
            kh_free_hash_table_info(&file->hash_info);
        }
    }
    
    /* Free the files array itself */
    free(g_memory_manager.files);
    
    /* Clear the structure */
    memset(&g_memory_manager, 0, sizeof(g_memory_manager));
}

/* Find variable with hash table lookup - using Robin Hood hashing */
static inline int kh_find_variable_indexed(kh_variable_t* variables, int count, const char* name,
                                          kh_generic_hash_entry_t* hash_table, uint32_t hash_table_size) {
    if (!variables || !name || count <= 0 || !hash_table || hash_table_size == 0) {
        return -1;
    }
    
    uint32_t name_hash = kh_hash_name_case_insensitive(name);
    int found = 0;
    uint32_t index = kh_generic_hash_find(hash_table, hash_table_size, name_hash, &found);
    
    if (found && index < hash_table_size) {  /* Additional bounds check */
        /* Hash found, verify the actual name matches */
        uint32_t var_index = hash_table[index].data_index;
        if (var_index < (uint32_t)count && variables[var_index].name &&
            kh_strcasecmp(variables[var_index].name, name) == 0) {
            return (int)var_index;
        }
    }
    
    return -1; /* Not found */
}

/* Format variable for output as ["TYPE", value] */
static int kh_format_variable_output(kh_type_t type, const char* value, char* output, int output_size) {
    if (!value || !output || output_size <= 0) return 0;
    
    const char* type_str = kh_get_string_from_type(type);
    
    /* Determine if value should be quoted based on type */
    int needs_quotes = 1;
    
    switch (type) {
        case KH_TYPE_BOOL:
        case KH_TYPE_SCALAR:
        case KH_TYPE_ARRAY:
        case KH_TYPE_HASHMAP:
        case KH_TYPE_TYPED_ARRAY:
        case KH_TYPE_TYPED_HASHMAP:
        case KH_TYPE_NIL:
            needs_quotes = 0; /* These types are not quoted in output */
            break;
        default:
            needs_quotes = 1; /* All string-based types are quoted */
            break;
    }
    
    if (needs_quotes) {
        return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", \"%s\"]", type_str, value) >= 0);
    } else {
        return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, value) >= 0);
    }
}

/* Convert binary format file to text format */
static int kh_convert_binary_to_text(const char* file_path) {
    if (!file_path) return 0;
    
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    kh_generic_hash_table_t hash_info = {0};
    char* temp_file_path = NULL;
    int result = 0;
    
    /* Read binary format */
    if (!kh_read_binary_file_with_hash(file_path, &variables, &variable_count, &hash_info)) {
        return 0;
    }
    
    /* Create temporary file path */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10); /* +10 for ".tmp" + null */
    if (!temp_file_path) {
        goto cleanup;
    }
    
    strcpy_s(temp_file_path, path_len + 10, file_path);
    strcat_s(temp_file_path, path_len + 10, ".tmp");
    
    /* Write text format to temporary file */
    if (!kh_write_text_file(temp_file_path, variables, variable_count)) {
        goto cleanup;
    }
    
    /* Replace original file with temporary file */
    if (DeleteFileA(file_path) && MoveFileA(temp_file_path, file_path)) {
        result = 1;
    } else {
        DeleteFileA(temp_file_path); /* Clean up temp file if move failed */
    }

cleanup:
    kh_free_variables(variables, variable_count);
    kh_free_hash_table_info(&hash_info);
    free(temp_file_path);
    return result;
}

/* Convert text format file to binary format */
static int kh_convert_text_to_binary(const char* file_path) {
    if (!file_path) return 0;
    
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    char* temp_file_path = NULL;
    int result = 0;
    
    /* Read text format */
    if (!kh_read_text_file(file_path, &variables, &variable_count)) {
        return 0;
    }
    
    /* Create temporary file path */
    size_t path_len = strlen(file_path);
    temp_file_path = (char*)malloc(path_len + 10); /* +10 for ".tmp" + null */
    if (!temp_file_path) {
        goto cleanup;
    }
    
    strcpy_s(temp_file_path, path_len + 10, file_path);
    strcat_s(temp_file_path, path_len + 10, ".tmp");
    
    /* Write binary format to temporary file */
    if (!kh_write_binary_file(temp_file_path, variables, variable_count)) {
        goto cleanup;
    }
    
    /* Replace original file with temporary file */
    if (DeleteFileA(file_path) && MoveFileA(temp_file_path, file_path)) {
        result = 1;
    } else {
        DeleteFileA(temp_file_path); /* Clean up temp file if move failed */
    }

cleanup:
    kh_free_variables(variables, variable_count);
    free(temp_file_path);
    return result;
}

/* Main UnbinarizeKHData function - convert binary to text format */
static int kh_unbinarize_khdata(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Check if file exists */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Check if already in text format */
    if (!kh_is_file_binary(file_path)) {
        strcpy_s(output, (size_t)output_size, "ALREADY TEXT FORMAT");
        result = 0;
        goto cleanup;
    }

    /* Convert binary to text */
    if (!kh_convert_binary_to_text(file_path)) {
        kh_set_error(output, output_size, "CONVERSION FAILED");
        goto cleanup;
    }
    
    /* Remove from memory if loaded */
    kh_remove_file_from_memory(clean_filename);
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    return result;
}

/* Main BinarizeKHData function - convert text to binary format */
static int kh_binarize_khdata(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Check if file exists */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Check if already in binary format */
    if (kh_is_file_binary(file_path)) {
        strcpy_s(output, (size_t)output_size, "ALREADY BINARY FORMAT");
        result = 0;
        goto cleanup;
    }
    
    /* Convert text to binary */
    if (!kh_convert_text_to_binary(file_path)) {
        kh_set_error(output, output_size, "CONVERSION FAILED");
        goto cleanup;
    }
    
    /* Remove from memory if loaded (will be reloaded as binary format next time) */
    kh_remove_file_from_memory(clean_filename);
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    return result;
}

/* Read from memory instead of disk */
static int kh_read_khdata_variable(const char* filename, const char* variable_name, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    khdata_file_t* file = NULL;
    int var_index;
    char* formatted_output = NULL;
    int result = 1;
    
    /* Allocate memory for cleaned strings */
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_var_name || !clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        kh_set_error(output, output_size, "FILE NOT FOUND OR LOAD FAILED");
        goto cleanup;
    }
    
    /* Find file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        kh_set_error(output, output_size, "FILE NOT FOUND IN MEMORY");
        goto cleanup;
    }
    
    /* Special case: return all variable names if var_name == filename */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        size_t result_size = (size_t)file->variable_count * 256 + 100;
        char* result_str = (char*)malloc(result_size);
        if (!result_str) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        
        strcpy_s(result_str, result_size, "[");
        for (int i = 0; i < file->variable_count; i++) {
            if (i > 0) strcat_s(result_str, result_size, ", ");
            strcat_s(result_str, result_size, "\"");
            strcat_s(result_str, result_size, file->variables[i].name);
            strcat_s(result_str, result_size, "\"");
        }
        strcat_s(result_str, result_size, "]");
        strcpy_s(output, (size_t)output_size, result_str);
        result = 0;
        free(result_str);
        goto cleanup;
    }
    
    /* Find specific variable */
    var_index = kh_find_variable_indexed(file->variables, file->variable_count, clean_var_name, file->hash_info.entries, file->hash_info.size);
    if (var_index == -1) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        goto cleanup;
    }
    
    /* Format output */
    size_t formatted_len = strlen(file->variables[var_index].value) + 50;
    formatted_output = (char*)malloc(formatted_len);
    if (!formatted_output) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    if (!kh_format_variable_output(file->variables[var_index].type, file->variables[var_index].value, formatted_output, (int)formatted_len)) {
        kh_set_error(output, output_size, "OUTPUT FORMATTING FAILED");
        goto cleanup;
    }

    strcpy_s(output, (size_t)output_size, formatted_output);
    result = 0;

cleanup:
    free(clean_var_name);
    free(clean_filename);
    free(formatted_output);
    return result;
}

/* Delete entire KHData file from memory and disk */
static int kh_delete_khdata_file(const char* filename, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
    char* clean_filename = NULL;
    int result = 1;
    
    clean_filename = (char*)malloc(strlen(filename) + 1);
    if (!clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH");
        goto cleanup;
    }
    
    /* Remove from memory first */
    kh_remove_file_from_memory(clean_filename);
    
    /* Check if file exists on disk */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        kh_set_error(output, output_size, "FILE NOT FOUND");
        goto cleanup;
    }
    
    /* Attempt to delete the file from disk */
    if (DeleteFileA(file_path)) {
        strcpy_s(output, (size_t)output_size, "SUCCESS");
        result = 0;
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_ACCESS_DENIED:
                kh_set_error(output, output_size, "ACCESS DENIED - FILE MAY BE IN USE");
                break;
            case ERROR_SHARING_VIOLATION:
                kh_set_error(output, output_size, "FILE IS IN USE BY ANOTHER PROCESS");
                break;
            default:
                kh_set_error(output, output_size, "FAILED TO DELETE FILE");
                break;
        }
    }

cleanup:
    free(clean_filename);
    return result;
}

/* Delete specific variable from KHData file in memory and sync to disk */
static int kh_delete_khdata_variable(const char* filename, const char* variable_name, 
                                             char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char* clean_filename = NULL;
    char* clean_var_name = NULL;
    char file_path[MAX_FILE_PATH_LENGTH];
    khdata_file_t* file = NULL;
    int var_index;
    int result = 1;
    
    /* Allocate memory for cleaned strings */
    clean_filename = (char*)malloc(strlen(filename) + 1);
    clean_var_name = (char*)malloc(strlen(variable_name) + 1);
    
    if (!clean_filename || !clean_var_name) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    
    if (!kh_get_khdata_file_path(clean_filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID FILE PATH");
        goto cleanup;
    }
    
    /* Load file into memory */
    if (!kh_load_file_into_memory(clean_filename)) {
        kh_set_error(output, output_size, "FILE NOT FOUND OR LOAD FAILED");
        goto cleanup;
    }
    
    file = kh_find_file_in_memory(clean_filename);
    if (!file || file->variable_count == 0) {
        kh_set_error(output, output_size, "FILE IS EMPTY");
        goto cleanup;
    }
    
    /* Find variable to delete */
    var_index = kh_find_variable_indexed(file->variables, file->variable_count, clean_var_name, file->hash_info.entries, file->hash_info.size);
    if (var_index == -1) {
        kh_set_error(output, output_size, "VARIABLE NOT FOUND");
        goto cleanup;
    }
    
    /* If this is the only variable, delete the entire file */
    if (file->variable_count == 1) {
        kh_remove_file_from_memory(clean_filename);
        if (DeleteFileA(file_path)) {
            strcpy_s(output, (size_t)output_size, "SUCCESS");
            result = 0;
        } else {
            kh_set_error(output, output_size, "FAILED TO DELETE FILE");
        }
        goto cleanup;
    }
    
    /* Free the variable's memory */
    free(file->variables[var_index].name);
    free(file->variables[var_index].value);
    
    /* Shift remaining variables */
    for (int i = var_index; i < file->variable_count - 1; i++) {
        file->variables[i] = file->variables[i + 1];
    }
    
    memset(&file->variables[file->variable_count - 1], 0, sizeof(kh_variable_t));
    file->variable_count--;
    
    /* Force hash table rebuild - safer than trying to update indices */
    file->hash_info.needs_rebuild = 1;
    
    /* Rebuild hash table immediately for consistency */
    if (!kh_rebuild_hash_table(file)) {
        kh_set_error(output, output_size, "FAILED TO REBUILD HASH TABLE");
        goto cleanup;
    }
    
    file->dirty = 1;
    if (!kh_save_file_from_memory(file)) {
        kh_set_error(output, output_size, "FAILED TO SAVE FILE");
        goto cleanup;
    }
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    free(clean_var_name);
    return result;
}

/* Calculate the formatted size of a variable's output from memory */
static long kh_get_variable_formatted_size(const char* filename, const char* variable_name) {
    if (!filename || !variable_name) return -1;
    
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    khdata_file_t* file = NULL;
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
    
    kh_clean_string(variable_name, clean_var_name, (int)strlen(variable_name) + 1);
    kh_clean_string(filename, clean_filename, (int)strlen(filename) + 1);
    
    /* Load file into memory if not already loaded */
    if (!kh_load_file_into_memory(clean_filename)) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Find file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Special case: variable name equals filename (return size of all variable names array) */
    if (kh_strcasecmp(clean_var_name, clean_filename) == 0) {
        total_size = 2; /* Opening and closing brackets */
        
        for (int i = 0; i < file->variable_count; i++) {
            if (i > 0) {
                total_size += 2; /* ", " separator */
            }
            if (file->variables[i].name) {
                total_size += (long)strlen(file->variables[i].name) + 2; /* Name with quotes */
            }
        }
        
        free(clean_var_name);
        free(clean_filename);
        return total_size;
    }
    
    /* Find specific variable */
    var_index = kh_find_variable_indexed(file->variables, file->variable_count, clean_var_name, file->hash_info.entries, file->hash_info.size);
    if (var_index == -1) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
    /* Calculate size of formatted output ["TYPE", value] */
    const char* type_str = kh_get_string_from_type(file->variables[var_index].type);
    if (type_str && file->variables[var_index].value) {
        total_size = (long)strlen(type_str) + (long)strlen(file->variables[var_index].value) + 10; /* Extra for formatting */
    }
    
    free(clean_var_name);
    free(clean_filename);
    return total_size;
}

/* Function to flush dirty files */
static int kh_flush_khdata(char* output, int output_size) {
    if (!output || output_size <= 0) return 1;
    
    output[0] = '\0';
    int files_flushed = 0;
    int files_failed = 0;
    
    /* Save all dirty files */
    for (int i = 0; i < g_memory_manager.file_count; i++) {
        khdata_file_t* file = &g_memory_manager.files[i];
        if (file && file->dirty && file->filename) {
            if (kh_save_file_from_memory(file)) {
                files_flushed++;
            } else {
                files_failed++;
            }
        }
    }
    
    if (files_failed > 0) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, 
                   "PARTIAL SUCCESS: %d FLUSHED, %d FAILED", files_flushed, files_failed);
        return 1;
    }
    
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "SUCCESS: %d FILES FLUSHED", files_flushed);
    return 0;
}

/* Main write function - expects [type, value] array */
static int kh_write_khdata_variable(const char* filename, const char* variable_name, 
                                   const char* type_value_array, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size) ||
        !kh_validate_non_empty_string(type_value_array, "TYPE-VALUE ARRAY", output, output_size)) {
        return 1;
    }
    
    char* clean_var_name = NULL;
    char* clean_filename = NULL;
    char* type_str = NULL;
    char* value_str = NULL;
    char* final_value = NULL;
    char normalized_value[256];
    kh_type_t data_type;
    khdata_file_t* file = NULL;
    int var_index;
    int result = 1;
    
    /* Cache string lengths */
    size_t filename_len = strlen(filename);
    size_t var_name_len = strlen(variable_name);
    
    /* Parse the [type, value] array */
    if (!kh_parse_type_value_array(type_value_array, &type_str, &value_str)) {
        kh_set_error(output, output_size, "INVALID TYPE-VALUE ARRAY FORMAT");
        return 1;
    }
    
    /* Get type enum from string */
    data_type = kh_get_type_from_string(type_str);
    if (data_type == KH_TYPE_UNKNOWN) {
        kh_set_error(output, output_size, "UNKNOWN TYPE");
        goto cleanup;
    }
    
    /* Destringify value if necessary */
    if (!kh_destringify_value(data_type, value_str, &final_value)) {
        kh_set_error(output, output_size, "VALUE DESTRINGIFICATION FAILED");
        goto cleanup;
    }
    
    /* Normalize special types */
    if (data_type == KH_TYPE_NAMESPACE) {
        if (kh_normalize_namespace_value(final_value, normalized_value, sizeof(normalized_value))) {
            free(final_value);
            final_value = (char*)malloc(strlen(normalized_value) + 1);
            if (final_value) {
                strcpy(final_value, normalized_value);
            }
        }
    } else if (data_type == KH_TYPE_SIDE) {
        if (kh_normalize_side_value(final_value, normalized_value, sizeof(normalized_value))) {
            free(final_value);
            final_value = (char*)malloc(strlen(normalized_value) + 1);
            if (final_value) {
                strcpy(final_value, normalized_value);
            }
        }
    }
    
    /* Validate value format based on type */
    if (!kh_validate_value_format(data_type, final_value)) {
        kh_set_error(output, output_size, "INVALID VALUE FORMAT FOR TYPE");
        goto cleanup;
    }
    
    /* Additional validation for complex types */
    if (data_type == KH_TYPE_TYPED_ARRAY && !kh_validate_typed_array(final_value)) {
        kh_set_error(output, output_size, "INVALID TYPED_ARRAY FORMAT");
        goto cleanup;
    }
    
    if (data_type == KH_TYPE_TYPED_HASHMAP && !kh_validate_typed_hashmap(final_value)) {
        kh_set_error(output, output_size, "INVALID TYPED_HASHMAP FORMAT");
        goto cleanup;
    }
    
    /* Check file limit before proceeding */
    if (!kh_check_file_limit(filename, output, output_size)) {
        goto cleanup;
    }
    
    /* Clean input strings using cached lengths */
    clean_var_name = (char*)malloc(var_name_len + 1);
    clean_filename = (char*)malloc(filename_len + 1);
    
    if (!clean_var_name || !clean_filename) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)var_name_len + 1);
    kh_clean_string(filename, clean_filename, (int)filename_len + 1);
    
    /* Validate inputs */
    size_t clean_var_len = strlen(clean_var_name);
    size_t final_value_len = strlen(final_value);
    
    if (!kh_validate_utf8_string(clean_var_name, (int)clean_var_len) ||
        !kh_validate_utf8_string(final_value, (int)final_value_len) ||
        !kh_validate_variable_name(clean_var_name)) {
        kh_set_error(output, output_size, "INVALID INPUT DATA");
        goto cleanup;
    }
    
    /* Load or create file in memory */
    file = kh_find_file_in_memory(clean_filename);
    if (!file) {
        if (!kh_load_file_into_memory(clean_filename)) {
            file = kh_add_file_to_memory(clean_filename);
            if (!file) {
                kh_set_error(output, output_size, "FAILED TO CREATE FILE IN MEMORY");
                goto cleanup;
            }
        } else {
            file = kh_find_file_in_memory(clean_filename);
        }
    }
    
    if (!file) {
        kh_set_error(output, output_size, "FILE NOT FOUND IN MEMORY");
        goto cleanup;
    }
    
    /* Find or add variable */
    var_index = kh_find_variable_indexed(file->variables, file->variable_count, clean_var_name, file->hash_info.entries, file->hash_info.size);
    
    if (var_index == -1) {
        /* Adding new variable - check if we need to expand array first */
        if (file->variable_count >= file->capacity) {
            int new_capacity = file->capacity * 2;
            if (new_capacity < 16) new_capacity = 16;
            
            kh_variable_t* new_variables = (kh_variable_t*)realloc(file->variables, 
                                                                  new_capacity * sizeof(kh_variable_t));
            if (!new_variables) {
                kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
                goto cleanup;
            }
            
            memset(&new_variables[file->capacity], 0, 
                   (new_capacity - file->capacity) * sizeof(kh_variable_t));
            
            file->variables = new_variables;
            file->capacity = new_capacity;
        }
        
        /* Check if hash table needs rebuild BEFORE adding */
        if (kh_generic_hash_needs_rebuild(&file->hash_info, file->variable_count + 1)) {
            if (!kh_rebuild_hash_table(file)) {
                kh_set_error(output, output_size, "FAILED TO REBUILD HASH TABLE");
                goto cleanup;
            }
        }
        
        /* Now add the new variable */
        var_index = file->variable_count;
        
        file->variables[var_index].name = (char*)malloc(clean_var_len + 1);
        if (!file->variables[var_index].name) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(file->variables[var_index].name, clean_var_len + 1, clean_var_name);
        
        /* Try to add to hash table */
        if (!kh_hash_table_add_entry(&file->hash_info, clean_var_name, var_index)) {
            /* Hash table add failed, remove the name we just added and fail */
            free(file->variables[var_index].name);
            file->variables[var_index].name = NULL;
            kh_set_error(output, output_size, "FAILED TO ADD TO HASH TABLE");
            goto cleanup;
        }
        
        /* Only increment count after successful hash table update */
        file->variable_count++;
    } else {
        /* Update existing variable - free old value */
        free(file->variables[var_index].value);
    }
    
    /* Set new value and type using cached length */
    file->variables[var_index].value = (char*)malloc(final_value_len + 1);
    if (!file->variables[var_index].value) {
        /* If this was a new variable, we need to clean up */
        if (var_index == file->variable_count - 1) {
            free(file->variables[var_index].name);
            file->variables[var_index].name = NULL;
            file->variable_count--;
            kh_hash_table_remove_entry(&file->hash_info, clean_var_name);
        }
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    strcpy_s(file->variables[var_index].value, final_value_len + 1, final_value);
    file->variables[var_index].type = data_type;
    
    /* Mark file as dirty */
    file->dirty = 1;
    
    /* Check file size and only auto-save if under 1MB */
    long file_size = kh_get_file_size(file->full_path);
    if (file_size < 0) {
        /* File doesn't exist yet, estimate size from memory */
        file_size = 0;
        for (int i = 0; i < file->variable_count; i++) {
            if (file->variables[i].name) file_size += strlen(file->variables[i].name);
            if (file->variables[i].value) file_size += strlen(file->variables[i].value);
            file_size += 50; /* Overhead for type info and structure */
        }
    }
    
    /* Auto-save only if under threshold */
    if (file_size <= MAX_KHDATA_FILE_AUTOSAVE_THRESHOLD) {
        if (!kh_save_file_from_memory(file)) {
            kh_set_error(output, output_size, "FAILED TO SAVE FILE");
            goto cleanup;
        }
    }
    /* If over threshold, file remains dirty and requires manual flush */
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_var_name);
    free(clean_filename);
    free(type_str);
    free(value_str);
    free(final_value);
    return result;
}

#endif /* PROCESS_KH_DATA_H */