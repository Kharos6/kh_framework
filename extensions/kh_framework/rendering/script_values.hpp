#pragma once

// The feature half (rendering_integration_features.hpp): the SQF value parsers (blend, mesh, shape, effect ids,
// colours, durations). Never include it on its own.
namespace RenderIntegration {

// A script number as an int: false when it is not finite or lies outside int's range (either conversion is
// undefined behaviour); INT_MIN itself is refused too (the bound is open), so the caller refuses it as it refuses any
// other out-of-range value. The fraction truncates.
inline bool kh_gv_int(const game_value& khgi_v, int& khgi_out) {
    const float khgi_f = static_cast<float>(khgi_v);
    if (!(khgi_f > -2147483648.0f && khgi_f < 2147483648.0f)) return false;   // NaN fails both.
    khgi_out = static_cast<int>(khgi_f);
    return true;
}

inline void kh_sanitize_color(float c[4]) {
    for (int i = 0; i < 4; ++i) if (!(c[i] == c[i])) c[i] = 1.0f;
    for (int i = 0; i < 3; ++i) {
        if (c[i] < 0.0f) c[i] = 0.0f;
        if (c[i] > 65504.0f) c[i] = 65504.0f;
    }
    if (c[3] < 0.0f) c[3] = 0.0f;
    if (c[3] > 1.0f) c[3] = 1.0f;
}

inline int blend_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = -1;
        if (!kh_gv_int(gv, id)) return -1;
        return (id >= 0 && id <= 5) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    s = kh_lower_copy(std::move(s));
    if (s == "normal")   return 0;
    if (s == "additive") return 1;
    if (s == "multiply") return 2;
    if (s == "screen")   return 3;
    if (s == "lighten")  return 4;
    if (s == "darken")   return 5;
    return -1;
}

// Maps a mesh designator to a live registry id: a string name / alias
// (case-insensitive) or a scalar id. Returns -1 for anything unknown.
inline int mesh_id_from_gv(const game_value& gv) {
    const int n = static_cast<int>(mesh_count());   // Published count (mesh_count's contract).

    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = -1;
        return kh_gv_int(gv, id) && kh_mesh_alive(id) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    s = kh_lower_copy(std::move(s));

    for (int i = 0; i < n; ++i) {
        if (!kh_mesh_alive(i)) continue;
        const MeshDef& md = mesh_def(i);
        if (s == md.name || (!md.alias.empty() && s == md.alias)) return i;
    }

    return -1;
}

// Maps a mask-shape designator to 0 (sphere / ellipsoid) / 1 (cube / box): a
// string (case-insensitive) or a scalar 0 / 1. Returns -1 for anything unknown.
inline int shape_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = -1;
        if (!kh_gv_int(gv, id)) return -1;
        return (id == 0 || id == 1) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    s = kh_lower_copy(std::move(s));
    if (s == "sphere" || s == "ellipsoid") return 0;
    if (s == "cube" || s == "box")         return 1;
    return -1;
}

// Reads a scalar (uniform) or [x, y, z] array into out[3]. Returns false on a
// malformed value (KH_ARG_EXACT: an array of any other length included).
inline bool read_vec3_or_uniform(const game_value& gv, float out[3]) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        const float v = static_cast<float>(gv);
        out[0] = v; out[1] = v; out[2] = v;
        return true;
    }

    if (gv.type_enum() == game_data_type::ARRAY) {
        auto& a = gv.to_array();
        if (a.size() != 3 ||   // KH_ARG_EXACT: contract 2 (sqf) - a fourth element is an error, not ignored.
            a[0].type_enum() != game_data_type::SCALAR ||
            a[1].type_enum() != game_data_type::SCALAR ||
            a[2].type_enum() != game_data_type::SCALAR) return false;
        out[0] = static_cast<float>(a[0]);
        out[1] = static_cast<float>(a[1]);
        out[2] = static_cast<float>(a[2]);
        return true;
    }

    return false;
}

inline int effect_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = -1;
        if (!kh_gv_int(gv, id)) return -1;
        // 18..20 = clarity/deband/rainlens; 21 = crt; 22 = ssgi; 23 =
        // fogscatter; 31 = dynamiclightfog (KH_DLF). 24 - 30 are internal side ids (EffectId's note: SSGI's 24 - 26,
        // 27 retired, KH_FX_SIDE's 28 - 30).
        return (id >= 0 && id <= KH_MAX_EFFECT && !(id >= 24 && id <= 30)) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    s = kh_lower_copy(std::move(s));
    if (s == "solid")      return 0;
    if (s == "invert")     return 1;
    if (s == "colorgrade") return 2;
    if (s == "vignette")   return 3;
    if (s == "chromatic")  return 4;
    if (s == "grain")      return 5;
    if (s == "sharpen")    return 6;
    if (s == "blur")       return 7;
    if (s == "bloom")      return 8;
    if (s == "distortion") return 9;
    if (s == "outline")    return 10;
    if (s == "pulse")      return 11;
    if (s == "halation")   return 12;
    if (s == "fog")        return 13;
    if (s == "lensflare")  return 14;
    if (s == "anamorphic") return 15;
    if (s == "sunflare")   return 16;
    if (s == "glitch")     return 17;
    if (s == "clarity")    return 18;
    if (s == "deband")     return 19;
    if (s == "rainlens" || s == "rain") return 20;
    if (s == "crt")        return 21;
    if (s == "ssgi")       return 22;
    if (s == "fogscatter") return 23;
    if (s == "dynamiclightfog") return 31;   // KH_DLF.
    return -1;
}

inline int kh_effect_from_gv(const game_value& gv, std::string& out_hlsl, std::string& err) {
    out_hlsl.clear();
    err.clear();

    if (gv.type_enum() == game_data_type::STRING) {
        const std::string khfx_s = static_cast<std::string>(gv);
        const bool khfx_is_hlsl = kh_ends_with_ci(khfx_s, ".hlsl");
        const bool khfx_is_cube = !khfx_is_hlsl && kh_ends_with_ci(khfx_s, ".cube");

        if (khfx_is_hlsl || khfx_is_cube) {
            const std::string khfx_resolved = RenderAssetDiscovery::find_asset_file(khfx_s);

            if (khfx_resolved.empty()) {
                err = "'" + khfx_s + "' not found " + RenderAssetDiscovery::searched(khfx_s);
                return -1;
            }

            out_hlsl = khfx_resolved;
            return khfx_is_cube ? KH_EFFECT_LUT : KH_EFFECT_CUSTOM;
        }
    }

    return effect_id_from_gv(gv);
}

// Parses a duration designator into the object's lifetime fields: scalar seconds -> hold that long, no fades; array
// [fadeIn, hold, fadeOut]. 0 / all-zero = infinite (default). Returns false on a malformed value.
inline bool parse_duration_gv(const game_value& gv, RenderObject& obj) {
    float fi = 0.0f, hd = 0.0f, fo = 0.0f;

    if (gv.type_enum() == game_data_type::SCALAR) {
        hd = static_cast<float>(gv);
    } else if (gv.type_enum() == game_data_type::ARRAY) {
        auto& a = gv.to_array();
        if (a.size() > 3) return false;   // KH_ARG_EXACT: contract 2 (sqf) - a fourth element is an error.
        if ((a.size() > 0 && a[0].type_enum() != game_data_type::SCALAR) ||
            (a.size() > 1 && a[1].type_enum() != game_data_type::SCALAR) ||
            (a.size() > 2 && a[2].type_enum() != game_data_type::SCALAR)) return false;
        if (a.size() >= 1) fi = static_cast<float>(a[0]);
        if (a.size() >= 2) hd = static_cast<float>(a[1]);
        if (a.size() >= 3) fo = static_cast<float>(a[2]);
    } else {
        return false;
    }

    // KH_DURATION_NAN: a part that is not a number is refused - the clamps below keep a NaN, and the sum's test
    // would then make the object permanent without a word. An infinite part is accepted.
    if (fi != fi || hd != hd || fo != fo) return false;
    fi = fi < 0.0f ? 0.0f : fi;
    hd = hd < 0.0f ? 0.0f : hd;
    fo = fo < 0.0f ? 0.0f : fo;
    obj.fade_in = fi;
    obj.hold_dur = hd;
    obj.fade_out = fo;
    obj.timed = (fi + hd + fo) > 0.0f;
    return true;
}
}