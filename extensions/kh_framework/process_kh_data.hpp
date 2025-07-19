#ifndef PROCESS_KH_DATA_HPP
#define PROCESS_KH_DATA_HPP

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

/* Hash table entry for indexed access - properly aligned */
#pragma pack(push, 4)
typedef struct {
    uint32_t name_hash;      /* Hash of variable name */
    uint32_t data_offset;    /* Offset to variable data in file */
    uint32_t reserved;       /* Reserved for future use/alignment */
} kh_hash_entry_t;
#pragma pack(pop)

/* Binary file header structure - enhanced for indexed format with alignment */
#pragma pack(push, 4)
typedef struct {
    unsigned int magic;              /* Magic number */
    unsigned int version;            /* Format version */
    unsigned int variable_count;     /* Number of variables */
    unsigned int hash_table_size;    /* Size of hash table (0 for old format) */
    unsigned int data_section_offset; /* Offset to data section */
    unsigned int reserved1;          /* Reserved for alignment */
    unsigned int reserved2;          /* Reserved for future use */
    unsigned int reserved3;          /* Reserved for future use */
} kh_file_header_t;
#pragma pack(pop)

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

/* Hash function for variable names - case insensitive */
static inline uint32_t kh_hash_variable_name(const char* name) {
    if (!name) return KH_HASH_EMPTY;
    
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
    return (hash == KH_HASH_EMPTY) ? 1 : hash;
}

/* Calculate optimal hash table size */
static inline uint32_t kh_calculate_hash_table_size(uint32_t variable_count) {
    /* Use next power of 2 that gives load factor <= 0.75 */
    uint32_t min_size = (uint32_t)((double)variable_count / KH_HASH_TABLE_LOAD_FACTOR);
    uint32_t size = KH_HASH_TABLE_MIN_SIZE;
    
    while (size < min_size) {
        size <<= 1;
    }
    
    return size;
}

/* Hash table lookup with linear probing - consistent collision resolution */
static inline uint32_t kh_hash_table_find(kh_hash_entry_t* hash_table, uint32_t hash_table_size, 
                                          uint32_t name_hash, int* found) {
    if (!hash_table || hash_table_size == 0 || name_hash == KH_HASH_EMPTY) {
        if (found) *found = 0;
        return 0;
    }
    
    uint32_t index = name_hash % hash_table_size;
    uint32_t original_index = index;
    
    do {
        if (hash_table[index].name_hash == KH_HASH_EMPTY) {
            /* Empty slot found - not in table */
            if (found) *found = 0;
            return index; /* Return empty slot for insertion */
        }
        
        if (hash_table[index].name_hash == name_hash) {
            /* Found matching hash */
            if (found) *found = 1;
            return index;
        }
        
        /* Linear probe to next slot */
        index = (index + 1) % hash_table_size;
        
    } while (index != original_index);
    
    /* Table is full */
    if (found) *found = 0;
    return 0;
}

/* Hash table insertion with linear probing */
static inline int kh_hash_table_insert(kh_hash_entry_t* hash_table, uint32_t hash_table_size,
                                       uint32_t name_hash, uint32_t data_offset) {
    if (!hash_table || hash_table_size == 0 || name_hash == KH_HASH_EMPTY) return 0;
    
    int found = 0;
    uint32_t index = kh_hash_table_find(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        /* Update existing entry */
        hash_table[index].data_offset = data_offset;
        return 1;
    }
    
    /* Insert new entry if slot is available */
    if (hash_table[index].name_hash == KH_HASH_EMPTY) {
        hash_table[index].name_hash = name_hash;
        hash_table[index].data_offset = data_offset;
        hash_table[index].reserved = 0;
        return 1;
    }
    
    return 0; /* Table full */
}

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

/* Check if file is in binary format and get version */
static int kh_get_file_format_version(const char* file_path, uint32_t* version) {
    if (!file_path || !version) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int result = 0;
    
    if (fopen_s(&file, file_path, "rb") != 0) {
        return 0; /* File doesn't exist or can't be opened */
    }
    
    /* Try to read header */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) == 1) {
        /* Check magic number */
        if (header.magic == KHDATA_MAGIC && header.version >= 1) {
            *version = header.version;
            result = 1;
        }
    }
    
    fclose(file);
    return result;
}

/* Check if file is already in binary format */
static int kh_is_file_binary(const char* file_path) {
    uint32_t version;
    return kh_get_file_format_version(file_path, &version);
}

/* Parse a single line from text format: variable_name=TYPE:value */
static int kh_parse_text_line(const char* line, char** name, kh_data_type_t* type, char** value) {
    if (!line || !name || !type || !value) return 0;
    
    *name = NULL;
    *value = NULL;
    *type = KH_TYPE_UNKNOWN;
    
    /* Skip empty lines and comments */
    const char* ptr = line;
    while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
    if (*ptr == '\0' || *ptr == '#' || *ptr == '\n' || *ptr == '\r') {
        return 0; /* Empty line or comment */
    }
    
    /* Find the equals sign */
    const char* equals_pos = strchr(ptr, '=');
    if (!equals_pos) return 0;
    
    /* Find the colon that separates type from value */
    const char* colon_pos = strchr(equals_pos + 1, ':');
    if (!colon_pos) return 0;
    
    /* Extract variable name */
    int name_len = (int)(equals_pos - ptr);
    while (name_len > 0 && (ptr[name_len-1] == ' ' || ptr[name_len-1] == '\t')) {
        name_len--; /* Trim trailing whitespace */
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
        type_len--; /* Trim trailing whitespace */
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
    
    /* Extract value */
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
    
    FILE* file;
    char* line_buffer = NULL;
    int buffer_size = 1024;
    kh_variable_t* temp_variables = NULL;
    int temp_count = 0;
    int temp_capacity = 16;
    int success = 1;
    
    *variables = NULL;
    *count = 0;
    
    if (fopen_s(&file, file_path, "r") != 0) {
        return 0;
    }
    
    /* Allocate initial arrays */
    line_buffer = (char*)malloc((size_t)buffer_size);
    temp_variables = (kh_variable_t*)calloc((size_t)temp_capacity, sizeof(kh_variable_t));
    
    if (!line_buffer || !temp_variables) {
        success = 0;
        goto cleanup;
    }
    
    /* Read line by line */
    while (fgets(line_buffer, buffer_size, file)) {
        /* Resize line buffer if needed */
        while (strlen(line_buffer) == (size_t)buffer_size - 1 && line_buffer[buffer_size - 2] != '\n') {
            buffer_size *= 2;
            char* new_buffer = (char*)realloc(line_buffer, (size_t)buffer_size);
            if (!new_buffer) {
                success = 0;
                goto cleanup;
            }
            line_buffer = new_buffer;
            
            /* Continue reading the rest of the line */
            if (!fgets(line_buffer + strlen(line_buffer), buffer_size - (int)strlen(line_buffer), file)) {
                break;
            }
        }
        
        char* name = NULL;
        kh_data_type_t type;
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
                    success = 0;
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

cleanup:
    fclose(file);
    free(line_buffer);
    
    if (!success && temp_variables) {
        /* Free partially allocated variables on failure */
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
    fprintf(file, "# Format: variable_name=TYPE:value\n");
    fprintf(file, "# Allowed Types:\n     # BOOL:true\n     # SCALAR:0\n     # ARRAY:[element1, element2, element3]\n     # HASHMAP:[[key, value]]\n     # STRING:string\n     # TEXT:text\n     # CODE:code\n\n");
    
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

/* Convert binary format file to text format */
static int kh_convert_binary_to_text(const char* file_path) {
    if (!file_path) return 0;
    
    kh_variable_t* variables = NULL;
    int variable_count = 0;
    char* temp_file_path = NULL;
    int result = 0;
    
    /* Read binary format */
    if (!kh_read_binary_file(file_path, &variables, &variable_count)) {
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
    free(temp_file_path);
    return result;
}

/* Ensure file is in binary format before reading/writing */
static int kh_ensure_binary_format(const char* file_path) {
    if (!file_path) return 0;
    
    /* Check if file exists */
    DWORD file_attributes = GetFileAttributesA(file_path);
    if (file_attributes == INVALID_FILE_ATTRIBUTES) {
        return 1; /* File doesn't exist, no conversion needed */
    }
    
    /* Check if already binary */
    if (kh_is_file_binary(file_path)) {
        return 1; /* Already binary */
    }
    
    /* Convert to binary */
    return kh_convert_text_to_binary(file_path);
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
        strcpy_s(output, (size_t)output_size, "ALREADY_TEXT_FORMAT");
        result = 0;
        goto cleanup;
    }
    
    /* Convert binary to text */
    if (!kh_convert_binary_to_text(file_path)) {
        kh_set_error(output, output_size, "CONVERSION_FAILED");
        goto cleanup;
    }
    
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
        strcpy_s(output, (size_t)output_size, "ALREADY_BINARY_FORMAT");
        result = 0;
        goto cleanup;
    }
    
    /* Convert text to binary */
    if (!kh_convert_text_to_binary(file_path)) {
        kh_set_error(output, output_size, "CONVERSION_FAILED");
        goto cleanup;
    }
    
    strcpy_s(output, (size_t)output_size, "SUCCESS");
    result = 0;

cleanup:
    free(clean_filename);
    return result;
}

/* Enhanced find variable with hash table lookup - consistent linear probing */
static inline int kh_find_variable_indexed(kh_variable_t* variables, int count, const char* name,
                                          kh_hash_entry_t* hash_table, uint32_t hash_table_size) {
    if (!variables || !name || count <= 0 || !hash_table || hash_table_size == 0) {
        return kh_find_variable(variables, count, name); /* Fallback to linear search */
    }
    
    uint32_t name_hash = kh_hash_variable_name(name);
    int found = 0;
    uint32_t index = kh_hash_table_find(hash_table, hash_table_size, name_hash, &found);
    
    if (found) {
        /* Hash found, verify the actual name matches */
        uint32_t var_index = hash_table[index].data_offset;
        if (var_index < (uint32_t)count && variables[var_index].name &&
            kh_strcasecmp(variables[var_index].name, name) == 0) {
            return (int)var_index;
        }
    }
    
    return -1; /* Not found */
}

/* Find variable by name (case-insensitive) - linear search fallback */
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

/* Validate file integrity before reading */
static int kh_validate_file_integrity(const char* file_path, long file_size) {
    if (!file_path || file_size < 0) return 0;
    
    /* File must be at least large enough for header */
    if (file_size < (long)sizeof(kh_file_header_t)) return 0;
    
    /* File shouldn't be larger than our maximum limit */
    if (file_size > MAX_TOTAL_KHDATA_SIZE_BYTES) return 0;
    
    FILE* file;
    kh_file_header_t header;
    int result = 1;
    
    if (fopen_s(&file, file_path, "rb") != 0) return 0;
    
    /* Read header for basic validation */
    if (fread(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        result = 0;
    } else {
        /* Validate magic number and version */
        if (header.magic != KHDATA_MAGIC || header.version < 1 || header.version > 2) {
            result = 0;
        }
        
        /* Check if variable count is reasonable */
        if (header.variable_count > 10000) {
            result = 0;
        }
        
        /* For version 2, validate hash table size */
        if (header.version >= 2) {
            if (header.hash_table_size > 0 && header.hash_table_size < KH_HASH_TABLE_MIN_SIZE) {
                result = 0;
            }
            if (header.data_section_offset >= (uint32_t)file_size) {
                result = 0;
            }
        }
    }
    
    fclose(file);
    return result;
}

/* Read binary file with hash table support - enhanced for indexed format */
int kh_read_binary_file(const char* file_path, kh_variable_t** variables, int* count) {
    if (!file_path || !variables || !count) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_variable_t* temp_variables = NULL;
    kh_hash_entry_t* hash_table = NULL;
    int success = 0;
    long file_size = 0;
    long bytes_read = 0;
    uint32_t format_version = 1;
    
    *variables = NULL;
    *count = 0;
    
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
    
    if (header.magic != KHDATA_MAGIC || header.version < 1 || header.version > 2) {
        fclose(file);
        return 0;
    }
    
    format_version = header.version;
    
    /* Validate variable count */
    if (header.variable_count > 10000 || 
        (header.variable_count > 0 && bytes_read >= file_size)) {
        fclose(file);
        return 0;
    }
    
    if (header.variable_count == 0) {
        fclose(file);
        return 1; /* Empty file is valid */
    }
    
    /* For version 2, read hash table */
    if (format_version >= 2 && header.hash_table_size > 0) {
        hash_table = (kh_hash_entry_t*)calloc(header.hash_table_size, sizeof(kh_hash_entry_t));
        if (!hash_table) {
            fclose(file);
            return 0;
        }
        
        if (fread(hash_table, sizeof(kh_hash_entry_t), header.hash_table_size, file) != header.hash_table_size) {
            free(hash_table);
            fclose(file);
            return 0;
        }
        bytes_read += header.hash_table_size * sizeof(kh_hash_entry_t);
        
        /* Seek to data section */
        if (fseek(file, header.data_section_offset, SEEK_SET) != 0) {
            free(hash_table);
            fclose(file);
            return 0;
        }
        bytes_read = header.data_section_offset;
    }
    
    /* Allocate variables array */
    temp_variables = (kh_variable_t*)calloc(header.variable_count, sizeof(kh_variable_t));
    if (!temp_variables) {
        free(hash_table);
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
            name_len == 0 || name_len > 1024) {
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
    free(hash_table);
    
    if (!success && temp_variables) {
        kh_free_variables(temp_variables, *count);
        *variables = NULL;
        *count = 0;
    }
    
    return success;
}

/* Write binary file with hash table indexing - enhanced for indexed format */
static int kh_write_binary_file(const char* file_path, kh_variable_t* variables, int count) {
    if (!file_path || (!variables && count > 0) || count < 0) return 0;
    
    FILE* file = NULL;
    kh_file_header_t header;
    kh_hash_entry_t* hash_table = NULL;
    int success = 0;
    char* temp_file_path = NULL;
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
        
        if (name_len == 0 || name_len > 1024 || value_len > MAX_TOTAL_KHDATA_SIZE_BYTES) {
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
        hash_table_size = kh_calculate_hash_table_size(count);
        hash_table = (kh_hash_entry_t*)calloc(hash_table_size, sizeof(kh_hash_entry_t));
        if (!hash_table) return 0;
        
        /* Build hash table using consistent linear probing */
        for (int i = 0; i < count; i++) {
            uint32_t name_hash = kh_hash_variable_name(variables[i].name);
            if (!kh_hash_table_insert(hash_table, hash_table_size, name_hash, i)) {
                /* Hash table insertion failed - should not happen with proper sizing */
                free(hash_table);
                return 0;
            }
        }
    }
    
    /* Calculate data section offset with proper alignment */
    data_section_offset = sizeof(kh_file_header_t) + (hash_table_size * sizeof(kh_hash_entry_t));
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
    
    /* Write header with version 2 (indexed format) */
    memset(&header, 0, sizeof(header)); /* Clear reserved fields */
    header.magic = KHDATA_MAGIC;
    header.version = 2; /* Use version 2 for indexed format */
    header.variable_count = (unsigned int)count;
    header.hash_table_size = hash_table_size;
    header.data_section_offset = data_section_offset;
    
    if (fwrite(&header, sizeof(kh_file_header_t), 1, file) != 1) {
        fclose(file);
        DeleteFileA(temp_file_path);
        free(hash_table);
        free(temp_file_path);
        return 0;
    }
    
    /* Write hash table */
    if (hash_table_size > 0) {
        if (fwrite(hash_table, sizeof(kh_hash_entry_t), hash_table_size, file) != hash_table_size) {
            goto write_failure;
        }
    }
    
    /* Pad to data section alignment */
    long current_pos = ftell(file);
    if (current_pos < 0) {
        goto write_failure;
    }
    
    while ((uint32_t)current_pos < data_section_offset) {
        if (fputc(0, file) == EOF) {
            goto write_failure;
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
            goto write_failure;
        }
        
        /* Write type */
        if (fwrite(&type_len, sizeof(unsigned int), 1, file) != 1 ||
            fwrite(type_str, type_len, 1, file) != 1) {
            goto write_failure;
        }
        
        /* Write value */
        if (fwrite(&value_len, sizeof(unsigned int), 1, file) != 1) {
            goto write_failure;
        }
        
        if (value_len > 0) {
            if (fwrite(variables[i].value, value_len, 1, file) != 1) {
                goto write_failure;
            }
        }
    }
    
    /* Flush and close file */
    if (fflush(file) != 0) {
        goto write_failure;
    }
    fclose(file);
    
    /* Atomic replace: move temp file to final location */
    if (DeleteFileA(file_path) || GetLastError() == ERROR_FILE_NOT_FOUND) {
        if (MoveFileA(temp_file_path, file_path)) {
            success = 1;
        }
    }
    
    /* Cleanup temp file if move failed */
    if (!success) {
        DeleteFileA(temp_file_path);
    }
    
    free(hash_table);
    free(temp_file_path);
    return success;
    
write_failure:
    fclose(file);
    DeleteFileA(temp_file_path);
    free(hash_table);
    free(temp_file_path);
    return 0;
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
        case KH_TYPE_BOOL: {
            /* Normalize boolean values to canonical string representation */
            const char* normalized_value;
            
            /* Parse the stored boolean value */
            int bool_val = kh_parse_boolean(value);
            normalized_value = bool_val ? "true" : "false";
            
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, normalized_value) >= 0);
        } 
        default:
            return (_snprintf_s(output, (size_t)output_size, _TRUNCATE, "[\"%s\", %s]", type_str, value) >= 0);
    }
}

/* Parse Arma 3 hashmap entry in format [key, value] - improved memory management */
static int kh_parse_hashmap_entry(const char* input, char** key, char** value) {
    if (!input || !key || !value) return 0;
    
    *key = NULL;
    *value = NULL;
    
    const char* ptr = input;
    int bracket_count = 0;
    int input_len = (int)strlen(input);
    
    /* Input length validation */
    if (input_len < 3) return 0; /* Minimum: [,] */
    if (input_len > MAX_HASHMAP_INPUT_SIZE) return 0; /* 32MB maximum */
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != '[') return 0;
    
    const char* bracket_start = ptr + 1;
    ptr = bracket_start;
    
    /* Find comma while tracking bracket depth - with bounds checking */
    const char* comma_pos = NULL;
    const char* bracket_end = NULL;
    int found_comma = 0;
    
    while (*ptr && (ptr - input) < input_len) {
        if (*ptr == '[') {
            bracket_count++;
        } else if (*ptr == ']') {
            bracket_count--;
            if (bracket_count < 0) {
                bracket_end = ptr;
                break;
            }
        } else if (*ptr == ',' && bracket_count == 0 && !found_comma) {
            comma_pos = ptr;
            found_comma = 1;
        }
        ptr++;
    }
    
    /* Validation: Must have found comma and closing bracket */
    if (!found_comma || !bracket_end || bracket_count != -1) {
        return 0;
    }
    
    /* Extract key with bounds checking */
    size_t key_len = (size_t)(comma_pos - bracket_start);
    if (key_len == 0 || key_len > MAX_HASHMAP_KEY_SIZE) return 0; /* Reasonable key length limit */
    
    /* Trim whitespace from key */
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
        key_len -= 2;
    }
    
    /* Validate key contains only safe characters */
    for (int i = 0; i < key_len; i++) {
        char c = (*key)[i];
        if (c < 32 || c == 127) {
            free(*key);
            *key = NULL;
            return 0;
        }
    }
    
    /* Extract value with bounds checking */
    const char* value_start = comma_pos + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) value_start++;
    
    size_t value_len = (size_t)(bracket_end - value_start);
    if (value_len > MAX_HASHMAP_VALUE_SIZE) { /* Reasonable value length limit */
        free(*key);
        *key = NULL;
        return 0;
    }
    
    /* Trim whitespace from value */
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t')) {
        value_len--;
    }
    
    *value = (char*)malloc((size_t)value_len + 1);
    if (!*value) {
        free(*key);
        *key = NULL;
        return 0;
    }
    
    if (value_len > 0) {
        memcpy(*value, value_start, (size_t)value_len);
    }
    (*value)[value_len] = '\0';
    
    return 1;
}

/* Enhanced full hashmap parsing with depth limits and validation (no size limits) */
static int kh_parse_full_hashmap(const char* input, char*** entries, int max_entries) {
    if (!input || !entries || max_entries <= 0 || max_entries > 1024) return 0;
    
    const char* ptr = input;
    int entry_count = 0;
    int bracket_depth = 0;
    int max_depth = 32; /* Reasonable nesting limit */
    const char* entry_start = NULL;
    int input_len = (int)strlen(input);
    
    /* Input validation */
    if (input_len < 2 || input_len > MAX_HASHMAP_INPUT_SIZE) return 0; /* Max input size limit */
    
    /* Allocate array of string pointers */
    *entries = (char**)calloc((size_t)max_entries, sizeof(char*));
    if (!*entries) return 0;
    
    /* Skip whitespace and find opening bracket */
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ptr++;
    if (*ptr != '[') {
        free(*entries);
        *entries = NULL;
        return 0;
    }
    ptr++; /* Skip opening bracket */
    
    while (*ptr && entry_count < max_entries && (ptr - input) < input_len) {
        if (*ptr == '[') {
            if (bracket_depth == 0) {
                entry_start = ptr;
            }
            bracket_depth++;
            
            if (bracket_depth > max_depth) {
                /* Cleanup on excessive nesting */
                for (int i = 0; i < entry_count; i++) {
                    free((*entries)[i]);
                }
                free(*entries);
                *entries = NULL;
                return 0;
            }
        } else if (*ptr == ']') {
            bracket_depth--;
            if (bracket_depth == 0 && entry_start) {
                /* Found complete entry */
                size_t entry_len = (size_t)(ptr - entry_start + 1);
                
                if (entry_len > 0 && entry_len <= MAX_HASHMAP_ENTRY_SIZE) { /* Reasonable entry size limit */
                    (*entries)[entry_count] = (char*)malloc((size_t)entry_len + 1);
                    if (!(*entries)[entry_count]) {
                        /* Cleanup on allocation failure */
                        for (int i = 0; i < entry_count; i++) {
                            free((*entries)[i]);
                        }
                        free(*entries);
                        *entries = NULL;
                        return 0;
                    }
                    
                    memcpy((*entries)[entry_count], entry_start, (size_t)entry_len);
                    (*entries)[entry_count][entry_len] = '\0';
                    entry_count++;
                }
                entry_start = NULL;
            } else if (bracket_depth < 0) {
                /* Mismatched brackets - cleanup */
                for (int i = 0; i < entry_count; i++) {
                    free((*entries)[i]);
                }
                free(*entries);
                *entries = NULL;
                return 0;
            }
        }
        ptr++;
    }
    
    /* Final validation */
    if (bracket_depth != -1) {
        /* Unbalanced brackets - cleanup */
        for (int i = 0; i < entry_count; i++) {
            free((*entries)[i]);
        }
        free(*entries);
        *entries = NULL;
        return 0;
    }
    
    return entry_count;
}

/* Enhanced hashmap merging with comprehensive validation (no size limits) */
static int kh_merge_hashmap(const char* existing_hashmap, const char* new_entry, char* result, int result_size) {
    if (!existing_hashmap || !new_entry || !result || result_size <= 0) return 0;
    
    /* Size validation */
    int existing_len = (int)strlen(existing_hashmap);
    int new_entry_len = (int)strlen(new_entry);
    
    if (existing_len <= 0 || existing_len > MAX_HASHMAP_INPUT_SIZE || new_entry_len <= 0 || new_entry_len > MAX_HASHMAP_INPUT_SIZE) {
        return 0;
    }
    
    /* Pre-validate existing hashmap format */
    if (!kh_validate_hashmap_format(existing_hashmap)) {
        return 0;
    }
    
    char** entries = NULL;
    int entry_count = 0;
    char* key = NULL;
    char* value = NULL;
    char* temp_result = NULL;
    int ret = 0;
    
    /* Allocate temporary result buffer with safety margin */
    size_t temp_size = (size_t)(existing_len + new_entry_len + HASHMAP_SAFETY_MARGIN);
    temp_result = (char*)malloc(temp_size);
    if (!temp_result) return 0;
    
    /* Initialize with existing hashmap */
    if (strcpy_s(temp_result, temp_size, existing_hashmap) != 0) {
        free(temp_result);
        return 0;
    }
    
    /* Try to parse as full hashmap first */
    entry_count = kh_parse_full_hashmap(new_entry, &entries, 16);
    
    if (entry_count > 0) {
        /* Process each entry */
        for (int i = 0; i < entry_count; i++) {
            if (kh_parse_hashmap_entry(entries[i], &key, &value)) {
                /* Calculate required space for formatted entry */
                size_t key_len = key ? strlen(key) : 0;
                size_t value_len = value ? strlen(value) : 0;
                size_t formatted_len = key_len + value_len + 20; /* Extra space for formatting */
                
                char* formatted_entry = (char*)malloc(formatted_len);
                if (!formatted_entry) {
                    free(key);
                    free(value);
                    goto cleanup;
                }
                
                if (_snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", 
                               key ? key : "", value ? value : "") < 0) {
                    free(formatted_entry);
                    free(key);
                    free(value);
                    goto cleanup;
                }
                
                /* Check if we have enough space in temp_result */
                size_t current_len = strlen(temp_result);
                size_t required_len = current_len + strlen(formatted_entry) + 10;
                
                if (required_len >= temp_size) {
                    free(formatted_entry);
                    free(key);
                    free(value);
                    goto cleanup;
                }
                
                /* Add to temp_result */
                const char* trimmed = temp_result;
                while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
                
                if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                    if (_snprintf_s(temp_result, temp_size, _TRUNCATE, "[%s]", formatted_entry) < 0) {
                        free(formatted_entry);
                        free(key);
                        free(value);
                        goto cleanup;
                    }
                } else {
                    if (current_len > 0 && temp_result[current_len-1] == ']') {
                        if (_snprintf_s(temp_result, temp_size, _TRUNCATE, "%.*s, %s]", 
                                       (int)(current_len - 1), temp_result, formatted_entry) < 0) {
                            free(formatted_entry);
                            free(key);
                            free(value);
                            goto cleanup;
                        }
                    }
                }
                
                free(formatted_entry);
                free(key);
                free(value);
                key = NULL;
                value = NULL;
            }
        }
        
        /* Copy result back with bounds checking */
        if (strlen(temp_result) < (size_t)result_size) {
            if (strcpy_s(result, (size_t)result_size, temp_result) == 0) {
                ret = 1;
            }
        }
    } else {
        /* Fall back to single entry parsing */
        if (kh_parse_hashmap_entry(new_entry, &key, &value)) {
            size_t key_len = key ? strlen(key) : 0;
            size_t value_len = value ? strlen(value) : 0;
            size_t formatted_len = key_len + value_len + 20;
            
            char* formatted_entry = (char*)malloc(formatted_len);
            if (formatted_entry) {
                if (_snprintf_s(formatted_entry, formatted_len, _TRUNCATE, "[\"%s\", %s]", 
                               key ? key : "", value ? value : "") >= 0) {
                    
                    const char* trimmed = existing_hashmap;
                    while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;
                    
                    if (strlen(trimmed) <= 2 || (strlen(trimmed) == 3 && strcmp(trimmed, "[ ]") == 0)) {
                        if (strlen(formatted_entry) + 2 < (size_t)result_size) {
                            if (_snprintf_s(result, (size_t)result_size, _TRUNCATE, "[%s]", formatted_entry) >= 0) {
                                ret = 1;
                            }
                        }
                    } else {
                        size_t existing_len = strlen(existing_hashmap);
                        if (existing_len + strlen(formatted_entry) + 10 < (size_t)result_size && 
                            existing_len > 0 && existing_hashmap[existing_len-1] == ']') {
                            if (_snprintf_s(result, (size_t)result_size, _TRUNCATE, "%.*s, %s]", 
                                           (int)(existing_len - 1), existing_hashmap, formatted_entry) >= 0) {
                                ret = 1;
                            }
                        }
                    }
                }
                free(formatted_entry);
            }
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
    free(temp_result);
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
    
    char file_path[MAX_FILE_PATH_LENGTH];
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
    
    /* Ensure file is in binary format before reading */
    if (!kh_ensure_binary_format(file_path)) {
        kh_set_error(output, output_size, "FAILED TO CONVERT TO BINARY FORMAT");
        goto cleanup;
    }
    
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
        
        /* Handle UTF-8 aware slicing */
        if (slice_index >= 0) {
            int data_len = (int)strlen(result_str);
            int slice_start = slice_index * SLICE_SIZE;
            int slice_end = slice_start + SLICE_SIZE;
            
            if (slice_start >= data_len) {
                kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
            } else {
                if (slice_end > data_len) slice_end = data_len;
                
                /* Ensure we don't break UTF-8 characters */
                slice_end = kh_utf8_safe_slice_end(result_str, data_len, slice_end);
                
                int slice_length = slice_end - slice_start;
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
    
    /* Find specific variable - uses linear search (hash table is internal to file reading) */
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
    
    /* Handle UTF-8 aware slicing */
    if (slice_index >= 0) {
        int data_len = (int)strlen(formatted_output);
        int slice_start = slice_index * SLICE_SIZE;
        int slice_end = slice_start + SLICE_SIZE;
        
        if (slice_start >= data_len) {
            kh_set_error(output, output_size, "SLICE INDEX OUT OF RANGE");
        } else {
            if (slice_end > data_len) slice_end = data_len;
            
            /* Ensure we don't break UTF-8 characters */
            slice_end = kh_utf8_safe_slice_end(formatted_output, data_len, slice_end);
            
            int slice_length = slice_end - slice_start;
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

/* Calculate the formatted size of a variable's output - optimized */
static long kh_get_variable_formatted_size(const char* filename, const char* variable_name) {
    if (!filename || !variable_name) return -1;
    
    char file_path[MAX_FILE_PATH_LENGTH];
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
    
    /* Ensure file is in binary format before reading */
    if (!kh_ensure_binary_format(file_path)) {
        free(clean_var_name);
        free(clean_filename);
        return -1;
    }
    
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
        kh_set_error(output, output_size, "VARIABLE NOT FOUND OR CONVERSION FAILED");
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

/* Enhanced write function with better memory management */
static int kh_write_khdata_variable(const char* filename, const char* variable_name, const char* value, 
                                  const char* type_str, const char* overwrite_str, char* output, int output_size) {
    if (!kh_validate_non_empty_string(filename, "FILENAME", output, output_size) ||
        !kh_validate_non_empty_string(variable_name, "VARIABLE NAME", output, output_size)) {
        return 1;
    }
    
    char file_path[MAX_FILE_PATH_LENGTH];
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
    
    /* Enhanced path validation */
    if (!kh_get_khdata_file_path(filename, file_path, sizeof(file_path))) {
        kh_set_error(output, output_size, "INVALID PATH OR FILENAME");
        return 1;
    }
    
    /* Allocate memory for cleaned strings - account for UTF-8 expansion */
    size_t var_name_len = strlen(variable_name) + 1;
    size_t value_len = strlen(value) + 1;
    
    clean_var_name = (char*)malloc(var_name_len);
    clean_value = (char*)malloc(value_len);
    combined_value = (char*)malloc(value_len * 2 + 1024); /* Extra space for UTF-8 and combination */
    
    if (!clean_var_name || !clean_value || !combined_value) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    kh_clean_string(variable_name, clean_var_name, (int)var_name_len);
    kh_clean_string(value, clean_value, (int)value_len);

    /* Validate UTF-8 in cleaned strings */
    if (!kh_validate_utf8_string(clean_var_name, (int)strlen(clean_var_name))) {
        kh_set_error(output, output_size, "INVALID UTF-8 IN VARIABLE NAME");
        goto cleanup;
    }
    
    if (!kh_validate_utf8_string(clean_value, (int)strlen(clean_value))) {
        kh_set_error(output, output_size, "INVALID UTF-8 IN VALUE");
        goto cleanup;
    }

    /* Variable name security check */
    if (!kh_validate_variable_name(clean_var_name)) {
        kh_set_error(output, output_size, "INVALID VARIABLE NAME - ONLY ALPHANUMERIC AND UNDERSCORE ALLOWED");
        goto cleanup;
    }
    
    /* Value format validation based on data type */
    if (!kh_validate_value_format((int)data_type, clean_value)) {
        switch (data_type) {
            case KH_TYPE_SCALAR:
                kh_set_error(output, output_size, "INVALID SCALAR VALUE - MUST BE A VALID NUMBER");
                break;
            case KH_TYPE_ARRAY:
                kh_set_error(output, output_size, "INVALID ARRAY FORMAT - CHECK BRACKET MATCHING");
                break;
            case KH_TYPE_HASHMAP:
                kh_set_error(output, output_size, "INVALID HASHMAP FORMAT - CHECK BRACKET MATCHING AND STRUCTURE");
                break;
            case KH_TYPE_BOOL:
                kh_set_error(output, output_size, "INVALID BOOLEAN VALUE - MUST BE true/false/1/0");
                break;
            default:
                kh_set_error(output, output_size, "INVALID VALUE FORMAT FOR TYPE");
                break;
        }
        goto cleanup;
    }

    /* Allow empty strings for certain data types */
    if (strlen(clean_value) == 0 && data_type != KH_TYPE_STRING && data_type != KH_TYPE_TEXT && data_type != KH_TYPE_CODE) {
        kh_set_error(output, output_size, "EMPTY VALUE");
        goto cleanup;
    }
    
    /* Ensure file is in binary format before reading */
    if (!kh_ensure_binary_format(file_path)) {
        kh_set_error(output, output_size, "FAILED TO CONVERT TO BINARY FORMAT");
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
        /* Check for type mismatch */
        if (variables[var_index].type != data_type) {
            kh_set_error(output, output_size, "TYPE MISMATCH - USE OVERWRITE TO CHANGE TYPE");
            goto cleanup;
        }

        /* Combine with existing value */
        if (!kh_combine_values(data_type, variables[var_index].value, clean_value, 
                             overwrite_flag, combined_value, (int)(value_len * 2 + 1024))) {
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
        
        size_t clean_value_len = strlen(clean_value) + 1;
        variables[var_index].value = (char*)malloc(clean_value_len);
        if (!variables[var_index].value) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            goto cleanup;
        }
        strcpy_s(variables[var_index].value, clean_value_len, clean_value);
        variables[var_index].type = data_type;
    }
    
    /* Write file with new indexed format */
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