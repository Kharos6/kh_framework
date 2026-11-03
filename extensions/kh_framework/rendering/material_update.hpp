#pragma once

// The feature half (rendering_integration_features.hpp): the updateRender3D "material" parser
// (apply_material_update). Never include it on its own.
namespace RenderIntegration {

// updateRender3D "material" parser. A nil textures or params list, and a nil third element of a texture entry
// (routing / colour), are left out and keep their defaults; "" or "*" as the selector names every submesh. Any other
// value of the wrong type, nil included, is a hard error, never a silent zero-cast.
// MAT_KEEP: an entry replaces its submeshes' whole material; a submesh no entry names keeps its own, so [] changes
// nothing and no command removes a material (only a new object starts without one). The set survives a mesh change
// by submesh index ("mesh", sqf_integration.hpp); a submesh past the old set draws with none.

inline bool apply_material_update(RenderObject& obj, const game_value& val, std::string& err) {
    if (val.type_enum() != game_data_type::ARRAY) { err = "value must be an array of material entries"; return false; }
    // Staged lookup (see apply_native_size): the submesh table must be the object's own mesh's, published or not.
    const MeshDef* sd = mesh_def_staged(obj.mesh);
    const MeshDef& md = sd ? *sd : mesh_def(0);

    // Copy-on-write base: the existing set (if any), sized to this mesh.
    MaterialSet next;
    if (obj.materials) next = *obj.materials;
    next.slots.resize(md.submeshes.size());

    auto lower = [](std::string s) { s = kh_lower_copy(std::move(s)); return s; };
    // The map slot a texture name takes, by shader kind: the arma model names its maps as Arma does and keeps AS /
    // SMDI in the orm / specular slots (Material::maps). USER_SLOTS: a .hlsl material takes both namings -
    // every slot there is (the name then says what slots 2 and 4 hold).
    auto slot_id = [&](const std::string& s, bool arma, bool um_user) -> int {
        if (s == "diffuse" || s == "albedo") return 0;
        if (s == "normal") return 1;
        if (s == "emissive") return 3;
        if (arma || um_user) {
            if (s == "as") return 2;
            if (s == "smdi") return 4;
            if (s == "speccolor") return 5;
        }
        if (!arma) {
            if (s == "orm") return 2;
            if (s == "specular") return 4;
        }
        return -1;
    };
    auto chan_id = [](const std::string& s) -> int {
        if (s == "r") return 0;
        if (s == "g") return 1;
        if (s == "b") return 2;
        if (s == "a") return 3;
        return -1;
    };

    auto& entries = val.to_array();

    for (size_t ei = 0; ei < entries.size(); ++ei) {
        if (entries[ei].type_enum() != game_data_type::ARRAY) { err = "material entry must be an array"; return false; }
        auto& ent = entries[ei].to_array();
        if (ent.size() < 2) { err = "material entry needs [selector, shader, ...]"; return false; }

        std::vector<size_t> targets;

        if (ent[0].type_enum() == game_data_type::SCALAR) {
            int si = 0;
            if (!gv_int(ent[0], si)) { err = "submesh index must be a finite number in int range"; return false; }
            if (si == -1) { for (size_t s = 0; s < md.submeshes.size(); ++s) targets.push_back(s); }
            else if (si >= 0 && si < static_cast<int>(md.submeshes.size())) targets.push_back(static_cast<size_t>(si));
            else { err = "submesh index " + std::to_string(si) + " out of range (mesh has " + std::to_string(md.submeshes.size()) + ")"; return false; }
        } else if (ent[0].type_enum() == game_data_type::STRING) {
            const std::string want = lower(static_cast<std::string>(ent[0]));
            if (want == "*" || want.empty()) { for (size_t s = 0; s < md.submeshes.size(); ++s) targets.push_back(s); }
            else {
                for (size_t s = 0; s < md.submeshes.size(); ++s) {
                    if (lower(md.submeshes[s].name) == want) targets.push_back(s);
                }
                if (targets.empty()) {
                    err = "no submesh named '" + want + "' (";
                    for (size_t s = 0; s < md.submeshes.size(); ++s) {
                        if (s) err += ", ";
                        err += "'" + md.submeshes[s].name + "'";
                    }
                    err += ")";
                    return false;
                }
            }
        } else { err = "selector must be a submesh name, index, or -1/\"*\""; return false; }

        if (ent[1].type_enum() != game_data_type::STRING) { err = "shader must be a string (\"pbr\", \"arma\" or a \".hlsl\" path)"; return false; }
        const std::string sh_raw = static_cast<std::string>(ent[1]);
        std::string sh_path;
        const bool arma = lower(sh_raw) == "arma";

        if (ends_with_ci(sh_raw, ".hlsl")) {
            sh_path = RenderAssetDiscovery::find_asset_file(sh_raw);

            if (sh_path.empty()) {
                err = "shader '" + sh_raw + "' not found " + RenderAssetDiscovery::searched(sh_raw);
                return false;
            }
        } else if (lower(sh_raw) != "pbr" && !arma) {
            err = "unknown shader '" + sh_raw + "' (\"pbr\", \"arma\" or a \".hlsl\" path)";
            return false;
        }
        const bool um_user = !sh_path.empty();   // USER_SLOTS: every slot, route and param.
        const char* const slots = arma ? "diffuse|normal|as|smdi|speccolor|emissive"
                                : um_user ? "diffuse|normal|orm|as|emissive|specular|smdi|speccolor"
                                                 : "diffuse|normal|orm|emissive|specular";

        Material mat;
        mat.used = true;
        mat.shader = !sh_path.empty() ? 1 : (arma ? 2 : 0);
        mat.user_shader = sh_path;
        mat.user_vertex = um_user && user_vertex_scan(sh_path);   // KH_USER_VS: the file defines KhUserVertex.

        if (ent.size() > 2 && !ent[2].is_nil()) {
            if (ent[2].type_enum() != game_data_type::ARRAY) { err = "textures must be an array of [path, slot, routing?] entries"; return false; }
            auto& texes = ent[2].to_array();

            for (size_t ti = 0; ti < texes.size(); ++ti) {
                if (texes[ti].type_enum() != game_data_type::ARRAY) { err = "texture entry must be [path, slot, routing?]"; return false; }
                auto& te = texes[ti].to_array();
                if (te.size() < 2 || te[0].type_enum() != game_data_type::STRING ||
                    te[1].type_enum() != game_data_type::STRING) {
                    err = "texture entry must be [path, slot, routing?]";
                    return false;
                }
                const std::string tpath = static_cast<std::string>(te[0]);
                const std::string name = lower(static_cast<std::string>(te[1]));
                int slot = slot_id(name, arma, um_user);
                // USER_TEX: user0..user5, read only by a .hlsl material's
                // own code (KhUserTex); no builtin input can be routed to one.
                const bool ut_user = name.size() == 5 && name.compare(0, 4, "user") == 0 &&
                                     name[4] >= '0' && name[4] < '0' + MAT_USER_MAPS;
                if (ut_user) {
                    if (sh_path.empty()) {
                        err = "texture slot '" + name + "' is for a .hlsl material only";
                        return false;
                    }
                    slot = MAT_MAPS_BUILTIN + (name[4] - '0');
                }
                if (slot < 0) {
                    err = "unknown texture slot '" + static_cast<std::string>(te[1]) + "' for " +
                          (arma ? "an arma" : "this") + " material (" + slots +
                          (sh_path.empty() ? "" : "|user0..user5") + ")";
                    return false;
                }
                if (!(ends_with_ci(tpath, ".png") || ends_with_ci(tpath, ".jpg") ||
                      ends_with_ci(tpath, ".jpeg") || ends_with_ci(tpath, ".tga") ||
                      ends_with_ci(tpath, ".bmp") || ends_with_ci(tpath, ".dds") ||
                      paa_ext(tpath))) {   // PAA.
                    err = "texture '" + tpath + "': unsupported extension (png|jpg|jpeg|tga|bmp|dds|paa|pac)";
                    return false;
                }
                const std::string resolved = RenderAssetDiscovery::find_asset_file(tpath);
                if (resolved.empty()) {
                    err = "texture '" + tpath + "' not found " + RenderAssetDiscovery::searched(tpath);
                    return false;
                }
                if (um_user && (slot == 2 || slot == 4)) {   // USER_SLOTS: one slot, two namings - the name says what it holds.
                    const bool an = name == "as" || name == "smdi";
                    bool& held = slot == 2 ? mat.map_as : mat.map_smdi;
                    if (!mat.maps[slot].path.empty() && held != an) {
                        err = slot == 2 ? "texture slots 'orm' and 'as' are one slot: give one of them"
                                        : "texture slots 'specular' and 'smdi' are one slot: give one of them";
                        return false;
                    }
                    held = an;
                }
                mat.maps[slot].path = resolved;
                mat.maps[slot].srgb = false;

                if (ut_user) {   // USER_TEX: the third element is the colour space.
                    if (te.size() > 2 && !te[2].is_nil()) {
                        const std::string cs = te[2].type_enum() == game_data_type::STRING
                                             ? lower(static_cast<std::string>(te[2])) : std::string();
                        if (cs != "srgb" && cs != "linear") {
                            err = "a user texture slot takes \"srgb\" or \"linear\" (the default) as its third element";
                            return false;
                        }
                        mat.maps[slot].srgb = cs == "srgb";
                    }
                    continue;
                }

                if (te.size() > 2 && !te[2].is_nil()) {
                    if (te[2].type_enum() != game_data_type::ARRAY) { err = "routing must be an array of [input, channel] entries"; return false; }
                    auto& routes = te[2].to_array();

                    for (size_t ri = 0; ri < routes.size(); ++ri) {
                        if (routes[ri].type_enum() != game_data_type::ARRAY) { err = "routing entry must be [input, channel]"; return false; }
                        auto& re = routes[ri].to_array();
                        if (re.size() < 2 || re[0].type_enum() != game_data_type::STRING ||
                            re[1].type_enum() != game_data_type::STRING) {
                            err = "routing entry must be [input, channel]";
                            return false;
                        }
                        const std::string input = lower(static_cast<std::string>(re[0]));
                        const int chan = chan_id(lower(static_cast<std::string>(re[1])));
                        if (chan < 0) { err = "routing channel must be r|g|b|a"; return false; }
                        const int route = slot * 4 + chan;
                        if      (input == "occlusion")  mat.route_occ = route;
                        else if (input == "alpha")      mat.route_alpha = route;
                        else if (input == "gloss")      mat.route_gloss = route;
                        else if ((arma || um_user) && input == "specular") mat.route_spec = route;
                        else if (!arma && input == "roughness") mat.route_rough = route;
                        else if (!arma && (input == "metallic" || input == "metalness")) mat.route_metal = route;
                        else {
                            err = "unknown routing input '" + input + "' (" +
                                  (arma ? "occlusion|gloss|specular|alpha"
                                   : um_user ? "occlusion|roughness|metallic|alpha|gloss|specular"
                                               : "occlusion|roughness|metallic|alpha|gloss") + ")";
                            return false;
                        }
                    }
                }
            }
        }

        if (ent.size() > 3 && !ent[3].is_nil()) {
            if (ent[3].type_enum() != game_data_type::ARRAY) { err = "params must be an array of [key, value] pairs"; return false; }
            auto& params = ent[3].to_array();
            // MAT_FINITE: every number here is checked finite (fresnel and vertexBound too); a refused one leaves
            // the object's material as it was (the set is built on a copy and interned only at the end).
            // UV_ANIM: nums reads a number, or an array of lo .. hi numbers, into out (a lone
            // number fills all hi elements when one allows it); false = refused (wrong shape, or a non-finite
            // number).
            auto nums = [](auto& v, size_t lo, size_t hi, float* out, bool one) -> bool {
                if (one && v.type_enum() == game_data_type::SCALAR) {
                    const float ua_f = static_cast<float>(v);
                    if (!std::isfinite(ua_f)) return false;
                    for (size_t k = 0; k < hi; ++k) out[k] = ua_f;
                    return true;
                }
                if (v.type_enum() != game_data_type::ARRAY) return false;
                auto& a = v.to_array();
                if (a.size() < lo || a.size() > hi) return false;
                for (size_t k = 0; k < a.size(); ++k) {
                    if (a[k].type_enum() != game_data_type::SCALAR) return false;
                    out[k] = static_cast<float>(a[k]);
                    if (!std::isfinite(out[k])) return false;
                }
                return true;
            };

            for (size_t pi = 0; pi < params.size(); ++pi) {
                if (params[pi].type_enum() != game_data_type::ARRAY) { err = "param entry must be [key, value]"; return false; }
                auto& pe = params[pi].to_array();
                if (pe.size() < 2 || pe[0].type_enum() != game_data_type::STRING) { err = "param entry must be [key, value]"; return false; }
                const std::string key = lower(static_cast<std::string>(pe[0]));

                if (key == "basecolor" || key == "color") {
                    if (pe[1].type_enum() != game_data_type::ARRAY) { err = "basecolor must be [r, g, b] numbers"; return false; }
                    auto& c = pe[1].to_array();
                    if ((c.size() > 0 && c[0].type_enum() != game_data_type::SCALAR) ||
                        (c.size() > 1 && c[1].type_enum() != game_data_type::SCALAR) ||
                        (c.size() > 2 && c[2].type_enum() != game_data_type::SCALAR)) { err = "basecolor must be [r, g, b] numbers"; return false; }
                    for (size_t k = 0; k < 3 && k < c.size(); ++k) {
                        mat.base_color[k] = static_cast<float>(c[k]);
                        if (!std::isfinite(mat.base_color[k])) {
                            err = "basecolor must be [r, g, b] finite numbers";
                            return false;
                        }
                    }
                } else if (!arma && key == "roughness") {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "roughness must be a number"; return false; }
                    mat.roughness = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.roughness)) { err = "roughness must be a finite number"; return false; }
                } else if (!arma && (key == "metalness" || key == "metallic")) {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "metalness must be a number"; return false; }
                    mat.metalness = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.metalness)) { err = "metalness must be a finite number"; return false; }
                } else if ((arma || um_user) && (key == "glossiness" || key == "gloss")) {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "glossiness must be a number"; return false; }
                    mat.glossiness = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.glossiness)) { err = "glossiness must be a finite number"; return false; }
                } else if ((arma || um_user) && (key == "specularcolor" || key == "speccolor")) {
                    if (pe[1].type_enum() != game_data_type::ARRAY) { err = "specularColor must be [r, g, b] numbers"; return false; }
                    auto& c = pe[1].to_array();
                    if ((c.size() > 0 && c[0].type_enum() != game_data_type::SCALAR) ||
                        (c.size() > 1 && c[1].type_enum() != game_data_type::SCALAR) ||
                        (c.size() > 2 && c[2].type_enum() != game_data_type::SCALAR)) { err = "specularColor must be [r, g, b] numbers"; return false; }
                    for (size_t k = 0; k < 3 && k < c.size(); ++k) {
                        mat.spec_color[k] = static_cast<float>(c[k]);
                        if (!std::isfinite(mat.spec_color[k])) {
                            err = "specularColor must be [r, g, b] finite numbers";
                            return false;
                        }
                    }
                } else if ((arma || um_user) && key == "fresnel") {
                    // [N, K], or Arma's own procedural texture string with a fresnel(N,K) in it, pasted from an rvmat.
                    float nk[2] = { 0.0f, 0.0f };
                    bool ok = false;
                    if (pe[1].type_enum() == game_data_type::ARRAY) {
                        auto& f = pe[1].to_array();
                        ok = f.size() == 2 && f[0].type_enum() == game_data_type::SCALAR &&
                             f[1].type_enum() == game_data_type::SCALAR;
                        if (ok) { nk[0] = static_cast<float>(f[0]); nk[1] = static_cast<float>(f[1]); }
                    } else if (pe[1].type_enum() == game_data_type::STRING) {
                        const std::string fs = lower(static_cast<std::string>(pe[1]));
                        const size_t at = fs.find("fresnel(");
                        if (at != std::string::npos) {
                            const char* p = fs.c_str() + at + 8;
                            char* e = nullptr;
                            nk[0] = strtof_c(p, &e);   // NUM_C.
                            if (e != p) {
                                while (*e == ' ') ++e;
                                if (*e == ',') {
                                    const char* q = e + 1;
                                    nk[1] = strtof_c(q, &e);   // NUM_C.
                                    while (e != q && *e == ' ') ++e;
                                    ok = e != q && *e == ')';
                                }
                            }
                        }
                    }
                    if (!ok) { err = "fresnel must be [N, K] or a string containing fresnel(N,K)"; return false; }
                    if (!(nk[0] >= 0.0f && nk[0] < 1.0e4f && nk[1] >= 0.0f && nk[1] < 1.0e4f)) {
                        err = "fresnel N and K must be finite numbers >= 0";
                        return false;
                    }
                    mat.fresnel_n = nk[0];
                    mat.fresnel_k = nk[1];
                } else if (um_user && key == "vertexbound") {   // KH_USER_VS: how far KhUserVertex moves any vertex (m).
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "vertexBound must be a number (metres)"; return false; }
                    const float vb = static_cast<float>(pe[1]);
                    if (!(vb >= 0.0f && vb < 1.0e6f)) { err = "vertexBound must be a finite number >= 0 (metres)"; return false; }
                    mat.vertex_bound = vb;
                } else if (um_user && key == "vertexstatic") {   // KH_USER_VS: the result moves only when its inputs do.
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "vertexStatic must be a number (0 or 1)"; return false; }
                    const float vs = static_cast<float>(pe[1]);
                    if (!std::isfinite(vs)) { err = "vertexStatic must be finite (0 or 1)"; return false; }
                    mat.vertex_static = vs != 0.0f;
                } else if (key == "uvscale") {   // UV_ANIM (every kind).
                    float ua_s[2] = { 1.0f, 1.0f };
                    if (!nums(pe[1], 2, 2, ua_s, true) ||
                        !(fabsf(ua_s[0]) >= 1.0e-4f && fabsf(ua_s[0]) <= 1.0e4f) ||
                        !(fabsf(ua_s[1]) >= 1.0e-4f && fabsf(ua_s[1]) <= 1.0e4f)) {
                        err = "uvScale must be a number or [u, v], each 0.0001 to 10000 in size (negative mirrors)";
                        return false;
                    }
                    mat.uv.scale[0] = ua_s[0];
                    mat.uv.scale[1] = ua_s[1];
                } else if (key == "uvoffset") {
                    float o[2] = { 0.0f, 0.0f };
                    if (!nums(pe[1], 2, 2, o, false) || !(fabsf(o[0]) <= 1.0e6f) ||
                        !(fabsf(o[1]) <= 1.0e6f)) {
                        err = "uvOffset must be [u, v] finite numbers";
                        return false;
                    }
                    mat.uv.offset[0] = o[0];
                    mat.uv.offset[1] = o[1];
                } else if (key == "uvscroll") {
                    float ua_c[2] = { 0.0f, 0.0f };
                    if (!nums(pe[1], 2, 2, ua_c, false) || !(fabsf(ua_c[0]) <= 1.0e4f) ||
                        !(fabsf(ua_c[1]) <= 1.0e4f)) {
                        err = "uvScroll must be [u, v] (uv units per second), each at most 10000 in size";
                        return false;
                    }
                    mat.uv.scroll[0] = ua_c[0];
                    mat.uv.scroll[1] = ua_c[1];
                } else if (key == "uvrotation" || key == "uvrotationspeed") {
                    float r = 0.0f;
                    if (pe[1].type_enum() != game_data_type::SCALAR || !nums(pe[1], 1, 1, &r, true) ||
                        !(fabsf(r) <= 1.0e6f)) {
                        err = key == "uvrotation" ? "uvRotation must be a finite number (degrees)"
                                                  : "uvRotationSpeed must be a finite number (degrees per second)";
                        return false;
                    }
                    (key == "uvrotation" ? mat.uv.rot : mat.uv.rot_speed) = r;
                } else if (key == "uvwarp") {
                    float w[3] = { 0.0f, 0.0f, 0.0f };
                    if (!nums(pe[1], 3, 3, w, false) || !(w[0] >= 0.0f && w[0] <= 1.0f) ||
                        !(w[1] >= 0.0f && w[1] <= 1.0e3f) || !(fabsf(w[2]) <= 1.0e3f)) {
                        err = "uvWarp must be [amplitude 0 - 1, frequency 0 - 1000, speed (cycles per second)]";
                        return false;
                    }
                    for (int k = 0; k < 3; ++k) mat.uv.warp[k] = w[k];
                } else if (key == "flipbook") {
                    float ua_f[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    const bool ok = nums(pe[1], 3, 4, ua_f, false);
                    const bool cr = ok && ua_f[0] >= 1.0f && ua_f[0] <= 256.0f &&
                                    ua_f[1] >= 1.0f && ua_f[1] <= 256.0f &&
                                    floorf(ua_f[0]) == ua_f[0] && floorf(ua_f[1]) == ua_f[1];
                    const float n = cr ? ua_f[0] * ua_f[1] : 0.0f;
                    if (ok && pe[1].to_array().size() < 4) ua_f[3] = n;
                    if (!cr || !(ua_f[2] >= 0.0f && ua_f[2] <= 1.0e3f) ||
                        !(ua_f[3] >= 1.0f && ua_f[3] <= n) || floorf(ua_f[3]) != ua_f[3]) {
                        err = "flipbook must be [columns, rows, fps, frames?]: whole columns and rows 1 - 256, fps "
                              "0 - 1000, frames whole, 1 to columns x rows (default all)";
                        return false;
                    }
                    for (int k = 0; k < 4; ++k) mat.uv.flip[k] = ua_f[k];
                } else if (key == "emissiveintensity" || key == "emissive") {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "emissiveIntensity must be a number"; return false; }
                    mat.emissive_intensity = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.emissive_intensity)) {
                        err = "emissiveIntensity must be a finite number";
                        return false;
                    }
                } else if (key == "normalstrength") {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "normalStrength must be a number"; return false; }
                    mat.normal_strength = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.normal_strength)) {
                        err = "normalStrength must be a finite number";
                        return false;
                    }
                } else if (key == "cutoff") {
                    if (pe[1].type_enum() != game_data_type::SCALAR) { err = "cutoff must be a number"; return false; }
                    mat.cutoff = static_cast<float>(pe[1]);
                    if (!std::isfinite(mat.cutoff)) { err = "cutoff must be a finite number"; return false; }
                } else if (key == "alphamode") {
                    if (pe[1].type_enum() != game_data_type::STRING) { err = "alphaMode must be \"opaque\", \"cutout\" or \"blend\""; return false; }
                    const std::string am = lower(static_cast<std::string>(pe[1]));
                    if (am == "opaque") mat.alpha_mode = 0;
                    else if (am == "cutout") mat.alpha_mode = 1;
                    else if (am == "blend") mat.alpha_mode = 2;
                    else { err = "alphaMode must be \"opaque\", \"cutout\" or \"blend\""; return false; }
                } else {
                    const char* const kind =
                        arma ? "basecolor|glossiness|specularcolor|fresnel|emissiveintensity|normalstrength|"
                               "cutoff|alphamode"
                      : um_user ? "basecolor|roughness|metalness|glossiness|specularcolor|fresnel|"
                                  "emissiveintensity|normalstrength|cutoff|alphamode|vertexbound|vertexstatic"
                                  : "basecolor|roughness|metalness|emissiveintensity|normalstrength|cutoff|alphamode";
                    err = "unknown material param '" + key + "' (" + kind +   // UV_ANIM: every kind's keys.
                          "|uvscale|uvoffset|uvscroll|uvrotation|uvrotationspeed|uvwarp|flipbook)";
                    return false;
                }
            }
        }

        uv_anim_derive(mat);   // UV_ANIM.
        for (size_t t : targets) next.slots[t] = mat;
    }

    next.any = false;
    for (const auto& s : next.slots) if (s.used) { next.any = true; break; }
    // KH_USER_VS: the set's vertex-stage summary (what the bounds, the bucket test and uvs_upload read without
    // walking the slots).
    next.vertex_any = false;
    next.vertex_bound = 0.0f;
    next.vertex_static = true;
    for (const auto& s : next.slots) {
        if (!s.used || s.shader != 1 || !s.user_vertex) continue;
        next.vertex_any = true;
        if (s.vertex_bound > next.vertex_bound) next.vertex_bound = s.vertex_bound;
        if (!s.vertex_static) next.vertex_static = false;
    }
    if (!next.vertex_any) next.vertex_static = false;
    else g_uvs_sets.store(1u, std::memory_order_relaxed);   // A flag, not a count: a script may re-send a material every frame.
    next.uv_anim_alpha = false;   // UV_ANIM: a clock-driven slot whose sampled alpha decides coverage and casting.
    for (const auto& s : next.slots) {
        if (s.used && s.uv_anim_time && s.alpha_mode != 0 && (s.route_alpha >= 0 || !s.maps[0].path.empty()))
            next.uv_anim_alpha = true;
    }
    next.hash = material_set_hash(next);   // MAT_POOL: intern key; INSTANCING: batch key until upload.
    obj.materials = material_intern(std::move(next));   // MAT_POOL.
    return true;
}
}