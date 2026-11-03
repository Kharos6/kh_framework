#pragma once

// The feature half (rendering_integration_features.hpp): the user .cube LUTs. Never include it on its own.
namespace RenderIntegration {
inline void kh_user_lut_cache_release();   // User.cube LUT cache (defined with the loader).

struct KhUserLutEntry {
    ID3D11Texture3D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    bool failed = false;
};

static std::unordered_map<std::string, KhUserLutEntry> g_user_lut_cache;

inline void kh_user_lut_cache_release() {
    for (auto& khlc_kv : g_user_lut_cache) {
        KH_SAFE_RELEASE(khlc_kv.second.srv);
        KH_SAFE_RELEASE(khlc_kv.second.tex);
    }

    g_user_lut_cache.clear();
}

// KH_NUM_C - strtof as it reads in the "C" locale: '.' is the decimal point whatever LC_NUMERIC the process holds.
// Plain strtof follows the CRT's global locale, which the host or any module sharing the UCRT may set; under a
// comma-decimal one it reads "0.5" as 0, refusing every .cube row and fresnel(N,K) string with a fraction. The
// locale is made once per process; a null one (_create_locale failed) leaves _strtof_l on the global locale.
inline float kh_strtof_c(const char* khsc_p, char** khsc_e) {
    static const _locale_t khsc_c = _create_locale(LC_ALL, "C");
    return _strtof_l(khsc_p, khsc_e, khsc_c);
}

// Cube text -> RGBA32F lattice (a = 1), red-fastest, domain-normalized.
inline bool kh_parse_cube_lut(const std::string& text, std::vector<float>& out_rgba,
                              int& out_n, std::string& err) {
    out_rgba.clear();
    out_n = 0;
    int khcl_n = 0;
    float khcl_dmin[3] = { 0.0f, 0.0f, 0.0f };
    float khcl_dmax[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<float> khcl_rgb;   // Parsed triples, file order.
    size_t khcl_pos = 0;
    const size_t khcl_len = text.size();
    int khcl_line = 0;
    // KH_CUBE_CR: a text with no '\n' breaks its lines at '\r' (CR-only line ends); any other splits at '\n', a '\r'
    // before it trimmed below. Split at '\n', such a text is one line (LUT_3D_SIZE or one row, never both) and is
    // refused whatever it holds, so the switch changes no other text's parse.
    const char khcl_br = text.find('\n') == std::string::npos ? '\r' : '\n';

    while (khcl_pos < khcl_len) {
        size_t khcl_eol = text.find(khcl_br, khcl_pos);
        if (khcl_eol == std::string::npos) khcl_eol = khcl_len;
        size_t khcl_b = khcl_pos;
        size_t khcl_e = khcl_eol;
        khcl_pos = khcl_eol + 1;
        khcl_line++;
        while (khcl_b < khcl_e && (text[khcl_b] == ' ' || text[khcl_b] == '\t')) khcl_b++;
        while (khcl_e > khcl_b && (text[khcl_e - 1] == '\r' || text[khcl_e - 1] == ' ' ||
                                   text[khcl_e - 1] == '\t')) khcl_e--;
        if (khcl_b >= khcl_e) continue;
        if (text[khcl_b] == '#') continue;
        const char* khcl_s = text.c_str() + khcl_b;
        const size_t khcl_sl = khcl_e - khcl_b;
        // KH_CUBE_ROW: where this line's values end. strtof skips whitespace, a line end included, so a short line
        // read its missing values from the next one - silently, as that line still gave its own three.
        const char* const khcl_le = khcl_s + khcl_sl;

        auto khcl_kw = [&](const char* kw) -> bool {
            const size_t kl = strlen(kw);
            return khcl_sl >= kl && _strnicmp(khcl_s, kw, kl) == 0 &&
                   (khcl_sl == kl || khcl_s[kl] == ' ' || khcl_s[kl] == '\t');
        };

        if (khcl_kw("TITLE")) continue;

        if (khcl_kw("LUT_1D_SIZE")) {
            err = "1D LUTs are not supported - export a 3D LUT (.cube with LUT_3D_SIZE)";
            return false;
        }

        if (khcl_kw("LUT_3D_SIZE")) {
            // KH_CUBE_ROW: read from this line alone, as every other keyword's values (strtol skips a line end too:
            // a bare LUT_3D_SIZE read the next line's first number).
            const char* const khcl_zp = khcl_s + 11;
            char* khcl_ze = nullptr;
            khcl_n = static_cast<int>(strtol(khcl_zp, &khcl_ze, 10));

            if (khcl_ze == khcl_zp || khcl_ze > khcl_le) {
                err = "line " + std::to_string(khcl_line) + ": expected a LUT_3D_SIZE value";
                return false;
            }

            if (khcl_n < 2 || khcl_n > 128) {
                err = "LUT_3D_SIZE " + std::to_string(khcl_n) + " out of range (2..128)";
                return false;
            }

            khcl_rgb.reserve(static_cast<size_t>(khcl_n) * khcl_n * khcl_n * 3);
            continue;
        }

        // KH_CUBE_INPUT_RANGE: DaVinci Resolve's form of the domain - LUT_3D_INPUT_RANGE min max, the input span
        // of every channel - is DOMAIN_MIN min min min / DOMAIN_MAX max max max.
        if (khcl_kw("LUT_3D_INPUT_RANGE")) {
            char* khcl_p = const_cast<char*>(khcl_s + 18);
            float khcl_ir[2];

            for (int khcl_c = 0; khcl_c < 2; ++khcl_c) {
                char* khcl_next = nullptr;
                khcl_ir[khcl_c] = kh_strtof_c(khcl_p, &khcl_next);   // KH_NUM_C.

                if (khcl_next == khcl_p || khcl_next > khcl_le) {   // KH_CUBE_ROW.
                    err = "line " + std::to_string(khcl_line) + ": expected two LUT_3D_INPUT_RANGE values";
                    return false;
                }

                khcl_p = khcl_next;
            }

            for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
                khcl_dmin[khcl_c] = khcl_ir[0];
                khcl_dmax[khcl_c] = khcl_ir[1];
            }

            continue;
        }

        if (khcl_kw("DOMAIN_MIN") || khcl_kw("DOMAIN_MAX")) {
            float* khcl_d = khcl_kw("DOMAIN_MIN") ? khcl_dmin : khcl_dmax;
            char* khcl_p = const_cast<char*>(khcl_s + 10);

            for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
                char* khcl_next = nullptr;
                khcl_d[khcl_c] = kh_strtof_c(khcl_p, &khcl_next);   // KH_NUM_C.

                if (khcl_next == khcl_p || khcl_next > khcl_le) {   // KH_CUBE_ROW.
                    err = "line " + std::to_string(khcl_line) + ": expected three DOMAIN values";
                    return false;
                }

                khcl_p = khcl_next;
            }

            continue;
        }

        // Anything else is a data row of three floats; an unknown keyword fails the strtof progression and the error
        // names the line. KH_CUBE_ROWS: no more rows than the largest lattice taken (128^3) - such a file is refused
        // at the end in any case (its count is not LUT_3D_SIZE^3); refused here, its rows cannot grow without bound.
        if (khcl_rgb.size() >= static_cast<size_t>(128) * 128 * 128 * 3) {
            err = "line " + std::to_string(khcl_line) + ": more data rows than a 128^3 LUT holds";
            return false;
        }
        char* khcl_p = const_cast<char*>(khcl_s);

        for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
            char* khcl_next = nullptr;
            const float khcl_v = kh_strtof_c(khcl_p, &khcl_next);   // KH_NUM_C.

            if (khcl_next == khcl_p || khcl_next > khcl_le) {   // KH_CUBE_ROW.
                err = "line " + std::to_string(khcl_line) + ": expected 'r g b' floats";
                return false;
            }

            // KH_CUBE_FINITE: a "nan" or an overflowed (inf) value is no colour; the LUT is refused (as a bad DOMAIN).
            if (!std::isfinite(khcl_v)) {
                err = "line " + std::to_string(khcl_line) + ": non-finite lattice value";
                return false;
            }
            // KH_CUBE_RANGE: nor is a finite value past +-65504. No grade comes near it; the pass's 2.2 decode
            // of one far past it overflows to inf (a blend against inf makes NaN), while 65504^2.2 stays finite.
            if (fabsf(khcl_v) > 65504.0f) {
                err = "line " + std::to_string(khcl_line) + ": lattice value beyond +-65504";
                return false;
            }

            khcl_rgb.push_back(khcl_v);
            khcl_p = khcl_next;
        }
    }

    if (khcl_n == 0) { err = "no LUT_3D_SIZE keyword"; return false; }
    // Every non-finite DOMAIN bound is refused: a nan, or a DOMAIN_MIN of -inf (an overflow reads as inf), would carry
    // a NaN through the remap below into an integer cell index - undefined, INT_MIN on MSVC: a read far outside the
    // table; the other infinities make no LUT either (+inf DOMAIN_MAX collapses its channel to one cell; +inf
    // DOMAIN_MIN or -inf DOMAIN_MAX is degenerate).
    for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
        if (!std::isfinite(khcl_dmin[khcl_c]) || !std::isfinite(khcl_dmax[khcl_c])) {
            err = "non-finite DOMAIN_MIN / DOMAIN_MAX";
            return false;
        }
    }
    const size_t khcl_want = static_cast<size_t>(khcl_n) * khcl_n * khcl_n;

    if (khcl_rgb.size() != khcl_want * 3) {
        err = "data row count " + std::to_string(khcl_rgb.size() / 3) + " != LUT_3D_SIZE^3 (" +
              std::to_string(khcl_want) + ")";
        return false;
    }

    // The remap is per input channel (the spec's per-channel domain semantics).
    const bool khcl_identity =
        fabsf(khcl_dmin[0]) < 1e-6f && fabsf(khcl_dmin[1]) < 1e-6f && fabsf(khcl_dmin[2]) < 1e-6f &&
        fabsf(khcl_dmax[0] - 1.0f) < 1e-6f && fabsf(khcl_dmax[1] - 1.0f) < 1e-6f &&
        fabsf(khcl_dmax[2] - 1.0f) < 1e-6f;

    if (!khcl_identity) {
        for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
            if (khcl_dmax[khcl_c] - khcl_dmin[khcl_c] < 1e-6f) {
                err = "degenerate DOMAIN (max <= min)";
                return false;
            }
        }

        const int khcl_hn = khcl_n;
        std::vector<float> khcl_src(std::move(khcl_rgb));
        khcl_rgb.assign(khcl_want * 3, 0.0f);

        auto khcl_at = [&](int r, int g, int b, int c) -> float {
            return khcl_src[(static_cast<size_t>(b) * khcl_hn * khcl_hn +
                             static_cast<size_t>(g) * khcl_hn + r) * 3 + c];
        };

        for (int khcl_b = 0; khcl_b < khcl_hn; ++khcl_b)
        for (int khcl_g = 0; khcl_g < khcl_hn; ++khcl_g)
        for (int khcl_r = 0; khcl_r < khcl_hn; ++khcl_r) {
            const int khcl_i3[3] = { khcl_r, khcl_g, khcl_b };
            float khcl_u[3];
            int   khcl_i0[3];
            float khcl_f[3];

            for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
                const float khcl_x = static_cast<float>(khcl_i3[khcl_c]) /
                                     static_cast<float>(khcl_hn - 1);
                float khcl_v = (khcl_x - khcl_dmin[khcl_c]) /
                               (khcl_dmax[khcl_c] - khcl_dmin[khcl_c]);
                khcl_v = khcl_v < 0.0f ? 0.0f : (khcl_v > 1.0f ? 1.0f : khcl_v);
                khcl_u[khcl_c] = khcl_v * static_cast<float>(khcl_hn - 1);
                khcl_i0[khcl_c] = static_cast<int>(khcl_u[khcl_c]);
                if (khcl_i0[khcl_c] > khcl_hn - 2) khcl_i0[khcl_c] = khcl_hn - 2;
                khcl_f[khcl_c] = khcl_u[khcl_c] - static_cast<float>(khcl_i0[khcl_c]);
            }

            const size_t khcl_o = (static_cast<size_t>(khcl_b) * khcl_hn * khcl_hn +
                                   static_cast<size_t>(khcl_g) * khcl_hn + khcl_r) * 3;

            for (int khcl_c = 0; khcl_c < 3; ++khcl_c) {
                float khcl_acc = 0.0f;

                for (int khcl_k = 0; khcl_k < 8; ++khcl_k) {
                    const int khcl_dr = khcl_k & 1, khcl_dg = (khcl_k >> 1) & 1, khcl_db = (khcl_k >> 2) & 1;
                    const float khcl_w =
                        (khcl_dr ? khcl_f[0] : 1.0f - khcl_f[0]) *
                        (khcl_dg ? khcl_f[1] : 1.0f - khcl_f[1]) *
                        (khcl_db ? khcl_f[2] : 1.0f - khcl_f[2]);
                    khcl_acc += khcl_w * khcl_at(khcl_i0[0] + khcl_dr, khcl_i0[1] + khcl_dg,
                                                 khcl_i0[2] + khcl_db, khcl_c);
                }

                khcl_rgb[khcl_o + khcl_c] = khcl_acc;
            }
        }
    }

    // RGBA (a = 1) in texture layout: x = r fastest, exactly the file order (SysMemPitch walks r, SlicePitch walks g).
    out_rgba.resize(khcl_want * 4);

    for (size_t khcl_i = 0; khcl_i < khcl_want; ++khcl_i) {
        out_rgba[khcl_i * 4 + 0] = khcl_rgb[khcl_i * 3 + 0];
        out_rgba[khcl_i * 4 + 1] = khcl_rgb[khcl_i * 3 + 1];
        out_rgba[khcl_i * 4 + 2] = khcl_rgb[khcl_i * 3 + 2];
        out_rgba[khcl_i * 4 + 3] = 1.0f;
    }

    out_n = khcl_n;
    return true;
}

// Cached LUT SRV for a resolved user .cube path (nullptr = absent or latched failure; the caller skips the pass).
// KH_LUT_SYNC: the read and parse run here, synchronously in the flush that first draws the pass, and again after a
// device reset or the session teardown (both drop the cache: kh_engine_reset_release, kh_session_teardown) - no
// worker, so a large .cube (65 or more points a side) holds that frame for a moment. Known and accepted.
inline ID3D11ShaderResourceView* kh_user_lut_srv(ID3D11Device* dev, const std::string& path) {
    if (!dev || path.empty()) return nullptr;
    static std::string khlu_key;
    khlu_key.assign("lut|");
    khlu_key += path;
    khlu_key = kh_lower_copy(std::move(khlu_key));
    auto khlu_it = g_user_lut_cache.find(khlu_key);
    if (khlu_it != g_user_lut_cache.end()) return khlu_it->second.srv;
    KhUserLutEntry khlu_e;
    std::string khlu_src, khlu_err;

    // KH_USER_SRC_MAX: a load that throws (an allocation in the parse) is a failed, latched entry like any other; let a
    // throw escape and nothing latches it, so the load runs again at every draw of the pass.
    try {
    if (kh_user_shader_source(path, khlu_src, khlu_err)) {   // The .hlsl reader: kh_asset_read's bytes (disk or PBO).
        std::vector<float> khlu_rgba;
        int khlu_n = 0;

        if (kh_parse_cube_lut(khlu_src, khlu_rgba, khlu_n, khlu_err)) {
            D3D11_TEXTURE3D_DESC khlu_td = {};
            khlu_td.Width = static_cast<UINT>(khlu_n);
            khlu_td.Height = static_cast<UINT>(khlu_n);
            khlu_td.Depth = static_cast<UINT>(khlu_n);
            khlu_td.MipLevels = 1;
            khlu_td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            khlu_td.Usage = D3D11_USAGE_IMMUTABLE;
            khlu_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA khlu_init = {};
            khlu_init.pSysMem = khlu_rgba.data();
            khlu_init.SysMemPitch = static_cast<UINT>(khlu_n) * 16;
            khlu_init.SysMemSlicePitch = static_cast<UINT>(khlu_n) * khlu_n * 16;
            HRESULT khlu_hr = dev->CreateTexture3D(&khlu_td, &khlu_init, &khlu_e.tex);

            if (SUCCEEDED(khlu_hr)) {
                khlu_hr = dev->CreateShaderResourceView(khlu_e.tex, nullptr, &khlu_e.srv);

                if (FAILED(khlu_hr)) {
                    KH_SAFE_RELEASE(khlu_e.tex);
                    khlu_err = "CreateShaderResourceView " + hr_str(khlu_hr);
                }
            } else {
                khlu_e.tex = nullptr;
                khlu_err = "CreateTexture3D " + hr_str(khlu_hr);
            }
        }
    }
    } catch (...) {
        KH_SAFE_RELEASE(khlu_e.srv);
        KH_SAFE_RELEASE(khlu_e.tex);
        khlu_err = "out of memory";
    }

    if (!khlu_e.srv) {
        khlu_e.failed = true;
        report_error_once_safe("KH RenderIntegration: user LUT '" + path + "': " + khlu_err);
    }

    g_user_lut_cache.emplace(std::move(khlu_key), khlu_e);
    return khlu_e.srv;
}

// The cache alone, for the PIP: the flush loads a LUT the first time its pass draws in the main view
// (kh_user_lut_srv - the scene chain and flush_locked's effect-mesh route, on the render thread or on the game thread
// under a park, and flush_ui_locked's UI lane); a PIP fire before that skips the pass.
inline ID3D11ShaderResourceView* kh_user_lut_cached(const std::string& path) {
    if (path.empty()) return nullptr;
    std::string khlc_key("lut|");
    khlc_key += path;
    khlc_key = kh_lower_copy(std::move(khlc_key));
    auto khlc_it = g_user_lut_cache.find(khlc_key);
    return khlc_it != g_user_lut_cache.end() ? khlc_it->second.srv : nullptr;
}
}