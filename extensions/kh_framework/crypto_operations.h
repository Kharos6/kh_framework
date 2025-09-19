#ifndef CRYPTO_OPERATIONS_HPP
#define CRYPTO_OPERATIONS_HPP

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

/* Crypto function definition */
typedef struct {
    const char* name;
    int (*func_ptr)(const char* input, char* output, int output_size);
    const char* description;
} crypto_function_t;

static struct {
    HCRYPTPROV hProv;
    int initialized;
} g_crypto_cache = {0, 0};

/* Initialize crypto provider cache */
static inline int kh_init_crypto_provider(void) {
    if (g_crypto_cache.initialized) return 1;
    
    /* Try providers in order of preference */
    if (CryptAcquireContextA(&g_crypto_cache.hProv, NULL, MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        g_crypto_cache.initialized = 1;
        return 1;
    }
    
    if (CryptAcquireContextA(&g_crypto_cache.hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        g_crypto_cache.initialized = 1;
        return 1;
    }
    
    if (CryptAcquireContextA(&g_crypto_cache.hProv, NULL, MS_ENHANCED_PROV_A, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        g_crypto_cache.initialized = 1;
        return 1;
    }
    
    if (CryptAcquireContextA(&g_crypto_cache.hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        g_crypto_cache.initialized = 1;
        return 1;
    }
    
    return 0;
}

/* Cleanup crypto provider cache */
static inline void kh_cleanup_crypto_provider(void) {
    if (g_crypto_cache.initialized && g_crypto_cache.hProv) {
        CryptReleaseContext(g_crypto_cache.hProv, 0);
        g_crypto_cache.hProv = 0;
        g_crypto_cache.initialized = 0;
    }
}

/* CRC32 lookup table - generated once and reused */
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

/* Initialize CRC32 lookup table */
static inline void kh_init_crc32_table(void) {
    if (crc32_table_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ KH_CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

/* Convert binary data to hex string */
static inline int kh_bytes_to_hex(const unsigned char* bytes, int byte_count, char* hex_output, int output_size) {
    if (!bytes || !hex_output || byte_count <= 0 || output_size <= 0) return 0;
    
    int required_size = byte_count * 2 + 1;
    if (required_size > output_size) return 0;
    
    for (int i = 0; i < byte_count; i++) {
        if (_snprintf_s(hex_output + (i * 2), (size_t)(output_size - (i * 2)), _TRUNCATE, "%02x", bytes[i]) < 0) {
            return 0;
        }
    }
    
    return 1;
}

/* Generic Windows CryptoAPI hash function */
static inline int kh_crypto_windows_hash(const char* input, char* output, int output_size, ALG_ID algorithm) {
    if (!input || !output || output_size <= 0) return 0;
    
    HCRYPTHASH hHash = 0;
    unsigned char* hash_buffer = NULL;
    int result = 0;
    
    /* Get input length */
    size_t input_len = strlen(input);
    
    /* Create hash object with cached provider */
    if (!CryptCreateHash(g_crypto_cache.hProv, algorithm, 0, 0, &hHash)) {
        return 0;
    }
    
    /* Hash the data */
    if (!CryptHashData(hHash, (const BYTE*)input, (DWORD)input_len, 0)) {
        goto cleanup;
    }
    
    /* Get hash size */
    DWORD hash_size = 0;
    DWORD size_size = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hash_size, &size_size, 0)) {
        goto cleanup;
    }
    
    /* Validate hash size */
    if (hash_size == 0 || hash_size > 64) {
        goto cleanup;
    }
    
    /* Check output buffer size */
    if ((int)(hash_size * 2 + 1) > output_size) {
        goto cleanup;
    }
    
    /* Allocate buffer for hash */
    hash_buffer = (unsigned char*)malloc(hash_size);
    if (!hash_buffer) {
        goto cleanup;
    }
    
    /* Get hash value */
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash_buffer, &hash_size, 0)) {
        goto cleanup;
    }
    
    /* Convert to hex string */
    if (!kh_bytes_to_hex(hash_buffer, (int)hash_size, output, output_size)) {
        goto cleanup;
    }
    
    result = 1;

cleanup:
    free(hash_buffer);
    if (hHash) CryptDestroyHash(hHash);
    /* Don't release the cached provider */
    return result;
}

/* MD5 hash implementation */
static int kh_crypto_md5(const char* input, char* output, int output_size) {
    return kh_crypto_windows_hash(input, output, output_size, CALG_MD5);
}

/* SHA-1 hash implementation */
static int kh_crypto_sha1(const char* input, char* output, int output_size) {
    return kh_crypto_windows_hash(input, output, output_size, CALG_SHA1);
}

/* SHA-256 hash implementation */
static int kh_crypto_sha256(const char* input, char* output, int output_size) {
    return kh_crypto_windows_hash(input, output, output_size, CALG_SHA_256);
}

/* SHA-512 hash implementation */
static int kh_crypto_sha512(const char* input, char* output, int output_size) {
    return kh_crypto_windows_hash(input, output, output_size, CALG_SHA_512);
}

/* FNV-1a 32-bit hash implementation */
static int kh_crypto_fnv1a_32(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);
    uint32_t hash = KH_FNV1A_32_OFFSET_BASIS;
    
    for (size_t i = 0; i < input_len; i++) {
        hash ^= (uint32_t)((unsigned char)input[i]);
        hash *= KH_FNV1A_32_PRIME;
    }
    
    /* Convert to hex string */
    unsigned char hash_bytes[4];
    hash_bytes[0] = (unsigned char)(hash & 0xFF);
    hash_bytes[1] = (unsigned char)((hash >> 8) & 0xFF);
    hash_bytes[2] = (unsigned char)((hash >> 16) & 0xFF);
    hash_bytes[3] = (unsigned char)((hash >> 24) & 0xFF);
    
    return kh_bytes_to_hex(hash_bytes, 4, output, output_size);
}

/* FNV-1a 64-bit hash implementation */
static int kh_crypto_fnv1a_64(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);    
    uint64_t hash = KH_FNV1A_64_OFFSET_BASIS;
    
    for (size_t i = 0; i < input_len; i++) {
        hash ^= (uint64_t)((unsigned char)input[i]);
        hash *= KH_FNV1A_64_PRIME;
    }
    
    /* Convert to hex string */
    unsigned char hash_bytes[8];
    for (int i = 0; i < 8; i++) {
        hash_bytes[i] = (unsigned char)((hash >> (i * 8)) & 0xFF);
    }
    
    return kh_bytes_to_hex(hash_bytes, 8, output, output_size);
}

/* CRC32 hash implementation */
static int kh_crypto_crc32(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);    
    uint32_t crc = 0xFFFFFFFFU;
    
    for (size_t i = 0; i < input_len; i++) {
        unsigned char byte = (unsigned char)input[i];
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    crc ^= 0xFFFFFFFFU;
    
    /* Convert to hex string */
    unsigned char crc_bytes[4];
    crc_bytes[0] = (unsigned char)(crc & 0xFF);
    crc_bytes[1] = (unsigned char)((crc >> 8) & 0xFF);
    crc_bytes[2] = (unsigned char)((crc >> 16) & 0xFF);
    crc_bytes[3] = (unsigned char)((crc >> 24) & 0xFF);
    
    return kh_bytes_to_hex(crc_bytes, 4, output, output_size);
}

/* xxHash 32-bit implementation */
static int kh_crypto_xxhash32(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);

    /* xxHash constants */
    const uint32_t PRIME32_1 = 0x9E3779B1U;
    const uint32_t PRIME32_2 = 0x85EBCA77U;
    const uint32_t PRIME32_3 = 0xC2B2AE3DU;
    const uint32_t PRIME32_4 = 0x27D4EB2FU;
    const uint32_t PRIME32_5 = 0x165667B1U;
    
    uint32_t seed = 0;
    uint32_t hash;
    const unsigned char* data = (const unsigned char*)input;
    const unsigned char* end = data + input_len;
    
    if (input_len >= 16) {
        const unsigned char* limit = end - 16;
        uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
        uint32_t v2 = seed + PRIME32_2;
        uint32_t v3 = seed + 0;
        uint32_t v4 = seed - PRIME32_1;
        
        do {
            /* Read 32-bit values safely without alignment issues */
            uint32_t k1, k2, k3, k4;
            memcpy(&k1, data, 4); data += 4;
            memcpy(&k2, data, 4); data += 4;
            memcpy(&k3, data, 4); data += 4;
            memcpy(&k4, data, 4); data += 4;
            
            v1 = ((v1 + k1 * PRIME32_2) << 13) | ((v1 + k1 * PRIME32_2) >> 19);
            v1 *= PRIME32_1;
            v2 = ((v2 + k2 * PRIME32_2) << 13) | ((v2 + k2 * PRIME32_2) >> 19);
            v2 *= PRIME32_1;
            v3 = ((v3 + k3 * PRIME32_2) << 13) | ((v3 + k3 * PRIME32_2) >> 19);
            v3 *= PRIME32_1;
            v4 = ((v4 + k4 * PRIME32_2) << 13) | ((v4 + k4 * PRIME32_2) >> 19);
            v4 *= PRIME32_1;
        } while (data <= limit);
        
        hash = ((v1 << 1) | (v1 >> 31)) + ((v2 << 7) | (v2 >> 25)) + 
               ((v3 << 12) | (v3 >> 20)) + ((v4 << 18) | (v4 >> 14));
    } else {
        hash = seed + PRIME32_5;
    }
    
    hash += (uint32_t)input_len;
    
    /* Process remaining bytes */
    while (data + 4 <= end) {
        uint32_t k1;
        memcpy(&k1, data, 4);
        k1 *= PRIME32_3;
        k1 = (k1 << 17) | (k1 >> 15);
        k1 *= PRIME32_4;
        hash ^= k1;
        hash = ((hash << 19) | (hash >> 13)) * PRIME32_1 + PRIME32_4;
        data += 4;
    }
    
    while (data < end) {
        uint32_t k1 = *data;
        k1 *= PRIME32_5;
        k1 = (k1 << 11) | (k1 >> 21);
        k1 *= PRIME32_1;
        hash ^= k1;
        hash = ((hash << 15) | (hash >> 17)) * PRIME32_2 + PRIME32_3;
        data++;
    }
    
    /* Finalize */
    hash ^= hash >> 16;
    hash *= PRIME32_2;
    hash ^= hash >> 13;
    hash *= PRIME32_3;
    hash ^= hash >> 16;
    
    /* Convert to hex string */
    unsigned char hash_bytes[4];
    hash_bytes[0] = (unsigned char)(hash & 0xFF);
    hash_bytes[1] = (unsigned char)((hash >> 8) & 0xFF);
    hash_bytes[2] = (unsigned char)((hash >> 16) & 0xFF);
    hash_bytes[3] = (unsigned char)((hash >> 24) & 0xFF);
    
    return kh_bytes_to_hex(hash_bytes, 4, output, output_size);
}

/* Simple Adler-32 checksum implementation */
static int kh_crypto_adler32(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);
    uint32_t a = 1, b = 0;
    const uint32_t MOD_ADLER = 65521;
    
    for (size_t i = 0; i < input_len; i++) {
        a = (a + (unsigned char)input[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    
    uint32_t checksum = (b << 16) | a;
    
    /* Convert to hex string */
    unsigned char checksum_bytes[4];
    checksum_bytes[0] = (unsigned char)(checksum & 0xFF);
    checksum_bytes[1] = (unsigned char)((checksum >> 8) & 0xFF);
    checksum_bytes[2] = (unsigned char)((checksum >> 16) & 0xFF);
    checksum_bytes[3] = (unsigned char)((checksum >> 24) & 0xFF);
    
    return kh_bytes_to_hex(checksum_bytes, 4, output, output_size);
}

/* Simple DJB2 hash implementation */
static int kh_crypto_djb2(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);
    uint32_t hash = 5381;
    
    for (size_t i = 0; i < input_len; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)input[i];
    }
    
    /* Convert to hex string */
    unsigned char hash_bytes[4];
    hash_bytes[0] = (unsigned char)(hash & 0xFF);
    hash_bytes[1] = (unsigned char)((hash >> 8) & 0xFF);
    hash_bytes[2] = (unsigned char)((hash >> 16) & 0xFF);
    hash_bytes[3] = (unsigned char)((hash >> 24) & 0xFF);
    
    return kh_bytes_to_hex(hash_bytes, 4, output, output_size);
}

/* SDBM hash implementation */
static int kh_crypto_sdbm(const char* input, char* output, int output_size) {
    if (!input || !output || output_size <= 0) return 0;
    
    size_t input_len = strlen(input);
    uint32_t hash = 0;
    
    for (size_t i = 0; i < input_len; i++) {
        hash = (unsigned char)input[i] + (hash << 6) + (hash << 16) - hash;
    }
    
    /* Convert to hex string */
    unsigned char hash_bytes[4];
    hash_bytes[0] = (unsigned char)(hash & 0xFF);
    hash_bytes[1] = (unsigned char)((hash >> 8) & 0xFF);
    hash_bytes[2] = (unsigned char)((hash >> 16) & 0xFF);
    hash_bytes[3] = (unsigned char)((hash >> 24) & 0xFF);
    
    return kh_bytes_to_hex(hash_bytes, 4, output, output_size);
}

/* Whitelist of allowed crypto functions */
static const crypto_function_t KH_ALLOWED_CRYPTO_FUNCTIONS[] = {
    /* Standard cryptographic hashes */
    {"md5", kh_crypto_md5, "MD5 hash (128-bit)"},
    {"sha1", kh_crypto_sha1, "SHA-1 hash (160-bit)"},
    {"sha256", kh_crypto_sha256, "SHA-256 hash (256-bit)"},
    {"sha512", kh_crypto_sha512, "SHA-512 hash (512-bit)"},
    
    /* Fast non-cryptographic hashes */
    {"fnv1a32", kh_crypto_fnv1a_32, "FNV-1a 32-bit hash"},
    {"fnv1a64", kh_crypto_fnv1a_64, "FNV-1a 64-bit hash"},
    {"crc32", kh_crypto_crc32, "CRC32 checksum"},
    {"xxhash32", kh_crypto_xxhash32, "xxHash 32-bit"},
    {"adler32", kh_crypto_adler32, "Adler-32 checksum"},
    {"djb2", kh_crypto_djb2, "DJB2 hash"},
    {"sdbm", kh_crypto_sdbm, "SDBM hash"}
};

static const int KH_CRYPTO_FUNCTION_COUNT = sizeof(KH_ALLOWED_CRYPTO_FUNCTIONS) / sizeof(crypto_function_t);

typedef struct {
    uint32_t hash;
    const crypto_function_t* func;
} crypto_hash_entry_t;

static crypto_hash_entry_t g_crypto_hash_table[KH_CRYPTO_HASH_SIZE];
static int g_crypto_hash_initialized = 0;

/* Initialize crypto hash table */
static inline void kh_init_crypto_hash_table(void) {
    if (g_crypto_hash_initialized) return;
    
    memset(g_crypto_hash_table, 0, sizeof(g_crypto_hash_table));
    
    for (int i = 0; i < KH_CRYPTO_FUNCTION_COUNT; i++) {
        /* Hash lowercase name */
        char lower_name[32];
        const char* src = KH_ALLOWED_CRYPTO_FUNCTIONS[i].name;
        int j = 0;
        while (src[j] && j < 31) {
            lower_name[j] = (src[j] >= 'A' && src[j] <= 'Z') ? src[j] + 32 : src[j];
            j++;
        }
        lower_name[j] = '\0';
        
        uint32_t hash = kh_hash_name_case_insensitive(lower_name);
        uint32_t index = hash & (KH_CRYPTO_HASH_SIZE - 1);
        
        /* Linear probing for collisions */
        while (g_crypto_hash_table[index].hash != 0) {
            index = (index + 1) & (KH_CRYPTO_HASH_SIZE - 1);
        }
        
        g_crypto_hash_table[index].hash = hash;
        g_crypto_hash_table[index].func = &KH_ALLOWED_CRYPTO_FUNCTIONS[i];
    }
    
    g_crypto_hash_initialized = 1;
}

/* Find crypto function in whitelist */
static inline const crypto_function_t* kh_find_crypto_function(const char* name) {
    if (!name) return NULL;
    
    /* Convert to lowercase for comparison */
    char lower_name[32];
    int i = 0;
    while (name[i] && i < 31) {
        lower_name[i] = (name[i] >= 'A' && name[i] <= 'Z') ? name[i] + 32 : name[i];
        i++;
    }
    lower_name[i] = '\0';
    
    /* Hash lookup */
    uint32_t hash = kh_hash_name_case_insensitive(lower_name);
    uint32_t index = hash & (KH_CRYPTO_HASH_SIZE - 1);
    
    /* Linear probe to find match */
    for (int probe = 0; probe < KH_CRYPTO_HASH_SIZE; probe++) {
        if (g_crypto_hash_table[index].hash == 0) {
            break; /* Empty slot, not found */
        }
        if (g_crypto_hash_table[index].hash == hash) {
            /* Verify name matches */
            const char* func_name = g_crypto_hash_table[index].func->name;
            if (strcmp(func_name, lower_name) == 0) {
                return g_crypto_hash_table[index].func;
            }
        }
        index = (index + 1) & (KH_CRYPTO_HASH_SIZE - 1);
    }
    
    return NULL;
}

/* Main crypto operation processing function */
static int kh_process_crypto_operation(char* output, int output_size, const char** argv, int argc) {
    if (!output || output_size <= 0 || !argv || argc != 2) {
        if (output && output_size > 0) {
            if (argc != 2) {
                kh_set_error(output, output_size, "CRYPTO OPERATION REQUIRES 2 ARGUMENTS");
            } else {
                kh_set_error(output, output_size, "INVALID PARAMETERS");
            }
        }
        return 1;
    }
    
    char* clean_type = NULL;
    char* clean_data = NULL;
    const crypto_function_t* func;
    int result = 1;
    
    /* Allocate memory for cleaned inputs */
    clean_type = (char*)malloc(strlen(argv[0]) + 1);
    clean_data = (char*)malloc(strlen(argv[1]) + 1);
    
    if (!clean_type || !clean_data) {
        kh_set_error(output, output_size, "MEMORY ALLOCATION FAILED");
        goto cleanup;
    }
    
    /* Clean inputs */
    kh_clean_string(argv[0], clean_type, (int)strlen(argv[0]) + 1);
    kh_clean_string(argv[1], clean_data, (int)strlen(argv[1]) + 1);
    
    /* Validate inputs */
    if (strlen(clean_type) == 0) {
        kh_set_error(output, output_size, "EMPTY CRYPTO TYPE");
        goto cleanup;
    }
    
    /* Find crypto function */
    func = kh_find_crypto_function(clean_type);
    if (!func) {
        _snprintf_s(output, (size_t)output_size, _TRUNCATE, 
                   KH_ERROR_PREFIX "UNKNOWN CRYPTO TYPE '%s' - SUPPORTED: md5, sha1, sha256, sha512, fnv1a32, fnv1a64, crc32, xxhash32, adler32, djb2, sdbm", 
                   clean_type);
        goto cleanup;
    }
    
    /* Execute crypto function */
    if (!func->func_ptr(clean_data, output, output_size)) {
        kh_set_error(output, output_size, "CRYPTO OPERATION FAILED");
        goto cleanup;
    }
    
    result = 0;

cleanup:
    free(clean_type);
    free(clean_data);
    return result;
}

#endif /* CRYPTO_OPERATIONS_HPP */