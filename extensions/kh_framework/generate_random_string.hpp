#ifndef GENERATE_RANDOM_STRING_HPP
#define GENERATE_RANDOM_STRING_HPP

/* Character sets for random string generation */
static const char KH_CHARSET_NUMBERS[] = "0123456789";
static const char KH_CHARSET_LETTERS[] = "abcdefghijklmnopqrstuvwxyz";
static const char KH_CHARSET_SYMBOLS[] = "!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";

/* Character set information structure for better performance */
typedef struct {
    const char* charset;
    int length;
} charset_info_t;

static const charset_info_t KH_CHARSET_INFO[] = {
    {KH_CHARSET_NUMBERS, sizeof(KH_CHARSET_NUMBERS) - 1},
    {KH_CHARSET_LETTERS, sizeof(KH_CHARSET_LETTERS) - 1},
    {KH_CHARSET_SYMBOLS, sizeof(KH_CHARSET_SYMBOLS) - 1}
};

/* Build character set based on boolean flags [numbers, letters, symbols] - optimized */
static int kh_build_charset(char** charset, const char **argv, int argc) {
    if (!charset || !argv) return 0;
    
    int include_numbers = 0;
    int include_letters = 0;
    int include_symbols = 0;
    int total_size = 0;
    int charset_len = 0;
    
    /* Parse boolean flags from arguments */
    if (argc >= 2) include_numbers = kh_parse_boolean(argv[1]);
    if (argc >= 3) include_letters = kh_parse_boolean(argv[2]);
    if (argc >= 4) include_symbols = kh_parse_boolean(argv[3]);
    
    /* If no types specified, default to numbers and letters */
    if (!include_numbers && !include_letters && !include_symbols) {
        include_numbers = 1;
        include_letters = 1;
    }
    
    /* Calculate total size needed using pre-calculated lengths */
    if (include_numbers) total_size += KH_CHARSET_INFO[0].length;
    if (include_letters) total_size += KH_CHARSET_INFO[1].length;
    if (include_symbols) total_size += KH_CHARSET_INFO[2].length;
    
    if (total_size == 0) return 0;
    
    /* Allocate charset buffer */
    *charset = (char*)malloc((size_t)total_size + 1);
    if (!*charset) {
        return 0;
    }
    
    /* Build the character set efficiently using memcpy */
    if (include_numbers) {
        memcpy(*charset + charset_len, KH_CHARSET_INFO[0].charset, KH_CHARSET_INFO[0].length);
        charset_len += KH_CHARSET_INFO[0].length;
    }
    
    if (include_letters) {
        memcpy(*charset + charset_len, KH_CHARSET_INFO[1].charset, KH_CHARSET_INFO[1].length);
        charset_len += KH_CHARSET_INFO[1].length;
    }
    
    if (include_symbols) {
        memcpy(*charset + charset_len, KH_CHARSET_INFO[2].charset, KH_CHARSET_INFO[2].length);
        charset_len += KH_CHARSET_INFO[2].length;
    }
    
    (*charset)[charset_len] = '\0';
    return charset_len;
}

/* Generate random string using specified character set - optimized */
static inline void kh_generate_random_string(char* buffer, int length, const char* charset, int charset_size) {
    if (!buffer || length <= 0 || !charset || charset_size <= 0) return;
    
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = charset[kh_fast_rand() % charset_size];
    }
    buffer[length] = '\0';
}

/* Process random string generation request - enhanced error handling */
static int kh_process_random_string_generation(char *output, int output_size, const char **argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc < 1) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    int length;
    char* charset = NULL;
    int charset_size;
    int result = 1;
    
    kh_init_random();

    length = atoi(argv[0]);
    
    if (length <= 0) {
        kh_set_error(output, output_size, "INVALID LENGTH - MUST BE POSITIVE INTEGER");
        return 1;
    }

    if (length > SLICE_SIZE) {
        kh_set_error(output, output_size, "LENGTH TOO LARGE FOR OUTPUT BUFFER");
        return 1;
    }
    
    /* Check if output buffer can hold the result */
    if (length >= output_size) {
        kh_set_error(output, output_size, "OUTPUT BUFFER TOO SMALL");
        return 1;
    }
    
    /* Build character set based on boolean flags */
    charset_size = kh_build_charset(&charset, argv, argc);
    
    /* Fallback to default charset if build failed */
    if (charset_size == 0 || !charset) {
        const int numbers_len = KH_CHARSET_INFO[0].length;
        const int letters_len = KH_CHARSET_INFO[1].length;
        
        charset = (char*)malloc((size_t)(numbers_len + letters_len + 1));
        if (!charset) {
            kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
            return 1;
        }
        
        memcpy(charset, KH_CHARSET_INFO[0].charset, numbers_len);
        memcpy(charset + numbers_len, KH_CHARSET_INFO[1].charset, letters_len);
        charset_size = numbers_len + letters_len;
        charset[charset_size] = '\0';
    }
    
    /* Generate the random string */
    kh_generate_random_string(output, length, charset, charset_size);
    result = 0;
    
    free(charset);
    return result;
}

#endif /* GENERATE_RANDOM_STRING_HPP */