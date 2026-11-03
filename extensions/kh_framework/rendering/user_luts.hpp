#pragma once

// The feature half (rendering_integration_features.hpp): the user .cube LUTs. Never include it on its own.
namespace RenderIntegration {
inline void user_lut_cache_release();   // User.cube LUT cache (defined with the loader).

struct UserLutEntry {
    ID3D11Texture3D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    bool failed = false;
};

static std::unordered_map<std::string, UserLutEntry> g_user_lut_cache;

inline void user_lut_cache_release() {
    for (auto& kv : g_user_lut_cache) {
        KH_SAFE_RELEASE(kv.second.srv);
        KH_SAFE_RELEASE(kv.second.tex);
    }

    g_user_lut_cache.clear();
}

// NUM_C - strtof as it reads in the "C" locale: '.' is the decimal point whatever LC_NUMERIC the process holds.
// Plain strtof follows the CRT's global locale, which the host or any module sharing the UCRT may set; under a
// comma-decimal one it reads "0.5" as 0, refusing every .cube row and fresnel(N,K) string with a fraction. The
// locale is made once per process; a null one (_create_locale failed) leaves _strtof_l on the global locale.
inline float strtof_c(const char* p, char** e) {
    static const _locale_t c = _create_locale(LC_ALL, "C");
    return _strtof_l(p, e, c);
}

// Cube text -> RGBA32F lattice (a = 1), red-fastest, domain-normalized.
inline bool parse_cube_lut(const std::string& text, std::vector<float>& out_rgba,
                           int& out_n, std::string& err) {
    out_rgba.clear();
    out_n = 0;
    int n = 0;
    float dmin[3] = { 0.0f, 0.0f, 0.0f };
    float dmax[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<float> rgb;   // Parsed triples, file order.
    size_t pos = 0;
    const size_t len = text.size();
    int line = 0;
    // CUBE_CR: a text with no '\n' breaks its lines at '\r' (CR-only line ends); any other splits at '\n', a '\r'
    // before it trimmed below. Split at '\n', such a text is one line (LUT_3D_SIZE or one row, never both) and is
    // refused whatever it holds, so the switch changes no other text's parse.
    const char br = text.find('\n') == std::string::npos ? '\r' : '\n';

    while (pos < len) {
        size_t eol = text.find(br, pos);
        if (eol == std::string::npos) eol = len;
        size_t cl_b = pos;
        size_t e = eol;
        pos = eol + 1;
        line++;
        while (cl_b < e && (text[cl_b] == ' ' || text[cl_b] == '\t')) cl_b++;
        while (e > cl_b && (text[e - 1] == '\r' || text[e - 1] == ' ' ||
                            text[e - 1] == '\t')) e--;
        if (cl_b >= e) continue;
        if (text[cl_b] == '#') continue;
        const char* s = text.c_str() + cl_b;
        const size_t sl = e - cl_b;
        // CUBE_ROW: where this line's values end. strtof skips whitespace, a line end included, so a short line
        // read its missing values from the next one - silently, as that line still gave its own three.
        const char* const le = s + sl;

        auto cl_kw = [&](const char* kw) -> bool {
            const size_t kl = strlen(kw);
            return sl >= kl && _strnicmp(s, kw, kl) == 0 &&
                   (sl == kl || s[kl] == ' ' || s[kl] == '\t');
        };

        if (cl_kw("TITLE")) continue;

        if (cl_kw("LUT_1D_SIZE")) {
            err = "1D LUTs are not supported - export a 3D LUT (.cube with LUT_3D_SIZE)";
            return false;
        }

        if (cl_kw("LUT_3D_SIZE")) {
            // CUBE_ROW: read from this line alone, as every other keyword's values (strtol skips a line end too:
            // a bare LUT_3D_SIZE read the next line's first number).
            const char* const zp = s + 11;
            char* ze = nullptr;
            n = static_cast<int>(strtol(zp, &ze, 10));

            if (ze == zp || ze > le) {
                err = "line " + std::to_string(line) + ": expected a LUT_3D_SIZE value";
                return false;
            }

            if (n < 2 || n > 128) {
                err = "LUT_3D_SIZE " + std::to_string(n) + " out of range (2..128)";
                return false;
            }

            rgb.reserve(static_cast<size_t>(n) * n * n * 3);
            continue;
        }

        // CUBE_INPUT_RANGE: DaVinci Resolve's form of the domain - LUT_3D_INPUT_RANGE min max, the input span
        // of every channel - is DOMAIN_MIN min min min / DOMAIN_MAX max max max.
        if (cl_kw("LUT_3D_INPUT_RANGE")) {
            char* p = const_cast<char*>(s + 18);
            float ir[2];

            for (int cl_c = 0; cl_c < 2; ++cl_c) {
                char* next = nullptr;
                ir[cl_c] = strtof_c(p, &next);   // NUM_C.

                if (next == p || next > le) {   // CUBE_ROW.
                    err = "line " + std::to_string(line) + ": expected two LUT_3D_INPUT_RANGE values";
                    return false;
                }

                p = next;
            }

            for (int cl_c = 0; cl_c < 3; ++cl_c) {
                dmin[cl_c] = ir[0];
                dmax[cl_c] = ir[1];
            }

            continue;
        }

        if (cl_kw("DOMAIN_MIN") || cl_kw("DOMAIN_MAX")) {
            float* d = cl_kw("DOMAIN_MIN") ? dmin : dmax;
            char* p = const_cast<char*>(s + 10);

            for (int cl_c = 0; cl_c < 3; ++cl_c) {
                char* next = nullptr;
                d[cl_c] = strtof_c(p, &next);   // NUM_C.

                if (next == p || next > le) {   // CUBE_ROW.
                    err = "line " + std::to_string(line) + ": expected three DOMAIN values";
                    return false;
                }

                p = next;
            }

            continue;
        }

        // Anything else is a data row of three floats; an unknown keyword fails the strtof progression and the error
        // names the line. CUBE_ROWS: no more rows than the largest lattice taken (128^3) - such a file is refused
        // at the end in any case (its count is not LUT_3D_SIZE^3); refused here, its rows cannot grow without bound.
        if (rgb.size() >= static_cast<size_t>(128) * 128 * 128 * 3) {
            err = "line " + std::to_string(line) + ": more data rows than a 128^3 LUT holds";
            return false;
        }
        char* p = const_cast<char*>(s);

        for (int cl_c = 0; cl_c < 3; ++cl_c) {
            char* next = nullptr;
            const float v = strtof_c(p, &next);   // NUM_C.

            if (next == p || next > le) {   // CUBE_ROW.
                err = "line " + std::to_string(line) + ": expected 'r g b' floats";
                return false;
            }

            // CUBE_FINITE: a "nan" or an overflowed (inf) value is no colour; the LUT is refused (as a bad DOMAIN).
            if (!std::isfinite(v)) {
                err = "line " + std::to_string(line) + ": non-finite lattice value";
                return false;
            }
            // CUBE_RANGE: nor is a finite value past +-65504. No grade comes near it; the pass's 2.2 decode
            // of one far past it overflows to inf (a blend against inf makes NaN), while 65504^2.2 stays finite.
            if (fabsf(v) > 65504.0f) {
                err = "line " + std::to_string(line) + ": lattice value beyond +-65504";
                return false;
            }

            rgb.push_back(v);
            p = next;
        }
    }

    if (n == 0) { err = "no LUT_3D_SIZE keyword"; return false; }
    // Every non-finite DOMAIN bound is refused: a nan, or a DOMAIN_MIN of -inf (an overflow reads as inf), would carry
    // a NaN through the remap below into an integer cell index - undefined, INT_MIN on MSVC: a read far outside the
    // table; the other infinities make no LUT either (+inf DOMAIN_MAX collapses its channel to one cell; +inf
    // DOMAIN_MIN or -inf DOMAIN_MAX is degenerate).
    for (int cl_c = 0; cl_c < 3; ++cl_c) {
        if (!std::isfinite(dmin[cl_c]) || !std::isfinite(dmax[cl_c])) {
            err = "non-finite DOMAIN_MIN / DOMAIN_MAX";
            return false;
        }
    }
    const size_t want = static_cast<size_t>(n) * n * n;

    if (rgb.size() != want * 3) {
        err = "data row count " + std::to_string(rgb.size() / 3) + " != LUT_3D_SIZE^3 (" +
              std::to_string(want) + ")";
        return false;
    }

    // The remap is per input channel (the spec's per-channel domain semantics).
    const bool identity =
        fabsf(dmin[0]) < 1e-6f && fabsf(dmin[1]) < 1e-6f && fabsf(dmin[2]) < 1e-6f &&
        fabsf(dmax[0] - 1.0f) < 1e-6f && fabsf(dmax[1] - 1.0f) < 1e-6f &&
        fabsf(dmax[2] - 1.0f) < 1e-6f;

    if (!identity) {
        for (int cl_c = 0; cl_c < 3; ++cl_c) {
            if (dmax[cl_c] - dmin[cl_c] < 1e-6f) {
                err = "degenerate DOMAIN (max <= min)";
                return false;
            }
        }

        const int hn = n;
        std::vector<float> src(std::move(rgb));
        rgb.assign(want * 3, 0.0f);

        auto at = [&](int r, int g, int b, int c) -> float {
            return src[(static_cast<size_t>(b) * hn * hn +
                        static_cast<size_t>(g) * hn + r) * 3 + c];
        };

        for (int cl_b = 0; cl_b < hn; ++cl_b)
        for (int cl_g = 0; cl_g < hn; ++cl_g)
        for (int cl_r = 0; cl_r < hn; ++cl_r) {
            const int i3[3] = { cl_r, cl_g, cl_b };
            float u[3];
            int   i0[3];
            float f[3];

            for (int cl_c = 0; cl_c < 3; ++cl_c) {
                const float x = static_cast<float>(i3[cl_c]) /
                                static_cast<float>(hn - 1);
                float v = (x - dmin[cl_c]) /
                          (dmax[cl_c] - dmin[cl_c]);
                v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
                u[cl_c] = v * static_cast<float>(hn - 1);
                i0[cl_c] = static_cast<int>(u[cl_c]);
                if (i0[cl_c] > hn - 2) i0[cl_c] = hn - 2;
                f[cl_c] = u[cl_c] - static_cast<float>(i0[cl_c]);
            }

            const size_t o = (static_cast<size_t>(cl_b) * hn * hn +
                              static_cast<size_t>(cl_g) * hn + cl_r) * 3;

            for (int cl_c = 0; cl_c < 3; ++cl_c) {
                float acc = 0.0f;

                for (int k = 0; k < 8; ++k) {
                    const int dr = k & 1, dg = (k >> 1) & 1, db = (k >> 2) & 1;
                    const float w =
                        (dr ? f[0] : 1.0f - f[0]) *
                        (dg ? f[1] : 1.0f - f[1]) *
                        (db ? f[2] : 1.0f - f[2]);
                    acc += w * at(i0[0] + dr, i0[1] + dg,
                                  i0[2] + db, cl_c);
                }

                rgb[o + cl_c] = acc;
            }
        }
    }

    // RGBA (a = 1) in texture layout: x = r fastest, exactly the file order (SysMemPitch walks r, SlicePitch walks g).
    out_rgba.resize(want * 4);

    for (size_t i = 0; i < want; ++i) {
        out_rgba[i * 4 + 0] = rgb[i * 3 + 0];
        out_rgba[i * 4 + 1] = rgb[i * 3 + 1];
        out_rgba[i * 4 + 2] = rgb[i * 3 + 2];
        out_rgba[i * 4 + 3] = 1.0f;
    }

    out_n = n;
    return true;
}

// Cached LUT SRV for a resolved user .cube path (nullptr = absent or latched failure; the caller skips the pass).
// LUT_SYNC: the read and parse run here, synchronously in the flush that first draws the pass, and again after a
// device reset or the session teardown (both drop the cache: engine_reset_release, session_teardown) - no
// worker, so a large .cube (65 or more points a side) holds that frame for a moment. Known and accepted.
inline ID3D11ShaderResourceView* user_lut_srv(ID3D11Device* dev, const std::string& path) {
    if (!dev || path.empty()) return nullptr;
    static std::string key;
    key.assign("lut|");
    key += path;
    key = kh_lower_copy(std::move(key));
    auto it = g_user_lut_cache.find(key);
    if (it != g_user_lut_cache.end()) return it->second.srv;
    UserLutEntry e;
    std::string src, err;

    // USER_SRC_MAX: a load that throws (an allocation in the parse) is a failed, latched entry like any other; let a
    // throw escape and nothing latches it, so the load runs again at every draw of the pass.
    try {
    if (user_shader_source(path, src, err)) {   // The .hlsl reader: asset_read's bytes (disk or PBO).
        std::vector<float> rgba;
        int n = 0;

        if (parse_cube_lut(src, rgba, n, err)) {
            D3D11_TEXTURE3D_DESC td = {};
            td.Width = static_cast<UINT>(n);
            td.Height = static_cast<UINT>(n);
            td.Depth = static_cast<UINT>(n);
            td.MipLevels = 1;
            td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            td.Usage = D3D11_USAGE_IMMUTABLE;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem = rgba.data();
            init.SysMemPitch = static_cast<UINT>(n) * 16;
            init.SysMemSlicePitch = static_cast<UINT>(n) * n * 16;
            HRESULT hr = dev->CreateTexture3D(&td, &init, &e.tex);

            if (SUCCEEDED(hr)) {
                hr = dev->CreateShaderResourceView(e.tex, nullptr, &e.srv);

                if (FAILED(hr)) {
                    KH_SAFE_RELEASE(e.tex);
                    err = "CreateShaderResourceView " + hr_str(hr);
                }
            } else {
                e.tex = nullptr;
                err = "CreateTexture3D " + hr_str(hr);
            }
        }
    }
    } catch (...) {
        KH_SAFE_RELEASE(e.srv);
        KH_SAFE_RELEASE(e.tex);
        err = "out of memory";
    }

    if (!e.srv) {
        e.failed = true;
        report_error_once_safe("KH RenderIntegration: user LUT '" + path + "': " + err);
    }

    g_user_lut_cache.emplace(std::move(key), e);
    return e.srv;
}

// The cache alone, for the PIP: the flush loads a LUT the first time its pass draws in the main view
// (user_lut_srv - the scene chain and flush_locked's effect-mesh route, on the render thread or on the game thread
// under a park, and flush_ui_locked's UI lane); a PIP fire before that skips the pass.
inline ID3D11ShaderResourceView* user_lut_cached(const std::string& path) {
    if (path.empty()) return nullptr;
    std::string key("lut|");
    key += path;
    key = kh_lower_copy(std::move(key));
    auto it = g_user_lut_cache.find(key);
    return it != g_user_lut_cache.end() ? it->second.srv : nullptr;
}
}