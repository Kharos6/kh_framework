#ifndef COMMON_DECLARATIONS_H
#define COMMON_DECLARATIONS_H

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

#define EXTENSION_NAME "kh_framework"
#define KH_MAX_OUTPUT_SIZE 8192
#define MAX_FILE_PATH_LENGTH 255
#define MAX_TOTAL_KHDATA_SIZE_BYTES (1024LL * 1024LL * 1024LL)   /* 1GB total limit */
#define MAX_KHDATA_FILE_AUTOSAVE_THRESHOLD (1024 * 1024)         /* 1MB total limit */
#define MAX_KHDATA_FILES 1024                                    /* Maximum .khdata files allowed */
#define KH_HASH_TABLE_MIN_SIZE 16
#define KH_HASH_EMPTY 0                                          /* Empty hash table entry marker */
#define KH_FNV1A_32_OFFSET_BASIS 0x811c9dc5U
#define KH_FNV1A_32_PRIME 0x01000193U
#define KH_FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define KH_FNV1A_64_PRIME 0x100000001b3ULL
#define KH_CRC32_POLYNOMIAL 0xEDB88320U
#define KHDATA_MAGIC 0x5444484B                                  /* "KHDT" in little endian */
#define KH_ERROR_PREFIX "KH_ERROR: "
#define KH_TYPE_HASH_SIZE 64
#define KH_CRYPTO_HASH_SIZE 64
#define KH_FUNC_HASH_SIZE 64

#endif /* COMMON_DECLARATIONS_H */