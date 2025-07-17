#ifndef CRYPTO_OPERATIONS_HPP
#define CRYPTO_OPERATIONS_HPP

#include "common_defines.hpp"
#include <windows.h>
#include <wincrypt.h>

/* Crypto algorithm constants */
#define KH_CRYPTO_MAX_INPUT_SIZE (64 * 1024 * 1024)  /* 64MB max input */
#define KH_CRYPTO_MAX_OUTPUT_SIZE 8192                /* Max hex output size */

/* FNV-1a constants */
#define KH_FNV1A_32_OFFSET_BASIS 0x811c9dc5U
#define KH_FNV1A_32_PRIME 0x01000193U
#define KH_FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define KH_FNV1A_64_PRIME 0x100000001b3ULL

/* CRC32 polynomial */
#define KH_CRC32_POLYNOMIAL 0xEDB88320U

/* Crypto function definition */
typedef struct {
    const char* name;
    int (*func_ptr)(const char* input, char* output, int output_size);
    const char* description;
} crypto_function_t;

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
    
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    unsigned char* hash_buffer = NULL;
    int result = 0;
    
    /* Get input length */
    size_t input_len = strlen(input);
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) {
        return 0;
    }
    
    /* Acquire cryptographic context */
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        goto cleanup;
    }
    
    /* Create hash object */
    if (!CryptCreateHash(hProv, algorithm, 0, 0, &hHash)) {
        goto cleanup;
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
    if (hProv) CryptReleaseContext(hProv, 0);
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
    kh_init_crc32_table();
    
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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
            /* Process 16 bytes at a time */
            uint32_t k1 = *(uint32_t*)data; data += 4;
            uint32_t k2 = *(uint32_t*)data; data += 4;
            uint32_t k3 = *(uint32_t*)data; data += 4;
            uint32_t k4 = *(uint32_t*)data; data += 4;
            
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
        uint32_t k1 = *(uint32_t*)data;
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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
    if (input_len > KH_CRYPTO_MAX_INPUT_SIZE) return 0;
    
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

/* Whitelist of allowed crypto functions - SECURITY CRITICAL */
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

/* Find crypto function in whitelist - SECURITY CRITICAL */
static inline const crypto_function_t* kh_find_crypto_function(const char* name) {
    if (!name) return NULL;
    
    int i;
    for (i = 0; i < KH_CRYPTO_FUNCTION_COUNT; i++) {
        if (kh_strcasecmp(KH_ALLOWED_CRYPTO_FUNCTIONS[i].name, name) == 0) {
            return &KH_ALLOWED_CRYPTO_FUNCTIONS[i];
        }
    }
    return NULL;
}

/* Main crypto operation processing function - enhanced with better memory management */
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
    
    if (strlen(clean_data) > KH_CRYPTO_MAX_INPUT_SIZE) {
        kh_set_error(output, output_size, "INPUT DATA TOO LARGE");
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