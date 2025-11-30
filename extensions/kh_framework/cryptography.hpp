#pragma once

class CryptoProvider {
private:
    HCRYPTPROV hProv = 0;
    bool initialized = false;
    static CryptoProvider instance;
    
    CryptoProvider() {
        if (CryptAcquireContextW(&hProv, NULL, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, MS_ENHANCED_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            initialized = true;
        }
    }
    
public:
    ~CryptoProvider() {
        if (initialized && hProv) {
            CryptReleaseContext(hProv, 0);
        }
    }
    
    static HCRYPTPROV get() {
        return instance.hProv;
    }
    
    static bool is_initialized() {
        return instance.initialized;
    }
};

CryptoProvider CryptoProvider::instance;

class CryptoGenerator {
private:
    // Pre-computed lookup table for hex conversion
    static constexpr char hex_chars[] = "0123456789abcdef";

    template<typename T>

    static void append_hex(std::string& result, T value) {
        for (size_t i = 0; i < sizeof(T); i++) {
            uint8_t byte = (value >> (i * 8)) & 0xFF;
            result.push_back(hex_chars[byte >> 4]);
            result.push_back(hex_chars[byte & 0x0F]);
        }
    }

public:
    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }

        return ss.str();
    }

    // Windows CryptoAPI hash wrapper
    static std::string windows_hash(const std::string& input, ALG_ID algorithm) {
        HCRYPTPROV hProv = CryptoProvider::get();

        if (!hProv || !CryptoProvider::is_initialized()) {
            report_error("CryptoProvider not initialized - Windows cryptography unavailable");
            return "";
        }
            
        HCRYPTHASH hHash = 0;

        if (!CryptCreateHash(hProv, algorithm, 0, 0, &hHash)) {
            report_error("Failed to create hash context for algorithm " + std::to_string(algorithm));
            return "";
        }
        
        if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()), 
                        static_cast<DWORD>(input.length()), 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to hash data");
            return "";
        }
        
        DWORD hashSize = 0;
        DWORD sizeSize = sizeof(DWORD);

        if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashSize), &sizeSize, 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to get hash size");
            return "";
        }
        
        std::vector<uint8_t> hashData(hashSize);

        if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashSize, 0)) {
            CryptDestroyHash(hHash);
            report_error("Failed to get hash value");
            return "";
        }
        
        CryptDestroyHash(hHash);
        return bytes_to_hex(hashData);
    }

    // Hash implementations
    static std::string md5(const std::string& input) {
        return windows_hash(input, CALG_MD5);
    }

    static std::string sha1(const std::string& input) {
        return windows_hash(input, CALG_SHA1);
    }

    static std::string sha256(const std::string& input) {
        return windows_hash(input, CALG_SHA_256);
    }

    static std::string sha512(const std::string& input) {
        return windows_hash(input, CALG_SHA_512);
    }

    static std::string fnv1a32(const std::string& input) {
        uint32_t hash = 0x811c9dc5u;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
   
        while (data + 4 <= end) {
            hash ^= data[0];
            hash *= 0x01000193u;
            hash ^= data[1];
            hash *= 0x01000193u;
            hash ^= data[2];
            hash *= 0x01000193u;
            hash ^= data[3];
            hash *= 0x01000193u;
            data += 4;
        }

        while (data != end) {
            hash ^= *data++;
            hash *= 0x01000193u;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string fnv1a64(const std::string& input) {
        uint64_t hash = 0xcbf29ce484222325ull;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        while (data + 8 <= end) {
            uint64_t chunk;
            memcpy(&chunk, data, 8);
            hash ^= chunk;
            hash *= 0x100000001b3ull;
            data += 8;
        }

        while (data != end) {
            hash ^= *data++;
            hash *= 0x100000001b3ull;
        }
        
        std::string result;
        result.reserve(16);
        append_hex(result, hash);
        return result;
    }

    static std::string crc32(const std::string& input) {
        static uint32_t crc_table[256];
        static bool table_initialized = false;
        
        if (!table_initialized) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t crc = i;

                for (int j = 0; j < 8; j++) {
                    crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
                }

                crc_table[i] = crc;
            }

            table_initialized = true;
        }
        
        uint32_t crc = 0xFFFFFFFFu;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        while (data + 4 <= end) {
            crc = crc_table[(crc ^ data[0]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[1]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[2]) & 0xFF] ^ (crc >> 8);
            crc = crc_table[(crc ^ data[3]) & 0xFF] ^ (crc >> 8);
            data += 4;
        }
        
        while (data != end) {
            crc = crc_table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
        }
        
        crc ^= 0xFFFFFFFFu;
        std::string result;
        result.reserve(8);
        append_hex(result, crc);
        return result;
    }

    static std::string xxhash32(const std::string& input) {
        const uint32_t PRIME32_1 = 0x9E3779B1u;
        const uint32_t PRIME32_2 = 0x85EBCA77u;
        const uint32_t PRIME32_3 = 0xC2B2AE3Du;
        const uint32_t PRIME32_4 = 0x27D4EB2Fu;
        const uint32_t PRIME32_5 = 0x165667B1u;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        size_t len = input.length();
        const uint8_t* end = data + len;
        uint32_t seed = 0;
        uint32_t hash;
        
        if (len >= 16) {
            const uint8_t* limit = end - 16;
            uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
            uint32_t v2 = seed + PRIME32_2;
            uint32_t v3 = seed;
            uint32_t v4 = seed - PRIME32_1;
            
            do {
                // Process 16 bytes at a time (4x4 bytes)
                uint32_t k1, k2, k3, k4;
                memcpy(&k1, data, 4);
                memcpy(&k2, data + 4, 4);
                memcpy(&k3, data + 8, 4);
                memcpy(&k4, data + 12, 4);
                v1 += k1 * PRIME32_2;
                v1 = ((v1 << 13) | (v1 >> 19)) * PRIME32_1;
                v2 += k2 * PRIME32_2;
                v2 = ((v2 << 13) | (v2 >> 19)) * PRIME32_1;
                v3 += k3 * PRIME32_2;
                v3 = ((v3 << 13) | (v3 >> 19)) * PRIME32_1;
                v4 += k4 * PRIME32_2;
                v4 = ((v4 << 13) | (v4 >> 19)) * PRIME32_1;
                data += 16;
            } while (data <= limit);
            
            hash = ((v1 << 1) | (v1 >> 31)) + 
                ((v2 << 7) | (v2 >> 25)) + 
                ((v3 << 12) | (v3 >> 20)) + 
                ((v4 << 18) | (v4 >> 14));
        } else {
            hash = seed + PRIME32_5;
        }
        
        hash += static_cast<uint32_t>(len);
        
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
            uint32_t k1 = *data++;
            k1 *= PRIME32_5;
            k1 = (k1 << 11) | (k1 >> 21);
            k1 *= PRIME32_1;
            hash ^= k1;
            hash = ((hash << 15) | (hash >> 17)) * PRIME32_2 + PRIME32_3;
        }
        
        hash ^= hash >> 16;
        hash *= PRIME32_2;
        hash ^= hash >> 13;
        hash *= PRIME32_3;
        hash ^= hash >> 16;
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string adler32(const std::string& input) {
        // Use larger modulo operations less frequently
        const uint32_t MOD_ADLER = 65521;
        const size_t NMAX = 5552; // Largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
        uint32_t a = 1, b = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        size_t len = input.length();
        
        while (len > 0) {
            size_t k = len < NMAX ? len : NMAX;
            len -= k;
            
            while (k >= 16) {
                a += data[0]; b += a;
                a += data[1]; b += a;
                a += data[2]; b += a;
                a += data[3]; b += a;
                a += data[4]; b += a;
                a += data[5]; b += a;
                a += data[6]; b += a;
                a += data[7]; b += a;
                a += data[8]; b += a;
                a += data[9]; b += a;
                a += data[10]; b += a;
                a += data[11]; b += a;
                a += data[12]; b += a;
                a += data[13]; b += a;
                a += data[14]; b += a;
                a += data[15]; b += a;
                data += 16;
                k -= 16;
            }
            
            while (k-- > 0) {
                a += *data++;
                b += a;
            }
            
            a %= MOD_ADLER;
            b %= MOD_ADLER;
        }
        
        uint32_t checksum = (b << 16) | a;
        std::string result;
        result.reserve(8);
        append_hex(result, checksum);
        return result;
    }

    static std::string djb2(const std::string& input) {
        uint32_t hash = 5381;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        while (data + 4 <= end) {
            hash = ((hash << 5) + hash) + data[0];
            hash = ((hash << 5) + hash) + data[1];
            hash = ((hash << 5) + hash) + data[2];
            hash = ((hash << 5) + hash) + data[3];
            data += 4;
        }
        
        while (data != end) {
            hash = ((hash << 5) + hash) + *data++;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }

    static std::string sdbm(const std::string& input) {
        uint32_t hash = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
        const uint8_t* end = data + input.size();
        
        while (data + 4 <= end) {
            hash = data[0] + (hash << 6) + (hash << 16) - hash;
            hash = data[1] + (hash << 6) + (hash << 16) - hash;
            hash = data[2] + (hash << 6) + (hash << 16) - hash;
            hash = data[3] + (hash << 6) + (hash << 16) - hash;
            data += 4;
        }
        
        while (data != end) {
            hash = *data++ + (hash << 6) + (hash << 16) - hash;
        }
        
        std::string result;
        result.reserve(8);
        append_hex(result, hash);
        return result;
    }
};