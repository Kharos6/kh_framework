#ifndef GENERATE_RANDOM_STRING_HPP
#define GENERATE_RANDOM_STRING_HPP

#include <time.h>

/* Character sets */
static const char NUMBERS[] = "0123456789";
static const char LETTERS[] = "abcdefghijklmnopqrstuvwxyz";
static const char SYMBOLS[] = "!@#$%^&*()_+-=[]{}|;:,.<>?/~`\\";

/* (LCG - Linear Congruential Generator) */
static unsigned int rng_seed = 0;

/* Initialize seed once */
static void init_random(void) {
    if (rng_seed == 0) {
        rng_seed = (unsigned int)time(NULL) ^ (unsigned int)GetTickCount();
    }
}

/* Fast random number generator */
static inline unsigned int fast_rand(void) {
    rng_seed = rng_seed * 1103515245 + 12345;
    return rng_seed;
}

/* Helper function to compare strings, ignoring surrounding quotes */
static int string_equals_ignore_quotes(const char* str1, const char* str2) {
    const char* start = str1;
    const char* end = str1 + strlen(str1) - 1;
    
    /* Skip opening quote if present */
    if (*start == '"') {
        start++;
    }
    
    /* Skip closing quote if present */
    if (end >= start && *end == '"') {
        end--;
    }
    
    /* Calculate length without quotes */
    int len = (int)(end - start + 1);
    int str2_len = strlen(str2);
    
    /* Compare lengths first */
    if (len != str2_len) {
        return 0;
    }
    
    /* Compare strings */
    return strncmp(start, str2, len) == 0;
}

/* Build character set based on type arguments */
static int build_charset(char** charset, const char **argv, int argc, int start_index) {
    int charset_len = 0;
    int i, j;
    int include_numbers = 0;
    int include_letters = 0;
    int include_symbols = 0;
    int total_size;
    
    /* Check what types are requested */
    for (i = start_index; i < argc; i++) {
        if (string_equals_ignore_quotes(argv[i], "NUMBERS")) {
            include_numbers = 1;
        } else if (string_equals_ignore_quotes(argv[i], "LETTERS")) {
            include_letters = 1;
        } else if (string_equals_ignore_quotes(argv[i], "SYMBOLS")) {
            include_symbols = 1;
        }
    }
    
    /* If no types specified, default to numbers and letters */
    if (!include_numbers && !include_letters && !include_symbols) {
        include_numbers = 1;
        include_letters = 1;
    }
    
    /* Calculate total size needed */
    total_size = 0;
    if (include_numbers) total_size += strlen(NUMBERS);
    if (include_letters) total_size += strlen(LETTERS);
    if (include_symbols) total_size += strlen(SYMBOLS);
    
    /* Allocate charset buffer */
    *charset = (char*)malloc(total_size + 1);
    if (!*charset) {
        return 0; /* Memory allocation failed */
    }
    
    /* Build the character set */
    charset_len = 0;
    if (include_numbers) {
        for (j = 0; NUMBERS[j] != '\0'; j++) {
            (*charset)[charset_len++] = NUMBERS[j];
        }
    }
    
    if (include_letters) {
        for (j = 0; LETTERS[j] != '\0'; j++) {
            (*charset)[charset_len++] = LETTERS[j];
        }
    }
    
    if (include_symbols) {
        for (j = 0; SYMBOLS[j] != '\0'; j++) {
            (*charset)[charset_len++] = SYMBOLS[j];
        }
    }
    
    (*charset)[charset_len] = '\0';
    return charset_len;
}

/* Function to generate random string with custom charset */
static void generate_string(char* buffer, int length, const char* charset, int charset_size) {
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = charset[fast_rand() % charset_size];
    }
}

/* Process random string generation request */
static int process_random_string_generation(char *output, int outputSize, const char **argv, int argc) {
    int length;
    char* charset;
    int charset_size;
    
    init_random();
    
    if (argc < 1) {
        strcpy_s(output, outputSize, "KH_ERROR: NO ARGUMENTS");
        return 1;
    }
    
    length = atoi(argv[0]);
    
    if (length <= 0) {
        strcpy_s(output, outputSize, "KH_ERROR: INVALID LENGTH");
        return 1;
    }
    
    if (length >= outputSize) {
        strcpy_s(output, outputSize, "KH_ERROR: LENGTH TOO LARGE");
        return 1;
    }
    
    /* Build character set based on type arguments */
    charset_size = build_charset(&charset, argv, argc, 1);
    
    /* If no character types specified, use default */
    if (charset_size == 0) {
        charset = (char*)malloc(strlen(NUMBERS) + strlen(LETTERS) + 1);
        if (!charset) {
            strcpy_s(output, outputSize, "KH_ERROR: MEMORY ALLOCATION FAILED");
            return 1;
        }
        strcpy_s(charset, strlen(NUMBERS) + strlen(LETTERS) + 1, NUMBERS);
        strcat_s(charset, strlen(NUMBERS) + strlen(LETTERS) + 1, LETTERS);
        charset_size = strlen(charset);
    }
    
    generate_string(output, length, charset, charset_size);
    output[length] = '\0';
    
    free(charset);
    return 0;
}

#endif /* GENERATE_RANDOM_STRING_HPP */