#ifndef STRING_OPERATIONS_HPP
#define STRING_OPERATIONS_HPP

#include "common_defines.hpp"
#include <stdint.h>
#include <ctype.h>

/* Constants for string operations */
#define KH_STRING_ENCODE_MAGIC 0x4B48          /* "KH" in little endian */
#define KH_STRING_MAX_INPUT_SIZE (64 * 1024 * 1024) /* 64MB max input */
#define KH_STRING_ESCAPE_CHAR 0xFF             /* Escape character for unmapped chars */
#define KH_STRING_RLE_CHAR 0xFE                /* Run-length encoding marker */
#define KH_STRING_TABLE_SIZE 256               /* Size of encoding table */
#define KH_STRING_MIN_RLE_COUNT 3              /* Minimum count for RLE */
#define KH_STRING_MAX_SPLIT_PARTS 1024         /* Maximum parts for string splitting */
#define KH_STRING_MAX_REPLACE_COUNT 10000      /* Maximum replacements to prevent infinite loops */

/* String operation function types */
typedef enum {
    KH_STRING_FUNC_BOOL,          /* String -> Boolean */
    KH_STRING_FUNC_NUMBER,        /* String -> Number */
    KH_STRING_FUNC_STRING,        /* String -> String */
    KH_STRING_FUNC_STRING_PARAM,  /* String, Parameter -> String */
    KH_STRING_FUNC_STRING_2PARAM, /* String, Parameter1, Parameter2 -> String */
    KH_STRING_FUNC_STRING_3PARAM, /* String, Parameter1, Parameter2, Parameter3 -> String */
    KH_STRING_FUNC_ENCODE_DECODE  /* Special encode/decode functions */
} kh_string_func_type_t;

/* String function definition structure */
typedef struct {
    const char* name;
    kh_string_func_type_t type;
    int arg_count;
    void* func_ptr;
    const char* description;
} kh_string_function_t;

/* Character frequency table - most common UTF-8 characters ordered by frequency */
/* This gives us optimal compression for common text */
static const struct {
    uint32_t codepoint;
    uint8_t code;
} KH_CHAR_FREQUENCY_TABLE[] = {
    /* Most frequent characters (based on English + international usage) */
    {0x0020, 0x01}, /* space */
    {0x0065, 0x02}, /* 'e' */
    {0x0074, 0x03}, /* 't' */
    {0x0061, 0x04}, /* 'a' */
    {0x006F, 0x05}, /* 'o' */
    {0x0069, 0x06}, /* 'i' */
    {0x006E, 0x07}, /* 'n' */
    {0x0073, 0x08}, /* 's' */
    {0x0068, 0x09}, /* 'h' */
    {0x0072, 0x0A}, /* 'r' */
    {0x0064, 0x0B}, /* 'd' */
    {0x006C, 0x0C}, /* 'l' */
    {0x0075, 0x0D}, /* 'u' */
    {0x0063, 0x0E}, /* 'c' */
    {0x006D, 0x0F}, /* 'm' */
    {0x0077, 0x10}, /* 'w' */
    {0x0066, 0x11}, /* 'f' */
    {0x0067, 0x12}, /* 'g' */
    {0x0079, 0x13}, /* 'y' */
    {0x0070, 0x14}, /* 'p' */
    {0x0062, 0x15}, /* 'b' */
    {0x0076, 0x16}, /* 'v' */
    {0x006B, 0x17}, /* 'k' */
    {0x006A, 0x18}, /* 'j' */
    {0x0078, 0x19}, /* 'x' */
    {0x0071, 0x1A}, /* 'q' */
    {0x007A, 0x1B}, /* 'z' */
    
    /* Uppercase letters */
    {0x0045, 0x1C}, /* 'E' */
    {0x0054, 0x1D}, /* 'T' */
    {0x0041, 0x1E}, /* 'A' */
    {0x004F, 0x1F}, /* 'O' */
    {0x0049, 0x20}, /* 'I' */
    {0x004E, 0x21}, /* 'N' */
    {0x0053, 0x22}, /* 'S' */
    {0x0048, 0x23}, /* 'H' */
    {0x0052, 0x24}, /* 'R' */
    {0x0044, 0x25}, /* 'D' */
    {0x004C, 0x26}, /* 'L' */
    {0x0055, 0x27}, /* 'U' */
    {0x0043, 0x28}, /* 'C' */
    {0x004D, 0x29}, /* 'M' */
    {0x0057, 0x2A}, /* 'W' */
    {0x0046, 0x2B}, /* 'F' */
    {0x0047, 0x2C}, /* 'G' */
    {0x0059, 0x2D}, /* 'Y' */
    {0x0050, 0x2E}, /* 'P' */
    {0x0042, 0x2F}, /* 'B' */
    {0x0056, 0x30}, /* 'V' */
    {0x004B, 0x31}, /* 'K' */
    {0x004A, 0x32}, /* 'J' */
    {0x0058, 0x33}, /* 'X' */
    {0x0051, 0x34}, /* 'Q' */
    {0x005A, 0x35}, /* 'Z' */
    
    /* Digits */
    {0x0030, 0x36}, /* '0' */
    {0x0031, 0x37}, /* '1' */
    {0x0032, 0x38}, /* '2' */
    {0x0033, 0x39}, /* '3' */
    {0x0034, 0x3A}, /* '4' */
    {0x0035, 0x3B}, /* '5' */
    {0x0036, 0x3C}, /* '6' */
    {0x0037, 0x3D}, /* '7' */
    {0x0038, 0x3E}, /* '8' */
    {0x0039, 0x3F}, /* '9' */
    
    /* Common punctuation and symbols */
    {0x002E, 0x40}, /* '.' */
    {0x002C, 0x41}, /* ',' */
    {0x003A, 0x42}, /* ':' */
    {0x003B, 0x43}, /* ';' */
    {0x0021, 0x44}, /* '!' */
    {0x003F, 0x45}, /* '?' */
    {0x0028, 0x46}, /* '(' */
    {0x0029, 0x47}, /* ')' */
    {0x005B, 0x48}, /* '[' */
    {0x005D, 0x49}, /* ']' */
    {0x007B, 0x4A}, /* '{' */
    {0x007D, 0x4B}, /* '}' */
    {0x0022, 0x4C}, /* '"' */
    {0x0027, 0x4D}, /* '\'' */
    {0x002D, 0x4E}, /* '-' */
    {0x005F, 0x4F}, /* '_' */
    {0x002F, 0x50}, /* '/' */
    {0x005C, 0x51}, /* '\\' */
    {0x002B, 0x52}, /* '+' */
    {0x003D, 0x53}, /* '=' */
    {0x002A, 0x54}, /* '*' */
    {0x0026, 0x55}, /* '&' */
    {0x0025, 0x56}, /* '%' */
    {0x0024, 0x57}, /* '$' */
    {0x0023, 0x58}, /* '#' */
    {0x0040, 0x59}, /* '@' */
    {0x007C, 0x5A}, /* '|' */
    {0x007E, 0x5B}, /* '~' */
    {0x0060, 0x5C}, /* '`' */
    {0x005E, 0x5D}, /* '^' */
    {0x003C, 0x5E}, /* '<' */
    {0x003E, 0x5F}, /* '>' */
    
    /* Whitespace and control characters */
    {0x000A, 0x60}, /* '\n' */
    {0x000D, 0x61}, /* '\r' */
    {0x0009, 0x62}, /* '\t' */
    
    /* Common extended ASCII */
    {0x00A0, 0x63}, /* Non-breaking space */
    {0x00A1, 0x64}, /* ¡ */
    {0x00A2, 0x65}, /* ¢ */
    {0x00A3, 0x66}, /* £ */
    {0x00A4, 0x67}, /* ¤ */
    {0x00A5, 0x68}, /* ¥ */
    {0x00A6, 0x69}, /* ¦ */
    {0x00A7, 0x6A}, /* § */
    {0x00A8, 0x6B}, /* ¨ */
    {0x00A9, 0x6C}, /* © */
    {0x00AA, 0x6D}, /* ª */
    {0x00AB, 0x6E}, /* « */
    {0x00AC, 0x6F}, /* ¬ */
    {0x00AD, 0x70}, /* Soft hyphen */
    {0x00AE, 0x71}, /* ® */
    {0x00AF, 0x72}, /* ¯ */
    {0x00B0, 0x73}, /* ° */
    {0x00B1, 0x74}, /* ± */
    {0x00B2, 0x75}, /* ² */
    {0x00B3, 0x76}, /* ³ */
    {0x00B4, 0x77}, /* ´ */
    {0x00B5, 0x78}, /* µ */
    {0x00B6, 0x79}, /* ¶ */
    {0x00B7, 0x7A}, /* · */
    {0x00B8, 0x7B}, /* ¸ */
    {0x00B9, 0x7C}, /* ¹ */
    {0x00BA, 0x7D}, /* º */
    {0x00BB, 0x7E}, /* » */
    {0x00BC, 0x7F}, /* ¼ */
    {0x00BD, 0x80}, /* ½ */
    {0x00BE, 0x81}, /* ¾ */
    {0x00BF, 0x82}, /* ¿ */
    
    /* Common accented characters */
    {0x00C0, 0x83}, /* À */
    {0x00C1, 0x84}, /* Á */
    {0x00C2, 0x85}, /* Â */
    {0x00C3, 0x86}, /* Ã */
    {0x00C4, 0x87}, /* Ä */
    {0x00C5, 0x88}, /* Å */
    {0x00C6, 0x89}, /* Æ */
    {0x00C7, 0x8A}, /* Ç */
    {0x00C8, 0x8B}, /* È */
    {0x00C9, 0x8C}, /* É */
    {0x00CA, 0x8D}, /* Ê */
    {0x00CB, 0x8E}, /* Ë */
    {0x00CC, 0x8F}, /* Ì */
    {0x00CD, 0x90}, /* Í */
    {0x00CE, 0x91}, /* Î */
    {0x00CF, 0x92}, /* Ï */
    {0x00D0, 0x93}, /* Ð */
    {0x00D1, 0x94}, /* Ñ */
    {0x00D2, 0x95}, /* Ò */
    {0x00D3, 0x96}, /* Ó */
    {0x00D4, 0x97}, /* Ô */
    {0x00D5, 0x98}, /* Õ */
    {0x00D6, 0x99}, /* Ö */
    {0x00D7, 0x9A}, /* × */
    {0x00D8, 0x9B}, /* Ø */
    {0x00D9, 0x9C}, /* Ù */
    {0x00DA, 0x9D}, /* Ú */
    {0x00DB, 0x9E}, /* Û */
    {0x00DC, 0x9F}, /* Ü */
    {0x00DD, 0xA0}, /* Ý */
    {0x00DE, 0xA1}, /* Þ */
    {0x00DF, 0xA2}, /* ß */
    {0x00E0, 0xA3}, /* à */
    {0x00E1, 0xA4}, /* á */
    {0x00E2, 0xA5}, /* â */
    {0x00E3, 0xA6}, /* ã */
    {0x00E4, 0xA7}, /* ä */
    {0x00E5, 0xA8}, /* å */
    {0x00E6, 0xA9}, /* æ */
    {0x00E7, 0xAA}, /* ç */
    {0x00E8, 0xAB}, /* è */
    {0x00E9, 0xAC}, /* é */
    {0x00EA, 0xAD}, /* ê */
    {0x00EB, 0xAE}, /* ë */
    {0x00EC, 0xAF}, /* ì */
    {0x00ED, 0xB0}, /* í */
    {0x00EE, 0xB1}, /* î */
    {0x00EF, 0xB2}, /* ï */
    {0x00F0, 0xB3}, /* ð */
    {0x00F1, 0xB4}, /* ñ */
    {0x00F2, 0xB5}, /* ò */
    {0x00F3, 0xB6}, /* ó */
    {0x00F4, 0xB7}, /* ô */
    {0x00F5, 0xB8}, /* õ */
    {0x00F6, 0xB9}, /* ö */
    {0x00F7, 0xBA}, /* ÷ */
    {0x00F8, 0xBB}, /* ø */
    {0x00F9, 0xBC}, /* ù */
    {0x00FA, 0xBD}, /* ú */
    {0x00FB, 0xBE}, /* û */
    {0x00FC, 0xBF}, /* ü */
    {0x00FD, 0xC0}, /* ý */
    {0x00FE, 0xC1}, /* þ */
    {0x00FF, 0xC2}, /* ÿ */
    
    /* Some common Unicode characters */
    {0x2013, 0xC3}, /* – (en dash) */
    {0x2014, 0xC4}, /* — (em dash) */
    {0x2018, 0xC5}, /* ' (left single quotation mark) */
    {0x2019, 0xC6}, /* ' (right single quotation mark) */
    {0x201A, 0xC7}, /* ‚ (single low-9 quotation mark) */
    {0x201C, 0xC8}, /* " (left double quotation mark) */
    {0x201D, 0xC9}, /* " (right double quotation mark) */
    {0x201E, 0xCA}, /* „ (double low-9 quotation mark) */
    {0x2020, 0xCB}, /* † (dagger) */
    {0x2021, 0xCC}, /* ‡ (double dagger) */
    {0x2022, 0xCD}, /* • (bullet) */
    {0x2026, 0xCE}, /* … (horizontal ellipsis) */
    {0x2030, 0xCF}, /* ‰ (per mille sign) */
    {0x2032, 0xD0}, /* ′ (prime) */
    {0x2033, 0xD1}, /* ″ (double prime) */
    {0x2039, 0xD2}, /* ‹ (single left-pointing angle quotation mark) */
    {0x203A, 0xD3}, /* › (single right-pointing angle quotation mark) */
    {0x20AC, 0xD4}, /* € (euro sign) */
    {0x2122, 0xD5}, /* ™ (trade mark sign) */
    
    /* Reserve codes 0xD6-0xFD for future expansion */
    /* 0xFE is reserved for RLE marker */
    /* 0xFF is reserved for escape character */
};

static const int KH_CHAR_FREQUENCY_TABLE_SIZE = sizeof(KH_CHAR_FREQUENCY_TABLE) / sizeof(KH_CHAR_FREQUENCY_TABLE[0]);

/* Encoding/decoding lookup tables */
static uint8_t* encode_table = NULL;          /* UTF-8 codepoint -> compressed code */
static uint32_t* decode_table = NULL;         /* compressed code -> UTF-8 codepoint */
static int tables_initialized = 0;

/* Base64 encoding table for final output */
static const char KH_BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char KH_BASE64_PAD = '=';

/* Initialize encoding/decoding tables */
static inline int kh_init_string_tables(void) {
    if (tables_initialized) return 1;
    
    /* Allocate tables - encode_table covers full Unicode range */
    encode_table = (uint8_t*)calloc(0x110000, sizeof(uint8_t));
    if (!encode_table) {
        return 0;
    }
    
    decode_table = (uint32_t*)calloc(256, sizeof(uint32_t));
    if (!decode_table) {
        free(encode_table);
        encode_table = NULL;
        return 0;
    }
    
    /* Build tables from frequency data */
    for (int i = 0; i < KH_CHAR_FREQUENCY_TABLE_SIZE; i++) {
        uint32_t codepoint = KH_CHAR_FREQUENCY_TABLE[i].codepoint;
        uint8_t code = KH_CHAR_FREQUENCY_TABLE[i].code;
        
        if (codepoint < 0x110000) {
            encode_table[codepoint] = code;
            decode_table[code] = codepoint;
        }
    }
    
    tables_initialized = 1;
    return 1;
}

/* Free encoding/decoding tables */
static inline void kh_free_string_tables(void) {
    if (tables_initialized) {
        free(encode_table);
        free(decode_table);
        encode_table = NULL;
        decode_table = NULL;
        tables_initialized = 0;
    }
}

/* Get UTF-8 character length from first byte */
static inline int kh_utf8_char_length(unsigned char first_byte) {
    if (first_byte < 0x80) return 1;
    if ((first_byte & 0xE0) == 0xC0) return 2;
    if ((first_byte & 0xF0) == 0xE0) return 3;
    if ((first_byte & 0xF8) == 0xF0) return 4;
    return 0; /* Invalid UTF-8 */
}

/* Convert UTF-8 character to Unicode codepoint */
static inline uint32_t kh_utf8_to_codepoint(const char* utf8_char, int char_len) {
    if (!utf8_char || char_len <= 0) return 0;
    
    unsigned char* bytes = (unsigned char*)utf8_char;
    uint32_t codepoint = 0;
    
    switch (char_len) {
        case 1:
            codepoint = bytes[0];
            break;
        case 2:
            codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
            break;
        case 3:
            codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
            break;
        case 4:
            codepoint = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | 
                       ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
            break;
    }
    
    return codepoint;
}

/* Convert Unicode codepoint to UTF-8 */
static inline int kh_codepoint_to_utf8(uint32_t codepoint, char* utf8_char, int max_len) {
    if (!utf8_char || max_len <= 0) return 0;
    
    if (codepoint < 0x80) {
        if (max_len < 1) return 0;
        utf8_char[0] = (char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        if (max_len < 2) return 0;
        utf8_char[0] = (char)(0xC0 | (codepoint >> 6));
        utf8_char[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint < 0x10000) {
        if (max_len < 3) return 0;
        utf8_char[0] = (char)(0xE0 | (codepoint >> 12));
        utf8_char[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_char[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint < 0x110000) {
        if (max_len < 4) return 0;
        utf8_char[0] = (char)(0xF0 | (codepoint >> 18));
        utf8_char[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8_char[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_char[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    
    return 0; /* Invalid codepoint */
}

/* Encode binary data to base64 */
static inline int kh_encode_base64(const unsigned char* input, int input_len, char* output, int output_size) {
    if (!input || !output || input_len <= 0 || output_size <= 0) return 0;
    
    int output_len = ((input_len + 2) / 3) * 4;
    if (output_len >= output_size) return 0;
    
    int i, j;
    for (i = 0, j = 0; i < input_len; i += 3, j += 4) {
        uint32_t triple = (input[i] << 16);
        if (i + 1 < input_len) triple |= (input[i + 1] << 8);
        if (i + 2 < input_len) triple |= input[i + 2];
        
        output[j] = KH_BASE64_CHARS[(triple >> 18) & 0x3F];
        output[j + 1] = KH_BASE64_CHARS[(triple >> 12) & 0x3F];
        output[j + 2] = (i + 1 < input_len) ? KH_BASE64_CHARS[(triple >> 6) & 0x3F] : KH_BASE64_PAD;
        output[j + 3] = (i + 2 < input_len) ? KH_BASE64_CHARS[triple & 0x3F] : KH_BASE64_PAD;
    }
    
    output[output_len] = '\0';
    return output_len;
}

/* Decode base64 to binary data */
static inline int kh_decode_base64(const char* input, unsigned char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    int input_len = (int)strlen(input);
    if (input_len % 4 != 0) return 0;
    
    /* Build decode table */
    int decode_base64_table[256];
    for (int i = 0; i < 256; i++) decode_base64_table[i] = -1;
    for (int i = 0; i < 64; i++) decode_base64_table[(unsigned char)KH_BASE64_CHARS[i]] = i;
    
    int output_len = 0;
    for (int i = 0; i < input_len; i += 4) {
        if (output_len + 3 > output_size) return 0;
        
        int a = decode_base64_table[(unsigned char)input[i]];
        int b = decode_base64_table[(unsigned char)input[i + 1]];
        int c = (input[i + 2] == KH_BASE64_PAD) ? 0 : decode_base64_table[(unsigned char)input[i + 2]];
        int d = (input[i + 3] == KH_BASE64_PAD) ? 0 : decode_base64_table[(unsigned char)input[i + 3]];
        
        if (a == -1 || b == -1 || (input[i + 2] != KH_BASE64_PAD && c == -1) || 
            (input[i + 3] != KH_BASE64_PAD && d == -1)) {
            return 0;
        }
        
        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;
        
        output[output_len++] = (unsigned char)((triple >> 16) & 0xFF);
        if (input[i + 2] != KH_BASE64_PAD) output[output_len++] = (unsigned char)((triple >> 8) & 0xFF);
        if (input[i + 3] != KH_BASE64_PAD) output[output_len++] = (unsigned char)(triple & 0xFF);
    }
    
    return output_len;
}

/* Encode UTF-8 string to compressed binary format */
static inline int kh_encode_string_binary(const char* input, unsigned char** output, int* output_len) {
    if (!input || !output || !output_len) return 0;
    
    *output = NULL;
    *output_len = 0;
    
    if (!kh_init_string_tables()) return 0;
    
    int input_len = (int)strlen(input);
    if (input_len > KH_STRING_MAX_INPUT_SIZE) return 0;
    
    /* Allocate output buffer - worst case is 2x input size */
    unsigned char* buffer = (unsigned char*)malloc((size_t)input_len * 2 + 16);
    if (!buffer) return 0;
    
    int result = 0;
    int pos = 4;
    int i = 0;
    
    /* Write header */
    buffer[0] = (unsigned char)(KH_STRING_ENCODE_MAGIC & 0xFF);
    buffer[1] = (unsigned char)((KH_STRING_ENCODE_MAGIC >> 8) & 0xFF);
    buffer[2] = 1; /* Version */
    buffer[3] = 0; /* Reserved */
    
    while (i < input_len) {
        int char_len = kh_utf8_char_length((unsigned char)input[i]);
        if (char_len <= 0 || i + char_len > input_len) {
            goto cleanup;
        }
        
        uint32_t codepoint = kh_utf8_to_codepoint(&input[i], char_len);
        
        /* Check for run-length encoding opportunity */
        int run_count = 1;
        int next_pos = i + char_len;
        while (next_pos < input_len && run_count < 255) {
            int next_char_len = kh_utf8_char_length((unsigned char)input[next_pos]);
            if (next_char_len == char_len) {
                uint32_t next_codepoint = kh_utf8_to_codepoint(&input[next_pos], next_char_len);
                if (next_codepoint == codepoint) {
                    run_count++;
                    next_pos += next_char_len;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        /* Check buffer bounds before writing */
        if (pos >= (input_len * 2 + 16) - 10) {
            goto cleanup;
        }
        
        /* Use RLE if beneficial */
        if (run_count >= KH_STRING_MIN_RLE_COUNT && codepoint < 0x110000 && encode_table[codepoint] != 0) {
            buffer[pos++] = KH_STRING_RLE_CHAR;
            buffer[pos++] = (unsigned char)run_count;
            buffer[pos++] = encode_table[codepoint];
            i = next_pos;
        } else {
            /* Single character encoding */
            if (codepoint < 0x110000 && encode_table[codepoint] != 0) {
                /* Character is in our table */
                buffer[pos++] = encode_table[codepoint];
            } else {
                /* Character not in table - use escape sequence */
                buffer[pos++] = KH_STRING_ESCAPE_CHAR;
                buffer[pos++] = (unsigned char)char_len;
                for (int j = 0; j < char_len; j++) {
                    buffer[pos++] = (unsigned char)input[i + j];
                }
            }
            i += char_len;
        }
    }
    
    /* Success - transfer ownership */
    *output = buffer;
    *output_len = pos;
    result = 1;
    buffer = NULL; /* Prevent cleanup */

cleanup:
    free(buffer);
    return result;
}

/* Decode compressed binary format to UTF-8 string */
static inline int kh_decode_string_binary(const unsigned char* input, int input_len, char** output) {
    if (!input || input_len < 4 || !output) return 0;
    
    *output = NULL;
    
    if (!kh_init_string_tables()) return 0;
    
    /* Verify header */
    uint16_t magic = input[0] | (input[1] << 8);
    if (magic != KH_STRING_ENCODE_MAGIC || input[2] != 1) return 0;
    
    /* Allocate output buffer */
    char* buffer = (char*)malloc((size_t)input_len * 4 + 1);
    if (!buffer) return 0;
    
    int result = 0;
    int pos = 4;
    int out_pos = 0;
    
    while (pos < input_len) {
        unsigned char byte = input[pos++];
        
        if (byte == KH_STRING_RLE_CHAR) {
            /* Run-length encoded character */
            if (pos + 1 >= input_len) {
                goto cleanup;
            }
            
            unsigned char count = input[pos++];
            unsigned char code = input[pos++];
            
            if (decode_table[code] == 0) {
                goto cleanup;
            }
            
            uint32_t codepoint = decode_table[code];
            char utf8_char[4];
            int utf8_len = kh_codepoint_to_utf8(codepoint, utf8_char, 4);
            
            if (utf8_len <= 0) {
                goto cleanup;
            }
            
            /* Check buffer bounds */
            if (out_pos + (count * utf8_len) >= input_len * 4) {
                goto cleanup;
            }
            
            for (int i = 0; i < count; i++) {
                for (int j = 0; j < utf8_len; j++) {
                    buffer[out_pos++] = utf8_char[j];
                }
            }
        } else if (byte == KH_STRING_ESCAPE_CHAR) {
            /* Escaped character */
            if (pos >= input_len) {
                goto cleanup;
            }
            
            unsigned char char_len = input[pos++];
            if (char_len == 0 || char_len > 4 || pos + char_len > input_len) {
                goto cleanup;
            }
            
            /* Check buffer bounds */
            if (out_pos + char_len >= input_len * 4) {
                goto cleanup;
            }
            
            for (int i = 0; i < char_len; i++) {
                buffer[out_pos++] = (char)input[pos++];
            }
        } else {
            /* Regular encoded character */
            if (decode_table[byte] == 0) {
                goto cleanup;
            }
            
            uint32_t codepoint = decode_table[byte];
            char utf8_char[4];
            int utf8_len = kh_codepoint_to_utf8(codepoint, utf8_char, 4);
            
            if (utf8_len <= 0) {
                goto cleanup;
            }
            
            /* Check buffer bounds */
            if (out_pos + utf8_len >= input_len * 4) {
                goto cleanup;
            }
            
            for (int i = 0; i < utf8_len; i++) {
                buffer[out_pos++] = utf8_char[i];
            }
        }
    }
    
    buffer[out_pos] = '\0';
    *output = buffer;
    result = 1;
    buffer = NULL; /* Prevent cleanup */

cleanup:
    free(buffer);
    return result;
}

/* Main string encoding function */
static inline int kh_string_encode(const char* input_str, char* output, int output_size) {
    if (!input_str || !output || output_size <= 0) return 0;
    
    char* clean_input = NULL;
    unsigned char* binary_output = NULL;
    int binary_len = 0;
    int result = 0;
    
    clean_input = (char*)malloc(strlen(input_str) + 1);
    if (!clean_input) {
        goto cleanup;
    }
    
    kh_clean_string(input_str, clean_input, (int)strlen(input_str) + 1);
    
    if (strlen(clean_input) == 0) {
        strcpy_s(output, (size_t)output_size, "");
        result = 1;
        goto cleanup;
    }
    
    if (!kh_encode_string_binary(clean_input, &binary_output, &binary_len)) {
        goto cleanup;
    }
    
    int base64_len = kh_encode_base64(binary_output, binary_len, output, output_size);
    result = (base64_len > 0) ? 1 : 0;

cleanup:
    free(clean_input);
    free(binary_output);
    return result;
}

/* Main string decoding function */
static inline int kh_string_decode(const char* input_str, char* output, int output_size) {
    if (!input_str || !output || output_size <= 0) return 0;
    
    char* clean_input = NULL;
    unsigned char* binary_data = NULL;
    char* decoded_output = NULL;
    int result = 0;
    
    clean_input = (char*)malloc(strlen(input_str) + 1);
    if (!clean_input) {
        goto cleanup;
    }
    
    kh_clean_string(input_str, clean_input, (int)strlen(input_str) + 1);
    
    if (strlen(clean_input) == 0) {
        strcpy_s(output, (size_t)output_size, "");
        result = 1;
        goto cleanup;
    }
    
    /* Decode base64 */
    binary_data = (unsigned char*)malloc(strlen(clean_input) + 1);
    if (!binary_data) {
        goto cleanup;
    }
    
    int binary_len = kh_decode_base64(clean_input, binary_data, (int)strlen(clean_input) + 1);
    if (binary_len <= 0) {
        goto cleanup;
    }
    
    if (!kh_decode_string_binary(binary_data, binary_len, &decoded_output)) {
        goto cleanup;
    }
    
    /* Copy to output buffer */
    if (strlen(decoded_output) < (size_t)output_size) {
        strcpy_s(output, (size_t)output_size, decoded_output);
        result = 1;
    }

cleanup:
    free(clean_input);
    free(binary_data);
    free(decoded_output);
    return result;
}

/* NEW STRING PROCESSING FUNCTIONS */

/* Check if string contains only numeric characters */
static inline int kh_string_is_numeric(const char* input) {
    if (!input || strlen(input) == 0) return 0;
    
    const char* ptr = input;
    int has_digit = 0;
    
    /* Skip leading whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    /* Check for optional sign */
    if (*ptr == '+' || *ptr == '-') ptr++;
    
    /* Check for digits before decimal point */
    while (*ptr && isdigit(*ptr)) {
        has_digit = 1;
        ptr++;
    }
    
    /* Check for decimal point */
    if (*ptr == '.') {
        ptr++;
        /* Check for digits after decimal point */
        while (*ptr && isdigit(*ptr)) {
            has_digit = 1;
            ptr++;
        }
    }
    
    /* Skip trailing whitespace */
    while (*ptr && isspace(*ptr)) ptr++;
    
    return (has_digit && *ptr == '\0') ? 1 : 0;
}

/* Check if string contains only alphabetic characters */
static inline int kh_string_is_alpha(const char* input) {
    if (!input || strlen(input) == 0) return 0;
    
    const char* ptr = input;
    int has_alpha = 0;
    
    while (*ptr) {
        if (isalpha(*ptr)) {
            has_alpha = 1;
        } else if (!isspace(*ptr)) {
            return 0;
        }
        ptr++;
    }
    
    return has_alpha;
}

/* Check if string contains only alphanumeric characters */
static inline int kh_string_is_alnum(const char* input) {
    if (!input || strlen(input) == 0) return 0;
    
    const char* ptr = input;
    int has_alnum = 0;
    
    while (*ptr) {
        if (isalnum(*ptr)) {
            has_alnum = 1;
        } else if (!isspace(*ptr)) {
            return 0;
        }
        ptr++;
    }
    
    return has_alnum;
}

/* Check if string contains only symbol characters */
static inline int kh_string_is_symbols(const char* input) {
    if (!input || strlen(input) == 0) return 0;
    
    const char* ptr = input;
    int has_symbol = 0;
    
    while (*ptr) {
        if (!isalnum(*ptr) && !isspace(*ptr) && *ptr != '\0') {
            has_symbol = 1;
        } else if (!isspace(*ptr)) {
            return 0;
        }
        ptr++;
    }
    
    return has_symbol;
}

/* Get UTF-8 aware string length */
static inline int kh_string_utf8_length(const char* input) {
    if (!input) return 0;
    
    const char* ptr = input;
    int length = 0;
    int input_len = (int)strlen(input);
    
    while (*ptr && (ptr - input) < input_len) {
        int char_len = kh_utf8_char_length((unsigned char)*ptr);
        if (char_len <= 0 || (ptr - input) + char_len > input_len) {
            break;
        }
        length++;
        ptr += char_len;
    }
    
    return length;
}

/* Split string by delimiter and return specific part */
static inline int kh_string_split(const char* input, const char* delimiter, int index, char* output, int output_size) {
    if (!input || !delimiter || !output || output_size <= 0 || index < 0) return 0;
    
    char* input_copy = NULL;
    char** parts = NULL;
    int part_count = 0;
    int result = 0;
    
    /* Allocate copy of input */
    size_t input_len = strlen(input);
    input_copy = (char*)malloc(input_len + 1);
    if (!input_copy) return 0;
    
    strcpy_s(input_copy, input_len + 1, input);
    
    /* Allocate array for parts */
    parts = (char**)malloc(KH_STRING_MAX_SPLIT_PARTS * sizeof(char*));
    if (!parts) {
        free(input_copy);
        return 0;
    }
    
    /* Split the string */
    char* context = NULL;
    char* token = strtok_s(input_copy, delimiter, &context);
    
    while (token && part_count < KH_STRING_MAX_SPLIT_PARTS) {
        parts[part_count++] = token;
        token = strtok_s(NULL, delimiter, &context);
    }
    
    /* Return the requested part */
    if (index < part_count) {
        size_t part_len = strlen(parts[index]);
        if (part_len < (size_t)output_size) {
            strcpy_s(output, (size_t)output_size, parts[index]);
            result = 1;
        }
    }
    
    free(parts);
    free(input_copy);
    return result;
}

/* Extract substring by start and length */
static inline int kh_string_substring(const char* input, int start, int length, char* output, int output_size) {
    if (!input || !output || output_size <= 0 || start < 0 || length < 0) return 0;
    
    int input_len = (int)strlen(input);
    if (start >= input_len) return 0;
    
    int end = start + length;
    if (end > input_len) end = input_len;
    
    int substring_len = end - start;
    if (substring_len >= output_size) substring_len = output_size - 1;
    
    /* Use UTF-8 safe slicing */
    int safe_end = kh_utf8_safe_slice_end(input, input_len, start + substring_len);
    substring_len = safe_end - start;
    
    if (substring_len > 0) {
        memcpy(output, input + start, (size_t)substring_len);
        output[substring_len] = '\0';
        return 1;
    }
    
    output[0] = '\0';
    return 1;
}

/* Trim whitespace from both ends */
static inline int kh_string_trim(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    const char* start = input;
    const char* end = input + strlen(input) - 1;
    
    /* Skip leading whitespace */
    while (*start && isspace(*start)) start++;
    
    /* Skip trailing whitespace */
    while (end > start && isspace(*end)) end--;
    
    int length = (int)(end - start + 1);
    if (length >= output_size) length = output_size - 1;
    
    if (length > 0) {
        memcpy(output, start, (size_t)length);
        output[length] = '\0';
    } else {
        output[0] = '\0';
    }
    
    return 1;
}

/* Convert string to uppercase */
static inline int kh_string_upper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    int input_len = (int)strlen(input);
    if (input_len >= output_size) input_len = output_size - 1;
    
    for (int i = 0; i < input_len; i++) {
        output[i] = (char)toupper(input[i]);
    }
    output[input_len] = '\0';
    
    return 1;
}

/* Convert string to lowercase */
static inline int kh_string_lower(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    int input_len = (int)strlen(input);
    if (input_len >= output_size) input_len = output_size - 1;
    
    for (int i = 0; i < input_len; i++) {
        output[i] = (char)tolower(input[i]);
    }
    output[input_len] = '\0';
    
    return 1;
}

/* Reverse string */
static inline int kh_string_reverse(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    int input_len = (int)strlen(input);
    if (input_len >= output_size) input_len = output_size - 1;
    
    for (int i = 0; i < input_len; i++) {
        output[i] = input[input_len - 1 - i];
    }
    output[input_len] = '\0';
    
    return 1;
}

/* Replace all occurrences of substring */
static inline int kh_string_replace(const char* input, const char* search, const char* replace, char* output, int output_size) {
    if (!input || !search || !replace || !output || output_size <= 0) return 0;
    
    int search_len = (int)strlen(search);
    int replace_len = (int)strlen(replace);
    int input_len = (int)strlen(input);
    
    if (search_len == 0) return 0;
    
    const char* current = input;
    int output_pos = 0;
    int replace_count = 0;
    
    while (*current && output_pos < output_size - 1 && replace_count < KH_STRING_MAX_REPLACE_COUNT) {
        const char* found = strstr(current, search);
        
        if (found) {
            /* Copy text before the match */
            int before_len = (int)(found - current);
            if (output_pos + before_len >= output_size - 1) {
                before_len = output_size - 1 - output_pos;
            }
            
            if (before_len > 0) {
                memcpy(output + output_pos, current, (size_t)before_len);
                output_pos += before_len;
            }
            
            /* Copy replacement text */
            if (output_pos + replace_len >= output_size - 1) {
                replace_len = output_size - 1 - output_pos;
            }
            
            if (replace_len > 0) {
                memcpy(output + output_pos, replace, (size_t)replace_len);
                output_pos += replace_len;
            }
            
            current = found + search_len;
            replace_count++;
        } else {
            /* Copy remaining text */
            int remaining_len = (int)strlen(current);
            if (output_pos + remaining_len >= output_size - 1) {
                remaining_len = output_size - 1 - output_pos;
            }
            
            if (remaining_len > 0) {
                memcpy(output + output_pos, current, (size_t)remaining_len);
                output_pos += remaining_len;
            }
            break;
        }
    }
    
    output[output_pos] = '\0';
    return 1;
}

/* Check if string contains substring */
static inline int kh_string_contains(const char* input, const char* search) {
    if (!input || !search) return 0;
    return (strstr(input, search) != NULL) ? 1 : 0;
}

/* Check if string starts with prefix */
static inline int kh_string_starts_with(const char* input, const char* prefix) {
    if (!input || !prefix) return 0;
    
    size_t input_len = strlen(input);
    size_t prefix_len = strlen(prefix);
    
    if (prefix_len > input_len) return 0;
    
    return (strncmp(input, prefix, prefix_len) == 0) ? 1 : 0;
}

/* Check if string ends with suffix */
static inline int kh_string_ends_with(const char* input, const char* suffix) {
    if (!input || !suffix) return 0;
    
    size_t input_len = strlen(input);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > input_len) return 0;
    
    return (strcmp(input + input_len - suffix_len, suffix) == 0) ? 1 : 0;
}

/* Count occurrences of substring */
static inline int kh_string_count(const char* input, const char* search) {
    if (!input || !search || strlen(search) == 0) return 0;
    
    int count = 0;
    const char* current = input;
    size_t search_len = strlen(search);
    
    while ((current = strstr(current, search)) != NULL) {
        count++;
        current += search_len;
        if (count > 100000) break; /* Prevent excessive counting */
    }
    
    return count;
}

/* Wrapper functions for boolean operations */
static inline int kh_string_is_numeric_wrapper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_is_numeric(input) ? "true" : "false");
    return 1;
}

static inline int kh_string_is_alpha_wrapper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_is_alpha(input) ? "true" : "false");
    return 1;
}

static inline int kh_string_is_alnum_wrapper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_is_alnum(input) ? "true" : "false");
    return 1;
}

static inline int kh_string_is_symbols_wrapper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_is_symbols(input) ? "true" : "false");
    return 1;
}

static inline int kh_string_utf8_length_wrapper(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%d", kh_string_utf8_length(input));
    return 1;
}

static inline int kh_string_contains_wrapper(const char* input, const char* search, char* output, int output_size) {
    if (!input || !search || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_contains(input, search) ? "true" : "false");
    return 1;
}

static inline int kh_string_starts_with_wrapper(const char* input, const char* prefix, char* output, int output_size) {
    if (!input || !prefix || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_starts_with(input, prefix) ? "true" : "false");
    return 1;
}

static inline int kh_string_ends_with_wrapper(const char* input, const char* suffix, char* output, int output_size) {
    if (!input || !suffix || !output || output_size <= 0) return 0;
    strcpy_s(output, (size_t)output_size, kh_string_ends_with(input, suffix) ? "true" : "false");
    return 1;
}

static inline int kh_string_count_wrapper(const char* input, const char* search, char* output, int output_size) {
    if (!input || !search || !output || output_size <= 0) return 0;
    _snprintf_s(output, (size_t)output_size, _TRUNCATE, "%d", kh_string_count(input, search));
    return 1;
}

/* Wrapper functions for string operations */
static inline int kh_string_split_wrapper(const char* input, const char* delimiter, const char* index_str, char* output, int output_size) {
    if (!input || !delimiter || !index_str || !output || output_size <= 0) return 0;
    
    int index = atoi(index_str);
    return kh_string_split(input, delimiter, index, output, output_size);
}

static inline int kh_string_substring_wrapper(const char* input, const char* start_str, const char* length_str, char* output, int output_size) {
    if (!input || !start_str || !length_str || !output || output_size <= 0) return 0;
    
    int start = atoi(start_str);
    int length = atoi(length_str);
    return kh_string_substring(input, start, length, output, output_size);
}

/* Whitelist of allowed string functions - SECURITY CRITICAL */
static const kh_string_function_t KH_ALLOWED_STRING_FUNCTIONS[] = {
    /* Existing encode/decode functions */
    {"encode", KH_STRING_FUNC_ENCODE_DECODE, 1, (void*)kh_string_encode, "Encode string with compression"},
    {"decode", KH_STRING_FUNC_ENCODE_DECODE, 1, (void*)kh_string_decode, "Decode compressed string"},
    
    /* Boolean check functions */
    {"isnumeric", KH_STRING_FUNC_BOOL, 1, (void*)kh_string_is_numeric_wrapper, "Check if string contains only numbers"},
    {"isalpha", KH_STRING_FUNC_BOOL, 1, (void*)kh_string_is_alpha_wrapper, "Check if string contains only letters"},
    {"isalnum", KH_STRING_FUNC_BOOL, 1, (void*)kh_string_is_alnum_wrapper, "Check if string contains only letters and numbers"},
    {"issymbols", KH_STRING_FUNC_BOOL, 1, (void*)kh_string_is_symbols_wrapper, "Check if string contains only symbols"},
    {"contains", KH_STRING_FUNC_STRING_PARAM, 2, (void*)kh_string_contains_wrapper, "Check if string contains substring"},
    {"startswith", KH_STRING_FUNC_STRING_PARAM, 2, (void*)kh_string_starts_with_wrapper, "Check if string starts with prefix"},
    {"endswith", KH_STRING_FUNC_STRING_PARAM, 2, (void*)kh_string_ends_with_wrapper, "Check if string ends with suffix"},
    
    /* Numeric functions */
    {"length", KH_STRING_FUNC_NUMBER, 1, (void*)kh_string_utf8_length_wrapper, "Get UTF-8 string length"},
    {"count", KH_STRING_FUNC_STRING_PARAM, 2, (void*)kh_string_count_wrapper, "Count occurrences of substring"},
    
    /* String transformation functions */
    {"trim", KH_STRING_FUNC_STRING, 1, (void*)kh_string_trim, "Trim whitespace from both ends"},
    {"upper", KH_STRING_FUNC_STRING, 1, (void*)kh_string_upper, "Convert to uppercase"},
    {"lower", KH_STRING_FUNC_STRING, 1, (void*)kh_string_lower, "Convert to lowercase"},
    {"reverse", KH_STRING_FUNC_STRING, 1, (void*)kh_string_reverse, "Reverse string"},
    
    /* String manipulation functions */
    {"split", KH_STRING_FUNC_STRING_2PARAM, 3, (void*)kh_string_split_wrapper, "Split string by delimiter and return part at index"},
    {"substring", KH_STRING_FUNC_STRING_2PARAM, 3, (void*)kh_string_substring_wrapper, "Extract substring by start and length"},
    {"replace", KH_STRING_FUNC_STRING_3PARAM, 3, (void*)kh_string_replace, "Replace all occurrences of substring"}
};

static const int KH_STRING_FUNCTION_COUNT = sizeof(KH_ALLOWED_STRING_FUNCTIONS) / sizeof(kh_string_function_t);

/* Find string function in whitelist - SECURITY CRITICAL */
static inline const kh_string_function_t* kh_find_string_function(const char* name) {
    if (!name) return NULL;
    
    int i;
    for (i = 0; i < KH_STRING_FUNCTION_COUNT; i++) {
        if (kh_strcasecmp(KH_ALLOWED_STRING_FUNCTIONS[i].name, name) == 0) {
            return &KH_ALLOWED_STRING_FUNCTIONS[i];
        }
    }
    return NULL;
}

/* Process string operation request */
static inline int kh_process_string_operation(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc < 2) {
        if (output && output_size > 0) {
            kh_set_error(output, output_size, "INVALID PARAMETERS");
        }
        return 1;
    }
    
    char* clean_operation = NULL;
    char** clean_args = NULL;
    const kh_string_function_t* func;
    int result = 1;
    
    clean_operation = (char*)malloc(strlen(argv[0]) + 1);
    if (!clean_operation) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        return 1;
    }
    
    kh_clean_string(argv[0], clean_operation, (int)strlen(argv[0]) + 1);
    
    /* Find function in whitelist */
    func = kh_find_string_function(clean_operation);
    if (!func) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "UNKNOWN STRING FUNCTION '%s'", clean_operation);
        goto cleanup;
    }
    
    /* Check argument count */
    if (argc - 1 != func->arg_count) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, KH_ERROR_PREFIX "FUNCTION '%s' EXPECTS %d ARGUMENTS, GOT %d", 
                 clean_operation, func->arg_count, argc - 1);
        goto cleanup;
    }
    
    /* Clean arguments */
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
        kh_clean_string(argv[i], clean_args[i - 1], (int)strlen(argv[i]) + 1);
    }
    
    /* Execute function based on type */
    switch (func->type) {
        case KH_STRING_FUNC_ENCODE_DECODE:
            if (kh_strcasecmp(clean_operation, "encode") == 0) {
                if (!kh_string_encode(clean_args[0], output, output_size)) {
                    kh_set_error(output, output_size, "ENCODING FAILED");
                } else {
                    result = 0;
                }
            } else if (kh_strcasecmp(clean_operation, "decode") == 0) {
                if (!kh_string_decode(clean_args[0], output, output_size)) {
                    kh_set_error(output, output_size, "DECODING FAILED");
                } else {
                    result = 0;
                }
            } else {
                kh_set_error(output, output_size, "UNKNOWN ENCODE/DECODE OPERATION");
            }
            break;
            
        case KH_STRING_FUNC_BOOL:
        case KH_STRING_FUNC_NUMBER:
        case KH_STRING_FUNC_STRING: {
            int (*f)(const char*, char*, int) = (int (*)(const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case KH_STRING_FUNC_STRING_PARAM: {
            int (*f)(const char*, const char*, char*, int) = (int (*)(const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case KH_STRING_FUNC_STRING_2PARAM: {
            int (*f)(const char*, const char*, const char*, char*, int) = (int (*)(const char*, const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], clean_args[2], output, output_size)) {
                result = 0;
            } else {
                kh_set_error(output, output_size, "OPERATION FAILED");
            }
            break;
        }
        
        case KH_STRING_FUNC_STRING_3PARAM: {
            int (*f)(const char*, const char*, const char*, char*, int) = (int (*)(const char*, const char*, const char*, char*, int))func->func_ptr;
            if (f(clean_args[0], clean_args[1], clean_args[2], output, output_size)) {
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
        for (int i = 0; i < argc - 1; i++) {
            free(clean_args[i]);
        }
        free(clean_args);
    }
    return result;
}

/* Cleanup function for DLL unload */
static inline void kh_cleanup_string_operations(void) {
    kh_free_string_tables();
}

#endif /* STRING_OPERATIONS_HPP */