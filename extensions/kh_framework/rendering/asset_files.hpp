#pragma once

// The feature half (rendering_integration_features.hpp): asset discovery and file reads (RenderAssetDiscovery,
// kh_asset_read). Never include it on its own.
namespace RenderIntegration {

inline bool kh_ends_with_ci(const std::string& s, const char* suffix) {
    const size_t sl = strlen(suffix);
    if (s.size() < sl) return false;

    for (size_t i = 0; i < sl; ++i) {
        if (::tolower(static_cast<unsigned char>(s[s.size() - sl + i])) !=
            ::tolower(static_cast<unsigned char>(suffix[i]))) return false;
    }

    return true;
}

class RenderAssetDiscovery {
public:
    static std::vector<std::filesystem::path> find_all_render_directories() {
        return ModFolderSearcher::find_directories_in_mods("rendering");
    }

    static std::string find_asset_file(const std::string& request) {
        if (request.empty()) return "";
        {
            std::lock_guard<std::mutex> g(cache_mx());
            auto it = cache().find(request);
            if (it != cache().end()) return it->second;
        }
        // PBO_PATH (search_mod_folders.hpp): one leading separator names a file inside the loaded PBOs by its engine
        // path - no rendering folder, no file-name fallback. The resolved form is ONE backslash before the normalised
        // engine path, which no path the folder search below returns begins with (a drive, or "\\" on a network
        // share): kh_asset_read reads it from the PBO by that mark.
        if (ModFolderSearcher::is_pbo_path(request)) {
            const std::string khpb_key = ModFolderSearcher::normalise_pbo_path(request);
            if (khpb_key.empty() || !ModFolderSearcher::pbo_file_exists(khpb_key)) return "";
            const std::string khpb_res = "\\" + khpb_key;
            std::lock_guard<std::mutex> g(cache_mx());
            cache()[request] = khpb_res;
            return khpb_res;
        }
        std::vector<std::filesystem::path> search_paths;

        try {
            char docs_path[MAX_PATH];

            if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs_path) == S_OK) {
                search_paths.push_back(std::filesystem::path(docs_path) / "Arma 3" / "kh_framework" / "rendering");
            }
        } catch (...) {}

        // Priority 2: mod folders.
        try {
            auto mod_dirs = find_all_render_directories();
            search_paths.insert(search_paths.end(), mod_dirs.begin(), mod_dirs.end());
        } catch (...) {}

        std::string resolved;

        // Relative-path form first: root / request as given - PATH_CONFINE: only a request that names a file below
        // the folder (a drive, rooted or network path, '..', ':' or a device name touches no file here).
        for (const auto& root : search_paths) {
            try {
                const std::filesystem::path p = ModFolderSearcher::confined_join(root, request);
                if (p.empty()) break;   // The request's verdict, the same under every root.
                if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) { resolved = p.string(); break; }
            } catch (...) {}
        }

        // Bare-filename fallback: the name alone at the top of every root.
        if (resolved.empty()) {
            try {
                const std::string fname = std::filesystem::path(request).filename().string();
                auto found = ModFolderSearcher::find_file_by_name(search_paths, fname);
                if (!found.empty()) resolved = found.string();
            } catch (...) {}
        }

        if (!resolved.empty()) {
            std::lock_guard<std::mutex> g(cache_mx());
            cache()[request] = resolved;
        }

        return resolved;
    }

    // Where a request that resolved to nothing was looked for (the reports' wording).
    static std::string searched(const std::string& request) {
        if (ModFolderSearcher::is_pbo_path(request)) return "(no loaded PBO holds that engine path)";
        if (!ModFolderSearcher::is_plain_relative_path(request)) {   // PATH_CONFINE.
            return "(not a plain path below a rendering folder - a drive, rooted or network path, an empty, '.' or "
                   "'..' segment, a ':', a character Windows forbids, a trailing '.' or ' ', or a device name: "
                   "its file name alone was looked for at the top of Documents\\Arma 3\\kh_framework\\rendering "
                   "and of every mod's 'rendering' folder)";
        }
        return "(searched Documents\\Arma 3\\kh_framework\\rendering, then every mod's 'rendering' folder)";
    }

    // KH_SESSION_RESET: a file added or removed between sessions resolves anew.
    static void clear_cache() {
        std::lock_guard<std::mutex> g(cache_mx());
        cache().clear();
    }

private:
    static std::mutex& cache_mx() { static std::mutex m; return m; }
    static std::unordered_map<std::string, std::string>& cache() {
        static std::unordered_map<std::string, std::string> c;
        return c;
    }
};

// PBO_PATH: a resolved asset's bytes - find_asset_file's PBO form (one leading backslash) from the loaded PBOs
// (ModFolderSearcher::read_pbo_file_shared, whose cache keeps it), anything else from the disk. Every read of a
// resolved asset goes through here: the .fbx import, the texture loads (sync, the loader worker, the .dds and .paa
// routes) and kh_user_shader_source (.hlsl and .cube). False when it cannot be read, is empty, or holds khar_max
// bytes or more; khar_err says which ("cannot open file", "empty file", "file too large", "read failed", or the PBO
// reader's reason).
inline bool kh_asset_read(const std::string& khar_path, std::vector<uint8_t>& khar_out, uint64_t khar_max,
                          std::string* khar_err = nullptr) {
    khar_out.clear();
    auto khar_fail = [&](const std::string& khar_why) {
        khar_out.clear();
        if (khar_err) *khar_err = khar_why;
        return false;
    };

    try {
        if (ModFolderSearcher::is_pbo_path(khar_path)) {
            std::string khar_e;
            const std::shared_ptr<const std::vector<uint8_t>> khar_d =
                ModFolderSearcher::read_pbo_file_shared(khar_path, &khar_e);
            if (!khar_d) return khar_fail(khar_e);
            if (khar_d->empty()) return khar_fail("empty file");
            if (khar_d->size() >= khar_max) return khar_fail("file too large");
            khar_out.assign(khar_d->begin(), khar_d->end());
            return true;
        }

        std::ifstream khar_f(khar_path, std::ios::binary | std::ios::ate);
        if (!khar_f) return khar_fail("cannot open file");
        const std::streamsize khar_n = khar_f.tellg();
        if (khar_n <= 0) return khar_fail("empty file");
        if (static_cast<uint64_t>(khar_n) >= khar_max) return khar_fail("file too large");
        khar_out.resize(static_cast<size_t>(khar_n));
        khar_f.seekg(0);
        if (!khar_f.read(reinterpret_cast<char*>(khar_out.data()), khar_n)) return khar_fail("read failed");
        return true;
    } catch (...) {
        return khar_fail("read failed");
    }
}
}