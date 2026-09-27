#pragma once

#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004
#define SystemHandleInformationEx 64
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

using namespace std::literals::string_view_literals;

typedef NTSTATUS(NTAPI *_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

typedef NTSTATUS(NTAPI *_NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG Attributes,
    ULONG Options
);

typedef NTSTATUS(NTAPI *_NtQueryObject)(
    HANDLE ObjectHandle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
);

typedef struct _SYSTEM_HANDLE_EX {
    PVOID Object;
    HANDLE ProcessId;
    HANDLE Handle;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_EX, *PSYSTEM_HANDLE_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
    ULONG_PTR HandleCount;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef struct _OBJECT_TYPE_INFORMATION {
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    ULONG PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

// ===========================================================================
// PBO_READ - any file inside the loaded PBOs, the game's own and every mod's, read by its engine path: the PBO's
// prefix, then the path inside it - "x\kh\addons\main\ui\kh_logo_512.paa" is the file "ui\kh_logo_512.paa" of the
// PBO whose prefix is "x\kh\addons\main". The PBOs are the ones the game has open (the same handle discovery the mod
// folders come from, so the game's own Addons / DLC folders are in it with every mod's). Matching is ASCII
// case-insensitive and takes '/' for '\'. A PBO with no "prefix" in its header is addressed by its file name without
// the extension. When prefixes nest, the longest one that holds the file wins; two PBOs with the same prefix are
// asked in path order.
// The PBO list is the discovery's snapshot (taken once, at the first call of any of this class's lookups); a PBO
// the game opens later - a mission's - is seen after clear_cache(), which forgets it all (the next lookup
// rediscovers). Cost (unmeasured): the first lookup opens every loaded PBO once and reads its first entry (one 4 KB
// read each); a PBO's file table is read the first time a path inside its prefix is asked for, and kept. Call it off
// the render thread. Thread-safe (the table under pbo_mutex, the data read outside it, each read on its own handle).
// PBO_CACHE: a file's unpacked bytes, once read, are kept - a repeat read of the same path comes from memory and
// does not touch the PBO (a loaded PBO does not change while the game holds it open). Least recently used first out,
// past a budget of the cached bytes (PBO_CACHE_BUDGET by default; set_pbo_cache_budget, 0 = no caching); a file
// larger than the budget is read but not kept. read_pbo_file_shared hands out the cached bytes themselves (shared,
// read-only, no copy); read_pbo_file copies them into the caller's vector. Bytes a caller still holds outlive their
// eviction and are not counted against the budget. A failed read is not cached. clear_cache() empties it too.
// Format (the BI wiki's "PBO File Format"): little-endian; entries of { asciiz name; ulong method, original size,
// reserved, timestamp, stored size }; a first entry with an empty name and method 'Vers' carries asciiz key / value
// pairs up to an empty key ("prefix" among them); an entry with an empty name ends the table; the files' data
// follows back to back in table order. Method 'Cprs' with the two sizes different = LZSS-packed (below); 'Encr' =
// encrypted (not readable).
// ===========================================================================
class ModFolderSearcher {
private:
    static std::vector<std::filesystem::path> cached_mod_folders;
    static bool mod_folders_initialized;
    static std::mutex discovery_mutex;

    // PBO_READ
    static std::vector<std::filesystem::path> cached_pbo_files;   // Every loaded .pbo (discovery; discovery_mutex).
    static constexpr uint32_t PBO_METHOD_PACKED = 0x43707273;   // "Cprs"
    static constexpr uint32_t PBO_METHOD_PRODUCT = 0x56657273;  // "Vers"
    static constexpr uint32_t PBO_METHOD_ENCRYPTED = 0x456e6372;   // "Encr"
    struct PboEntry {
        uint64_t offset = 0;          // Of its data in the PBO file.
        uint32_t data_size = 0;       // Bytes stored.
        uint32_t original_size = 0;   // Bytes unpacked (a packed entry).
        uint32_t method = 0;
    };
    
    struct PboArchive {
        std::filesystem::path path;
        bool entries_loaded = false;   // The file table was read (successfully or not).
        std::unordered_map<std::string, PboEntry> entries;   // Normalised path inside the PBO -> entry.
    };

    static std::vector<PboArchive> pbo_archives;   // pbo_mutex; built once, never resized until clear_cache.
    static std::unordered_map<std::string, std::vector<size_t>> pbo_by_prefix;   // Normalised prefix -> archives.
    static bool pbo_index_built;
    static std::mutex pbo_mutex;
    // (the note above the class): most recently used at the front; every field under pbo_cache_mutex,
    // which is never held together with pbo_mutex or discovery_mutex.
    static constexpr size_t PBO_CACHE_BUDGET = size_t(256) << 20;   // 256 MB of cached file bytes.

    struct PboCached {
        std::string key;   // The normalised engine path.
        std::shared_ptr<const std::vector<uint8_t>> data;
    };

    static std::list<PboCached> pbo_cache_lru;
    static std::unordered_map<std::string, std::list<PboCached>::iterator> pbo_cache_map;
    static size_t pbo_cache_bytes;
    static size_t pbo_cache_budget;
    static std::mutex pbo_cache_mutex;

    // Evicts from the back until the cached bytes fit the budget (pbo_cache_mutex held).
    static void pbo_cache_trim() {
        while (pbo_cache_bytes > pbo_cache_budget && !pbo_cache_lru.empty()) {
            const PboCached& old = pbo_cache_lru.back();
            pbo_cache_bytes -= old.data->size();
            pbo_cache_map.erase(old.key);
            pbo_cache_lru.pop_back();
        }
    }

    // Lower-case ASCII, '/' -> '\', no leading or trailing '\'.
    static std::string pbo_normalise(const std::string& path) {
        std::string s;
        s.reserve(path.size());

        for (char c : path) {
            if (c == '/') c = '\\';
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
            s.push_back(c);
        }
    
        size_t a = 0, b = s.size();
        while (a < b && s[a] == '\\') ++a;
        while (b > a && s[b - 1] == '\\') --b;
        return s.substr(a, b - a);
    }

    // A path as UTF-8 (the engine's string encoding; std::filesystem::path::string() can throw on a character the
    // ANSI code page lacks).
    static std::string pbo_utf8(const std::filesystem::path& path) {
        const std::wstring w = path.wstring();
        if (w.empty()) return std::string();
        const int wn = static_cast<int>(w.size());
        const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, nullptr, 0, nullptr, nullptr);
        if (n <= 0) return std::string();
        std::string s(static_cast<size_t>(n), '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(), wn, &s[0], n, nullptr, nullptr);
        return s;
    }

    // The name to open a path by: from MAX_PATH - 12 characters on (Win32's limit for a directory; a file's is
    // MAX_PATH - 1, past which a process that is not long-path aware cannot open it), the \\?\ form - the discovery's
    // paths are absolute and backslashed, as that form needs.
    static std::wstring pbo_open_name(const std::filesystem::path& path) {
        std::wstring w = path.wstring();
        if (w.size() < MAX_PATH - 12 || w.rfind(L"\\\\?\\", 0) == 0) return w;
        if (w.rfind(L"\\\\", 0) == 0) return L"\\\\?\\UNC\\" + w.substr(2);   // \\server\share\...
        return L"\\\\?\\" + w;
    }

    // A read-only handle that lets the game keep its own open; positional reads (no shared file pointer).
    class PboFile {
    public:
        explicit PboFile(const std::filesystem::path& path) {
            const std::wstring name = pbo_open_name(path);

            handle_ = CreateFileW(name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

            LARGE_INTEGER bytes;
            if (handle_ != INVALID_HANDLE_VALUE && GetFileSizeEx(handle_, &bytes)) {
                size_ = static_cast<uint64_t>(bytes.QuadPart);
            }
        }

        ~PboFile() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
        PboFile(const PboFile&) = delete;
        PboFile& operator=(const PboFile&) = delete;
        bool ok() const { return handle_ != INVALID_HANDLE_VALUE; }
        uint64_t size() const { return size_; }

        bool read_at(uint64_t offset, void* dst, size_t count) const {
            if (offset > size_ || size_ - offset < count) return false;
            uint8_t* out = static_cast<uint8_t*>(dst);

            while (count > 0) {
                const DWORD want = static_cast<DWORD>(std::min<size_t>(count, 0x40000000u));
                OVERLAPPED ov = {};
                ov.Offset = static_cast<DWORD>(offset & 0xFFFFFFFFull);
                ov.OffsetHigh = static_cast<DWORD>(offset >> 32);
                DWORD got = 0;
                if (!ReadFile(handle_, out, want, &got, &ov) || got != want) return false;
                out += got;
                offset += got;
                count -= got;
            }

            return true;
        }
    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
        uint64_t size_ = 0;
    };

    // Sequential reads of a PBO's header, through a buffer of chunk bytes.
    struct PboHeaderReader {
        const PboFile& file;
        size_t chunk;
        uint64_t pos = 0;   // File offset of the next byte.
        std::vector<uint8_t> buf;
        uint64_t buf_at = 0;   // File offset of buf[0].
        PboHeaderReader(const PboFile& f, size_t c) : file(f), chunk(c) {}

        bool byte(uint8_t& b) {
            if (pos < buf_at || pos >= buf_at + buf.size()) {
                if (pos >= file.size()) return false;
                const size_t n = static_cast<size_t>(std::min<uint64_t>(chunk, file.size() - pos));
                buf.resize(n);
                if (!file.read_at(pos, buf.data(), n)) { buf.clear(); return false; }
                buf_at = pos;
            }
            
            b = buf[static_cast<size_t>(pos - buf_at)];
            ++pos;
            return true;
        }
        bool u32(uint32_t& v) {
            uint8_t b[4];
            for (int i = 0; i < 4; ++i) if (!byte(b[i])) return false;

            v = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
                (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);

            return true;
        }
        bool asciiz(std::string& s) {
            s.clear();
            uint8_t b;

            while (true) {
                if (!byte(b)) return false;
                if (b == 0) return true;
                s.push_back(static_cast<char>(b));
            }
        }
    };

    // The PBO's prefix (normalised; the file name without its extension when the header names none). With entries,
    // the whole file table too (normalised paths; the first of a repeated name wins). False on a header that ends
    // early. An entry whose data would run past the end of the file ends the table there: the entries before it keep
    // their offsets (each is the sum of the sizes before it), it and the rest cannot be located.
    static bool pbo_read_header(const PboFile& file, const std::filesystem::path& path, std::string& prefix,
                                std::unordered_map<std::string, PboEntry>* entries) {
        PboHeaderReader r(file, entries ? 0x40000 : 0x1000);
        prefix.clear();
        bool first = true;
        std::string name, key, value;
        std::vector<std::pair<std::string, PboEntry>> table;

        while (true) {
            PboEntry e;
            uint32_t reserved = 0, timestamp = 0;

            if (!r.asciiz(name) || !r.u32(e.method) || !r.u32(e.original_size) || !r.u32(reserved) ||
                !r.u32(timestamp) || !r.u32(e.data_size)) return false;

            if (name.empty()) {
                if (e.method != PBO_METHOD_PRODUCT) break;   // The end of the table.

                while (true) {   // The product entry's key / value pairs, up to an empty key.
                    if (!r.asciiz(key)) return false;
                    if (key.empty()) break;
                    if (!r.asciiz(value)) return false;
                    if (first && prefix.empty() && pbo_normalise(key) == "prefix") prefix = pbo_normalise(value);
                }
        
                first = false;
                continue;
            }

            if (!entries) break;   // Only the prefix was asked for: a file entry means none follows.
            first = false;
            table.emplace_back(pbo_normalise(name), e);
        }

        if (prefix.empty()) prefix = pbo_normalise(pbo_utf8(path.stem()));
        if (!entries) return true;
        uint64_t offset = r.pos;   // The data starts right after the table.

        for (auto& te : table) {
            te.second.offset = offset;
            offset += te.second.data_size;
            if (offset > file.size()) break;
            if (!te.first.empty()) entries->emplace(te.first, te.second);
        }

        return true;
    }

    // LZSS as PBOs pack it: flag bytes read lowest bit first, 1 = a literal byte, 0 = a two-byte back-reference
    // (the first byte and the second's high nibble a 12-bit distance back in a 4 KB window that starts full of
    // spaces, the low nibble the length minus 3); then a 4-byte sum of the unpacked bytes, checked.
    static bool pbo_unlzss(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_len) {
        uint8_t ring[4096];
        std::memset(ring, ' ', sizeof(ring));
        size_t r = 4096 - 18;
        size_t ip = 0, op = 0;
        uint32_t sum = 0;
    
        while (op < out_len) {
            if (ip >= in_len) return false;
            const uint8_t flags = in[ip++];

            for (int bit = 0; bit < 8 && op < out_len; ++bit) {
                if (flags & (1u << bit)) {
                    if (ip >= in_len) return false;
                    const uint8_t c = in[ip++];
                    out[op++] = c;
                    sum += c;
                    ring[r] = c;
                    r = (r + 1) & 4095;
                } else {
                    if (in_len - ip < 2) return false;
                    const size_t dist = in[ip] | (static_cast<size_t>(in[ip + 1] & 0xF0) << 4);
                    const size_t len = static_cast<size_t>(in[ip + 1] & 0x0F) + 3;
                    ip += 2;
                    if (len > out_len - op) return false;
                    size_t src = (r - dist) & 4095;

                    for (size_t k = 0; k < len; ++k) {
                        const uint8_t c = ring[src];
                        src = (src + 1) & 4095;
                        out[op++] = c;
                        sum += c;
                        ring[r] = c;
                        r = (r + 1) & 4095;
                    }
                }
            }
        }
    
        if (in_len - ip < 4) return false;

        const uint32_t stored = static_cast<uint32_t>(in[ip]) | (static_cast<uint32_t>(in[ip + 1]) << 8) |
                                (static_cast<uint32_t>(in[ip + 2]) << 16) | (static_cast<uint32_t>(in[ip + 3]) << 24);

        return stored == sum;
    }

    // Builds the prefix table once (pbo_mutex held by the caller; discovery_mutex is taken inside it and never the
    // other way round). An empty PBO list builds nothing, so a later lookup asks again: a discovery that failed
    // (it returned early or threw) runs again then, but one that completed and found no PBO is kept until
    // clear_cache().
    static void pbo_index_ensure() {
        if (pbo_index_built) return;
        const std::vector<std::filesystem::path> pbos = get_loaded_pbo_files();
        if (pbos.empty()) return;
        std::vector<PboArchive> archives;   // Built whole, then committed (a throw part-way leaves nothing built).
        std::unordered_map<std::string, std::vector<size_t>> by_prefix;
        std::string prefix;

        for (const auto& path : pbos) {
            PboFile file(path);
            if (!file.ok() || !pbo_read_header(file, path, prefix, nullptr)) continue;
            by_prefix[prefix].push_back(archives.size());
            PboArchive a;
            a.path = path;
            archives.push_back(std::move(a));
        }

        pbo_archives = std::move(archives);
        pbo_by_prefix = std::move(by_prefix);
        pbo_index_built = true;
    }

    // The PBO and entry holding a normalised engine path (pbo_mutex held by the caller). Longest prefix first.
    static bool pbo_find(const std::string& vpath, std::filesystem::path& pbo, PboEntry& entry) {
        for (size_t cut = vpath.rfind('\\'); cut != std::string::npos && cut > 0; cut = vpath.rfind('\\', cut - 1)) {
            const auto it = pbo_by_prefix.find(vpath.substr(0, cut));
            if (it == pbo_by_prefix.end()) continue;
            const std::string inner = vpath.substr(cut + 1);

            for (size_t idx : it->second) {
                PboArchive& a = pbo_archives[idx];

                if (!a.entries_loaded) {   // Committed once read whole (a header that fails keeps an empty table).
                    PboFile file(a.path);
                    if (!file.ok()) continue;   // Not opened this time: not cached, a later lookup tries again.
                    std::unordered_map<std::string, PboEntry> table;
                    std::string prefix;
                    if (!pbo_read_header(file, a.path, prefix, &table)) table.clear();
                    a.entries = std::move(table);
                    a.entries_loaded = true;
                }

                const auto e = a.entries.find(inner);

                if (e != a.entries.end()) {
                    pbo = a.path;
                    entry = e->second;
                    return true;
                }
            }
        }

        return false;
    }

    // a file read from its PBO (no cache), key its normalised path, virtual_path as asked (messages).
    static std::shared_ptr<const std::vector<uint8_t>> pbo_read_uncached(const std::string& key,
                                                                         const std::string& virtual_path,
                                                                         std::string* err) {
        auto fail = [&](const std::string& why) {
            if (err) *err = why + ": " + virtual_path;
            return std::shared_ptr<const std::vector<uint8_t>>();
        };

        std::filesystem::path pbo;
        PboEntry entry;

        try {
            std::lock_guard<std::mutex> lock(pbo_mutex);
            pbo_index_ensure();
            if (!pbo_find(key, pbo, entry)) return fail("no loaded PBO holds the file");
        } catch (const std::exception&) {   // Out of memory; nothing is left half-built.
            return fail("out of memory");
        }

        if (entry.method == PBO_METHOD_ENCRYPTED) return fail("the file is encrypted");
        if (entry.method != 0 && entry.method != PBO_METHOD_PACKED) return fail("unknown packing method");

        const bool packed = entry.method == PBO_METHOD_PACKED && entry.original_size != 0 &&
                            entry.original_size != entry.data_size;

        PboFile file(pbo);
        if (!file.ok()) return fail("cannot open " + pbo_utf8(pbo));

        try {
            std::vector<uint8_t> stored(entry.data_size);
            if (!file.read_at(entry.offset, stored.data(), stored.size())) return fail("cannot read " + pbo_utf8(pbo));
            auto out = std::make_shared<std::vector<uint8_t>>();

            if (!packed) {
                *out = std::move(stored);
            } else {
                out->resize(entry.original_size);
                if (!pbo_unlzss(stored.data(), stored.size(), out->data(), out->size())) {
                    return fail("corrupt packed data in " + pbo_utf8(pbo));
                }
            }

            if (err) err->clear();
            return out;
        } catch (const std::bad_alloc&) {
            return fail("out of memory");
        }
    }

    // Discover active mod folders by inspecting PBO file handles
    static std::vector<std::filesystem::path> discover_mod_folders() {
        std::lock_guard<std::mutex> lock(discovery_mutex);
        
        if (mod_folders_initialized) {
            return cached_mod_folders;
        }

        std::vector<std::filesystem::path> mod_folders;
        NTSTATUS status;
        ULONG handleInfoSize = 0x10000;

        // Get NT API functions from ntdll.dll
        auto NtQuerySystemInformation = reinterpret_cast<_NtQuerySystemInformation>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation"));
        auto NtDuplicateObject = reinterpret_cast<_NtDuplicateObject>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDuplicateObject"));
        auto NtQueryObject = reinterpret_cast<_NtQueryObject>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryObject"));

        if (!NtQuerySystemInformation || !NtDuplicateObject || !NtQueryObject) {
            return mod_folders;
        }

        HANDLE pid = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(GetCurrentProcessId()));
        HANDLE processHandle = GetCurrentProcess();
        struct FreeDeleter { void operator()(void* p) { free(p); } };
        
        std::unique_ptr<SYSTEM_HANDLE_INFORMATION_EX, FreeDeleter> handleInfo(
            static_cast<PSYSTEM_HANDLE_INFORMATION_EX>(malloc(handleInfoSize))
        );

        if (!handleInfo) {
            return mod_folders;
        }

        while ((status = NtQuerySystemInformation(
            SystemHandleInformationEx,
            handleInfo.get(),
            handleInfoSize,
            nullptr
        )) == STATUS_INFO_LENGTH_MISMATCH) {
            handleInfoSize *= 2;
            handleInfo.reset(static_cast<PSYSTEM_HANDLE_INFORMATION_EX>(malloc(handleInfoSize)));

            if (!handleInfo) {
                return mod_folders;
            }
        }

        if (!NT_SUCCESS(status)) {
            return mod_folders;
        }

        std::set<std::filesystem::path> unique_folders;
        std::set<std::filesystem::path> unique_pbos;

        // Iterate through all handles looking for PBO files
        for (ULONG i = 0; i < handleInfo->HandleCount; i++) {
            SYSTEM_HANDLE_EX handle = handleInfo->Handles[i];

            // Only process handles from our process
            if (handle.ProcessId != pid) {
                continue;
            }

            HANDLE dupHandle = nullptr;

            if (!NT_SUCCESS(NtDuplicateObject(
                processHandle,
                static_cast<HANDLE>(handle.Handle),
                GetCurrentProcess(),
                &dupHandle,
                0, 0, 0
            ))) {
                continue;
            }

            std::unique_ptr<void, decltype(&CloseHandle)> dupHandleGuard(dupHandle, &CloseHandle);

            if (GetFileType(dupHandle) != FILE_TYPE_DISK) {
                continue;
            }

            std::unique_ptr<OBJECT_TYPE_INFORMATION, FreeDeleter> objectTypeInfo(
                static_cast<POBJECT_TYPE_INFORMATION>(malloc(0x1000))
            );

            if (!objectTypeInfo) {
                continue;
            }

            if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectTypeInformation, objectTypeInfo.get(), 0x1000, NULL))) {
                continue;
            }

            // Skip handles with problematic access rights
            if (handle.GrantedAccess == 0x0012019f) {
                continue;
            }

            ULONG returnLength = 0;   // A failed query need not write it (read on the retry below).
            std::unique_ptr<void, FreeDeleter> objectNameInfo(malloc(0x1000));
            
            if (!objectNameInfo) {
                continue;
            }
            
            if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo.get(), 0x1000, &returnLength))) {
                objectNameInfo.reset(malloc(returnLength));
                
                if (!objectNameInfo) {
                    continue;
                }

                if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo.get(), returnLength, NULL))) {
                    continue;
                }
            }

            UNICODE_STRING objectName = *static_cast<PUNICODE_STRING>(objectNameInfo.get());

            // Check if this is a PBO file
            if (objectName.Length) {
                std::wstring_view tmp_type(objectTypeInfo->Name.Buffer);
                std::wstring_view tmp_name(objectName.Buffer);

                if (tmp_type == L"File"sv && tmp_name.find(L".pbo"sv) != std::wstring::npos) {
                    // A path that does not fit returns the size it needs (terminator included) and writes nothing.
                    std::vector<wchar_t> buffer(MAX_PATH);

                    DWORD path_len = GetFinalPathNameByHandleW(dupHandle, buffer.data(),
                                                               static_cast<DWORD>(buffer.size()), VOLUME_NAME_DOS);

                    if (path_len >= buffer.size()) {
                        buffer.resize(static_cast<size_t>(path_len) + 1);

                        path_len = GetFinalPathNameByHandleW(dupHandle, buffer.data(),
                                                             static_cast<DWORD>(buffer.size()), VOLUME_NAME_DOS);
                    }

                    if (path_len > 0 && path_len < buffer.size()) {
                        try {
                            std::filesystem::path pbo_path(buffer.data());
                            
                            // Strip \\?\ prefix if present
                            std::wstring path_str = pbo_path.wstring();

                            if (path_str.find(L"\\\\?\\UNC\\") == 0) {   // \\?\UNC\server\share -> \\server\share
                                path_str = L"\\\\" + path_str.substr(8);
                                pbo_path = path_str;
                            } else if (path_str.find(L"\\\\?\\") == 0) {
                                path_str = path_str.substr(4);
                                pbo_path = path_str;
                            }

                            // the archive itself (a name that only contains ".pbo" is not one).
                            std::wstring pbo_ext = pbo_path.extension().wstring();
                            for (auto& c : pbo_ext) if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c + 32);
                            if (pbo_ext == L".pbo") unique_pbos.insert(pbo_path);
                            
                            // Go up two levels: file.pbo -> addons -> @ModFolder
                            if (pbo_path.has_parent_path()) {
                                auto addons_folder = pbo_path.parent_path();

                                if (addons_folder.has_parent_path()) {
                                    auto mod_folder = addons_folder.parent_path();
                                    unique_folders.insert(mod_folder);
                                }
                            }
                        } catch (...) {
                            // Invalid path, skip
                        }
                    }
                }
            }
        }

        // Convert set to vector
        mod_folders.assign(unique_folders.begin(), unique_folders.end());
        cached_mod_folders = mod_folders;
        cached_pbo_files.assign(unique_pbos.begin(), unique_pbos.end());   // PBO_READ.
        mod_folders_initialized = true;
        return mod_folders;
    }

public:
    static std::vector<std::filesystem::path> get_active_mod_folders() {
        return discover_mod_folders();
    }

    static std::vector<std::filesystem::path> find_directories_in_mods(const std::string& dir_name) {
        std::vector<std::filesystem::path> found_dirs;
        auto mod_folders = discover_mod_folders();
        
        for (const auto& mod_folder : mod_folders) {
            std::filesystem::path target_path = mod_folder / dir_name;
            
            try {
                if (std::filesystem::exists(target_path) && 
                    std::filesystem::is_directory(target_path)) {
                    found_dirs.push_back(target_path);
                }
            } catch (...) {
                // Permission denied or invalid path, skip
            }
        }
        
        return found_dirs;
    }

    static std::vector<std::filesystem::path> find_files_with_extension(
        const std::vector<std::filesystem::path>& directories,
        const std::string& extension) {
        std::vector<std::filesystem::path> found_files;
        std::string lowercase_ext = extension;
        std::transform(lowercase_ext.begin(), lowercase_ext.end(), lowercase_ext.begin(), ::tolower);
        
        for (const auto& dir : directories) {
            try {
                if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                    continue;
                }
                
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (entry.is_regular_file()) {
                        std::string file_ext = entry.path().extension().string();
                        std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
                        
                        if (file_ext == lowercase_ext) {
                            found_files.push_back(entry.path());
                        }
                    }
                }
            } catch (...) {
                // Error reading directory, skip
            }
        }
        
        return found_files;
    }

    static std::filesystem::path find_first_file_with_extension(
        const std::vector<std::filesystem::path>& directories,
        const std::string& extension) {
        
        std::string lowercase_ext = extension;
        std::transform(lowercase_ext.begin(), lowercase_ext.end(), lowercase_ext.begin(), ::tolower);
        
        for (const auto& dir : directories) {
            try {
                if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                    continue;
                }
                
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (entry.is_regular_file()) {
                        std::string file_ext = entry.path().extension().string();
                        std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
                        
                        if (file_ext == lowercase_ext) {
                            return entry.path();
                        }
                    }
                }
            } catch (...) {
                // Error reading directory, skip
            }
        }
        
        return std::filesystem::path();
    }

    static std::filesystem::path find_file_by_name(
        const std::vector<std::filesystem::path>& directories,
        const std::string& filename) {
        
        for (const auto& dir : directories) {
            try {
                std::filesystem::path full_path = dir / filename;
                
                if (std::filesystem::exists(full_path) && 
                    std::filesystem::is_regular_file(full_path)) {
                    return full_path;
                }
            } catch (...) {
                // Error accessing file, skip
            }
        }
        
        return std::filesystem::path();
    }

    template<typename Predicate>

    static std::vector<std::filesystem::path> find_files(
        const std::vector<std::filesystem::path>& directories,
        Predicate predicate) {
        std::vector<std::filesystem::path> found_files;
        
        for (const auto& dir : directories) {
            try {
                if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                    continue;
                }
                
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (entry.is_regular_file() && predicate(entry.path())) {
                        found_files.push_back(entry.path());
                    }
                }
            } catch (...) {
                // Error reading directory, skip
            }
        }
        
        return found_files;
    }

    // every .pbo the game has open (the game's own and every mod's).
    static std::vector<std::filesystem::path> get_loaded_pbo_files() {
        discover_mod_folders();
        std::lock_guard<std::mutex> lock(discovery_mutex);
        return cached_pbo_files;
    }

    // whether a loaded PBO holds the file at this engine path ("prefix\path\inside.ext").
    static bool pbo_file_exists(const std::string& virtual_path) {
        try {
            std::lock_guard<std::mutex> lock(pbo_mutex);
            pbo_index_ensure();
            std::filesystem::path pbo;
            PboEntry entry;
            return pbo_find(pbo_normalise(virtual_path), pbo, entry);
        } catch (const std::exception&) {   // Out of memory; nothing is left half-built.
            return false;
        }
    }

    // the file at this engine path, unpacked - its cached bytes when a read kept them,
    // shared and read-only, with no copy. Null when no loaded PBO holds it or it cannot be read (err, when given,
    // says which).
    static std::shared_ptr<const std::vector<uint8_t>> read_pbo_file_shared(const std::string& virtual_path,
                                                                           std::string* err = nullptr) {
        std::string key;

        try {
            key = pbo_normalise(virtual_path);
            std::lock_guard<std::mutex> lock(pbo_cache_mutex);
            const auto it = pbo_cache_map.find(key);

            if (it != pbo_cache_map.end()) {
                pbo_cache_lru.splice(pbo_cache_lru.begin(), pbo_cache_lru, it->second);   // Now the most recent.
                if (err) err->clear();
                return it->second->data;
            }
        } catch (const std::exception&) {   // Out of memory.
            if (err) *err = "out of memory: " + virtual_path;
            return nullptr;
        }

        std::shared_ptr<const std::vector<uint8_t>> data = pbo_read_uncached(key, virtual_path, err);
        if (!data) return nullptr;
    
        try {   // Kept unless larger than the budget, or another thread kept it meanwhile.
            std::lock_guard<std::mutex> lock(pbo_cache_mutex);

            if (data->size() <= pbo_cache_budget) {
                const auto ins = pbo_cache_map.emplace(key, pbo_cache_lru.end());

                if (ins.second) {
                    try {
                        pbo_cache_lru.push_front(PboCached{ key, data });
                    } catch (...) {
                        pbo_cache_map.erase(ins.first);
                        throw;
                    }

                    ins.first->second = pbo_cache_lru.begin();
                    pbo_cache_bytes += data->size();
                    pbo_cache_trim();
                }
            }
        } catch (const std::exception&) {}   // Out of memory: not kept; the read stands.

        return data;
    }

    // the file at this engine path, unpacked, copied into out (from the cache when a read kept it). False
    // when no loaded PBO holds it or it cannot be read (err, when given, says which).
    static bool read_pbo_file(const std::string& virtual_path, std::vector<uint8_t>& out, std::string* err = nullptr) {
        out.clear();
        const std::shared_ptr<const std::vector<uint8_t>> data = read_pbo_file_shared(virtual_path, err);
        if (!data) return false;

        try {
            out = *data;
        } catch (const std::bad_alloc&) {
            out.clear();
            if (err) *err = "out of memory: " + virtual_path;
            return false;
        }

        return true;
    }

    // the byte budget of the cached files (0 = keep none); a smaller one evicts at once.
    static void set_pbo_cache_budget(size_t bytes) {
        std::lock_guard<std::mutex> lock(pbo_cache_mutex);
        pbo_cache_budget = bytes;
        pbo_cache_trim();
    }

    static void clear_cache() {
        {
            std::lock_guard<std::mutex> lock(discovery_mutex);
            cached_mod_folders.clear();
            cached_pbo_files.clear();
            mod_folders_initialized = false;
        }

        {
            std::lock_guard<std::mutex> lock(pbo_mutex);   // rediscovered at the next lookup.
            pbo_archives.clear();
            pbo_by_prefix.clear();
            pbo_index_built = false;
        }
        
        std::lock_guard<std::mutex> lock(pbo_cache_mutex);   // (the budget stays).
        pbo_cache_lru.clear();
        pbo_cache_map.clear();
        pbo_cache_bytes = 0;
    }

    static size_t get_mod_count() {
        return discover_mod_folders().size();
    }
};

std::vector<std::filesystem::path> ModFolderSearcher::cached_mod_folders;
bool ModFolderSearcher::mod_folders_initialized = false;
std::mutex ModFolderSearcher::discovery_mutex;
std::vector<std::filesystem::path> ModFolderSearcher::cached_pbo_files;
std::vector<ModFolderSearcher::PboArchive> ModFolderSearcher::pbo_archives;
std::unordered_map<std::string, std::vector<size_t>> ModFolderSearcher::pbo_by_prefix;
bool ModFolderSearcher::pbo_index_built = false;
std::mutex ModFolderSearcher::pbo_mutex;
std::list<ModFolderSearcher::PboCached> ModFolderSearcher::pbo_cache_lru;
std::unordered_map<std::string, std::list<ModFolderSearcher::PboCached>::iterator> ModFolderSearcher::pbo_cache_map;
size_t ModFolderSearcher::pbo_cache_bytes = 0;
size_t ModFolderSearcher::pbo_cache_budget = ModFolderSearcher::PBO_CACHE_BUDGET;
std::mutex ModFolderSearcher::pbo_cache_mutex;