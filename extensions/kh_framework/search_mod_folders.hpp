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

class ModFolderSearcher {
private:
    static std::vector<std::filesystem::path> cached_mod_folders;
    static bool mod_folders_initialized;
    static std::mutex discovery_mutex;

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

            ULONG returnLength;
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
                    wchar_t buffer[MAX_PATH];

                    if (GetFinalPathNameByHandleW(dupHandle, buffer, sizeof(buffer), VOLUME_NAME_DOS) > 0) {
                        try {
                            std::filesystem::path pbo_path(buffer);
                            
                            // Strip \\?\ prefix if present
                            std::wstring path_str = pbo_path.wstring();

                            if (path_str.find(L"\\\\?\\") == 0) {
                                path_str = path_str.substr(4);
                                pbo_path = path_str;
                            }
                            
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

    static void clear_cache() {
        std::lock_guard<std::mutex> lock(discovery_mutex);
        cached_mod_folders.clear();
        mod_folders_initialized = false;
    }

    static size_t get_mod_count() {
        return discover_mod_folders().size();
    }
};

std::vector<std::filesystem::path> ModFolderSearcher::cached_mod_folders;
bool ModFolderSearcher::mod_folders_initialized = false;
std::mutex ModFolderSearcher::discovery_mutex;