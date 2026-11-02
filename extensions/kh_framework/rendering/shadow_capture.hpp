#pragma once

// The injection half (rendering_integration_injection.hpp): the shadow band and pair captures and the mask
// classification. Never include it on its own.
namespace RenderIntegration {

// KH_BAND_RECV_SKIP: a band seal is read by ShadowBandFactor only for mesh pixels whose view-z falls in [near, far), so
// a resolve whose band no live mesh reaches seals nothing anyone reads. Replays the shader's range test on the CPU over
// the scene grid's loose cell bounds (conservative); a capture is skipped only when NO cell reaches the range under
// every view the seal could carry. A skipped slot keeps its seal and its freshness clock is touched (the expiry must
// not move the far fade onto the next band). The sun-raw derivation (rendering/engine_lighting.hpp) still samples every
// resolve.
static constexpr float KH_BAND_RECV_SLACK_M = 1.0f;   // On top of the cells' loose bounds.
// The view's z column as (x, y, z, t): vz = dot(p, xyz) + t. Column layout (vcol / stage_view): [8..11]; row layout
// (frame_view): [2], [6], [10], [14].
inline bool kh_band_cells_reach(const float khbr_z[4], float khbr_near, float khbr_far) {
    for (const auto& khbr_kv : g_scene.grid) {
        const KhSceneCell& khbr_c = khbr_kv.second;
        if (khbr_c.slots.empty()) continue;
        const int32_t khbr_cx = static_cast<int32_t>(static_cast<uint32_t>(khbr_kv.first >> 32));
        const int32_t khbr_cz = static_cast<int32_t>(static_cast<uint32_t>(khbr_kv.first & 0xFFFFFFFFu));
        const float khbr_r = khbr_c.rmax + KH_BAND_RECV_SLACK_M;
        const float khbr_x0 = static_cast<float>(khbr_cx) * KH_SCENE_CELL_M - khbr_r;
        const float khbr_x1 = static_cast<float>(khbr_cx + 1) * KH_SCENE_CELL_M + khbr_r;
        const float khbr_z0 = static_cast<float>(khbr_cz) * KH_SCENE_CELL_M - khbr_r;
        const float khbr_z1 = static_cast<float>(khbr_cz + 1) * KH_SCENE_CELL_M + khbr_r;
        const float khbr_y0 = khbr_c.ymin - khbr_r;
        const float khbr_y1 = khbr_c.ymax + khbr_r;
        const float khbr_ctr[3] = { 0.5f * (khbr_x0 + khbr_x1), 0.5f * (khbr_y0 + khbr_y1), 0.5f * (khbr_z0 + khbr_z1) };
        const float khbr_hx = 0.5f * (khbr_x1 - khbr_x0);
        const float khbr_hy = 0.5f * (khbr_y1 - khbr_y0);
        const float khbr_hz = 0.5f * (khbr_z1 - khbr_z0);
        const float khbr_vz = khbr_ctr[0] * khbr_z[0] + khbr_ctr[1] * khbr_z[1] + khbr_ctr[2] * khbr_z[2] + khbr_z[3];
        const float khbr_ext = fabsf(khbr_z[0]) * khbr_hx + fabsf(khbr_z[1]) * khbr_hy + fabsf(khbr_z[2]) * khbr_hz;
        if (khbr_vz + khbr_ext >= khbr_near && khbr_vz - khbr_ext < khbr_far) return true;
    }

    return false;
}

inline void band_capture(ID3D11DeviceContext* ctx, const float* cb, uint32_t off, uint32_t nf) {
    if (!g_ro.cycle_pv_valid || !g_ls.atlas_tex) {  return; }
    if (off + 20 > nf) {  return; }   // Need matrix + border.
    const float* sm = cb + off;
    const float* border = cb + off + 16;

    if (!(border[1] > border[0]) || border[0] < 0.0f || border[1] > 4000.0f) {
        memcpy(g_ls.band_last_reject, border, sizeof(g_ls.band_last_reject));
        return;
    }
    // Slot: match an existing band by near edge, else take an invalid one, else replace the farthest band when this one
    // is nearer (none otherwise).
    int slot = -1;

    for (int i = 0; i < 8; ++i) {
        if (g_ls.band[i].valid && fabsf(g_ls.band[i].border[0] - border[0]) < 0.5f) { slot = i; break; }
    }

    if (slot < 0) {
        for (int i = 0; i < 8; ++i) {
            if (!g_ls.band[i].valid) { slot = i; break; }
        }
    }

    if (slot < 0) {   // All valid, different bands: replace the farthest if nearer.
        int far_i = 0;

        for (int i = 1; i < 8; ++i) {
            if (g_ls.band[i].border[0] > g_ls.band[far_i].border[0]) far_i = i;
        }

        if (border[0] < g_ls.band[far_i].border[0]) slot = far_i;
        else return;
    }

    {
        auto& b = g_ls.band[slot];
        const float now = effect_time_seconds();
        // View source: the bridge PV.
        if (!g_ro.cycle_pv_valid && !g_ls.frame_view_valid) {  return; }
        // KH_SUN_RAW: the raw cascade axis of every capture, ahead of the seal - the light direction the engine shadows
        // this frame with. Folded through the fresh frame view when live, the bridge PV otherwise.
        {
            const bool khsr_fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                                 now - g_ls.frame_view_time < 0.05f &&
                                 g_ls.pub_fresh_ms != 0 &&
                                 steady_now_ms() - g_ls.pub_fresh_ms <= 1000;
            const float* khsr_r2 = sm + 8;
            float khsr_w[3];
            for (int khsr_c = 0; khsr_c < 3; ++khsr_c) {
                khsr_w[khsr_c] = khsr_fv
                    ? g_ls.frame_view[khsr_c * 4 + 0] * khsr_r2[0] +
                      g_ls.frame_view[khsr_c * 4 + 1] * khsr_r2[1] +
                      g_ls.frame_view[khsr_c * 4 + 2] * khsr_r2[2]
                    : g_ro.cycle_pv.view[khsr_c][0] * khsr_r2[0] +
                      g_ro.cycle_pv.view[khsr_c][1] * khsr_r2[1] +
                      g_ro.cycle_pv.view[khsr_c][2] * khsr_r2[2];
            }
            const float khsr_n = sqrtf(khsr_w[0] * khsr_w[0] + khsr_w[1] * khsr_w[1] + khsr_w[2] * khsr_w[2]);
            if (khsr_n > 1.0e-6f) {
                khsr_w[0] /= khsr_n; khsr_w[1] /= khsr_n; khsr_w[2] /= khsr_n;
                if (khsr_w[1] < 0.0f) { khsr_w[0] = -khsr_w[0]; khsr_w[1] = -khsr_w[1]; khsr_w[2] = -khsr_w[2]; }
                kh_sun_raw_note(khsr_w);
            }
        }
        // Every capture reseals; no throttle (with one back buffer per band, per-frame reseals cost 0.16 ms GPU / 0.17
        // ms CPU and never miss the pool).

        bool khbx_foreign = false;

        {
            const float* khbx_r2 = sm + 8;
            const bool khbx_fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                                 now - g_ls.frame_view_time < 0.05f;

            if (!(khbx_fv || g_ro.cycle_pv_valid)) {
                khbx_foreign = true;
// Pristine: no view source at all.
            } else if (!khbx_fv && !g_ls.view_src_valid && b.last_time > 0.0f) {
                khbx_foreign = true;
            } else {
                float khbx_w[3];

                for (int c = 0; c < 3; ++c) {
                    khbx_w[c] = khbx_fv
                        ? g_ls.frame_view[c * 4 + 0] * khbx_r2[0] +
                          g_ls.frame_view[c * 4 + 1] * khbx_r2[1] +
                          g_ls.frame_view[c * 4 + 2] * khbx_r2[2]
                        : g_ro.cycle_pv.view[c][0] * khbx_r2[0] +
                          g_ro.cycle_pv.view[c][1] * khbx_r2[1] +
                          g_ro.cycle_pv.view[c][2] * khbx_r2[2];
                }

                float khbx_deg = -1.0f;

                if (kh_sun_axis_foreign(khbx_w[0], khbx_w[1], khbx_w[2], &khbx_deg)) {
                    khbx_foreign = true;
                }
            }
        }

        // KH_BAND_RECV_SKIP (see kh_band_cells_reach).
        bool khrs_skip = false;

        if (!khbx_foreign && g_ls.receive_warmed) {
            kh_scene_sync();   // KH_SCENE: the live grid, this frame's adds included.
            bool khrs_reach = false;
            const bool khrs_fvf = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                                  now - g_ls.frame_view_time < 0.05f;

            if (khrs_fvf) {   // The incoming capture, through the fresh frame view.
                const float khrs_z[4] = { g_ls.frame_view[2], g_ls.frame_view[6], g_ls.frame_view[10], g_ls.frame_view[14] };
                khrs_reach = kh_band_cells_reach(khrs_z, border[0], border[1]);
            }

            if (!khrs_reach) {   // ...and through the bridge PV (the fallback view).
                const float khrs_z[4] = { g_ro.cycle_pv.view[0][2], g_ro.cycle_pv.view[1][2],
                                          g_ro.cycle_pv.view[2][2], g_ro.cycle_pv.view[3][2] };
                khrs_reach = kh_band_cells_reach(khrs_z, border[0], border[1]);
            }

            if (!khrs_reach && b.valid) {   // The standing seal the table keeps serving.
                khrs_reach = kh_band_cells_reach(b.vcol + 8, b.border[0], b.border[1]);
            }

            if (!khrs_reach && b.stage_valid && b.stage_view_ok) {   // The pending stage.
                khrs_reach = kh_band_cells_reach(b.stage_view + 8, b.stage_border[0], b.stage_border[1]);
            }

            khrs_skip = !khrs_reach;

            if (khrs_skip) {
                if (b.valid) b.last_time = now;   // A seal the meshes left is not a stale seal.
            }
        }

        if (!khbx_foreign && !khrs_skip) {

        if (!b.tex) {
            D3D11_TEXTURE2D_DESC td = {};
            g_ls.atlas_tex->GetDesc(&td);
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.CPUAccessFlags = 0;
            td.MiscFlags = 0;
            ID3D11Device* dev = nullptr;
            g_ls.atlas_tex->GetDevice(&dev);
            if (!dev) return;

            for (int bi = 0; bi < 8; ++bi) {
                auto& bb = g_ls.band[bi];
                if (bb.tex) continue;

                if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &bb.tex)) && bb.tex) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
                    sd.Format = kh_atlas_view_fmt(g_ls.atlas_fmt);   // The atlas's own format (a band is its copy).
                    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    sd.Texture2D.MipLevels = 1;

                    if (FAILED(dev->CreateShaderResourceView(bb.tex, &sd, &bb.srv))) {
                        bb.srv = nullptr;
                        bb.tex->Release();
                        bb.tex = nullptr;
                    }
                }
            }

            // The staging back buffers share the band desc exactly (CopyResource demands it, and the commit swaps them
            // into band slots).
            const int khbp_n = kh_band_pool_n();   // Only create what is used.
            for (int khbp_k = 0; khbp_k < khbp_n; ++khbp_k) {
                if (g_band_pool[khbp_k].tex) continue;

                if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_band_pool[khbp_k].tex)) &&
                    g_band_pool[khbp_k].tex) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
                    sd.Format = kh_atlas_view_fmt(g_ls.atlas_fmt);   // As the band slots above.
                    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    sd.Texture2D.MipLevels = 1;

                    if (FAILED(dev->CreateShaderResourceView(g_band_pool[khbp_k].tex,
                                                             &sd, &g_band_pool[khbp_k].srv))) {
                        g_band_pool[khbp_k].srv = nullptr;
                        g_band_pool[khbp_k].tex->Release();
                        g_band_pool[khbp_k].tex = nullptr;
                    }
                }

                g_band_pool[khbp_k].busy = false;
            }

            dev->Release();
            if (!b.tex || !b.srv) {  return; }
        }

        bool khdf_staged = false;
        {

            if (b.valid) {
                // A valid slot re-seals through the staging pool: on a pool miss its existing complete seal stands and
                // the next sweep retries - except one miss a flush (g_band_fall_flush), which takes the immediate path
                // below.
                khdf_staged = true;
                int khdf_p = b.stage_pool;   // re-sweep of a slot already staged.

                if (khdf_p < 0) {
                    const int khdf_n = kh_band_pool_n();
                    kh_band_pool_ensure_n(khdf_n);   // Creation follows the count.
                    for (int k = 0; k < khdf_n; ++k) {
                        if (!g_band_pool[k].busy && g_band_pool[k].tex && g_band_pool[k].srv) {
                            khdf_p = k;
                            break;
                        }
                    }
                }

                if (khdf_p < 0 || !g_band_pool[khdf_p].tex) {
                    const uint64_t khdf_fs = g_flush_serial.load(std::memory_order_relaxed);
                    if (g_band_fall_flush != khdf_fs) {
                        g_band_fall_flush = khdf_fs;
                        khdf_staged = false;   // The immediate path below.
                    }
                } else {
                    g_band_pool[khdf_p].busy = true;
                    b.stage_pool = khdf_p;
                    memcpy(b.stage_sm, sm, sizeof(b.stage_sm));
                    memcpy(b.stage_border, border, sizeof(b.stage_border));
                    b.stage_view_ok = false;
                    b.stage_pair_ok = false;   // Fresh stage, fresh pair.
                    b.stage_first_ok = false;
                    {
                        const bool khsv_fvf = g_ls.frame_view_valid &&
                            g_ls.frame_view_time >= 0.0f &&
                            now - g_ls.frame_view_time < 0.05f;
                        if (khsv_fvf) {
                            for (int khsv_c = 0; khsv_c < 3; ++khsv_c)
                                for (int khsv_r = 0; khsv_r < 4; ++khsv_r)
                                    b.stage_view[khsv_c * 4 + khsv_r] =
                                        g_ls.frame_view[khsv_r * 4 + khsv_c];
                            b.stage_view_ok = true;
                        } else if (g_ro.cycle_pv_valid) {
                            for (int khsv_c = 0; khsv_c < 3; ++khsv_c)
                                for (int khsv_r = 0; khsv_r < 4; ++khsv_r)
                                    b.stage_view[khsv_c * 4 + khsv_r] =
                                        g_ro.cycle_pv.view[khsv_r][khsv_c];
                            b.stage_view_ok = true;
                        }
                    }
                    b.stage_since = now;
                    b.stage_valid = true;
                    b.last_time = now;
                    g_ls.stage_any = true;
                    // The copy happens here, at the resolve that paired this sm with this atlas content.
                    ctx->CopyResource(g_band_pool[khdf_p].tex, g_ls.atlas_tex);
                }
            }
        }

        // A staged capture skips only the immediate seal below; the sun derivation (KH_SUN_RAW) already ran above, and
        // it is measurement, not consumption.
        if (!khdf_staged) {

        memcpy(b.sm, sm, sizeof(b.sm));
        memcpy(b.border, border, sizeof(b.border));

        const bool fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                        now - g_ls.frame_view_time < 0.05f;
        const bool khsv_nat = !fv || kh_band_view_native(g_ls.frame_view, true);
        // A seal born on it still goes out pending (pending_view below), and the frame view's completion loop
        // (shadow_view_scan) completes it or drops it, as it does every seal.
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 4; ++r) {
                b.vcol[c * 4 + r] = (fv && khsv_nat) ? g_ls.frame_view[r * 4 + c]
                                       : g_ro.cycle_pv.view[r][c];
            }
        }

        b.pending_view = true;
        b.vcol_bridge  = !fv;

        b.pending_since = now;
        if (g_ls.first_capture_time < 0.0f) g_ls.first_capture_time = now;
        ctx->CopyResource(b.tex, g_ls.atlas_tex);
        b.last_time = now;
        b.valid = true;

        }   // End !khdf_staged (immediate path).

        }   // End !khbx_foreign (seal section).

    }
}

// At a resolve draw: the bound PS b0..b13 and VS b0..b3 CBs are looked up in the registry; a sampling-family matrix
// found there is the transform for the atlas content this draw samples. band_capture seals the pair into that band's
// slot (one of eight): copy the atlas and store the matrix together.
inline void resolve_pair_capture(ID3D11DeviceContext* ctx) {
    if (!shadow_live_wanted() || !g_ls.atlas_tex) return;
    // RV binds constants at varying slots: scan PS b0..b13 and VS b0..b3 (every resolve hit comes from the small
    // VS-side block).
    ID3D11Buffer* bufs[18] = {};
    ctx->PSGetConstantBuffers(0, 14, bufs);
    ctx->VSGetConstantBuffers(0, 4, bufs + 14);
    {
        if (g_ls.resolve_window == 0) {

            for (int b = 0; b < 18; ++b) { if (bufs[b]) bufs[b]->Release(); }

            return;
        }

        g_ls.resolve_window--;

        for (int b = 0; b < 18; ++b) {
            if (!bufs[b]) continue;

            for (uint32_t i = 0; i < 16; ++i) {
                if (g_ls.cb_reg[i].buf != static_cast<ID3D11Resource*>(bufs[b])) continue;
                const float* f = g_ls.cb_reg[i].data;
                const uint32_t nf = g_ls.cb_reg[i].floats;

                if (nf >= KH_REG_READ_MIN && nf <= 512) {   // KH_REG_SLICE: floats 176..199 alone are kept.
                    const float* m180 = f + 180;
                    const float n0 = m180[0] * m180[0] + m180[1] * m180[1] + m180[2] * m180[2];
                    const float n1 = m180[4] * m180[4] + m180[5] * m180[5] + m180[6] * m180[6];

                    if (n0 > 1e-10f && n0 < 1.0f && n1 > 1e-10f && n1 < 1.0f) {
                        g_ls.resolve_seen_since_cast = true;

                        if (!g_mask_cast_fired && g_ls.frame_view_valid &&
                            g_ls.view_published_this_frame) {
                            g_mask_cast_arm = true;
                        }
                        const bool rt_resolve_ok = g_mask.rt_is_resolve ||
                            (g_mask.rt_fmt_ok && g_mask.rt_w >= 620);

                        if (rt_resolve_ok) {
                        if (nf >= 180) {
                            const float swp_now = effect_time_seconds();
                            const bool swp_gap = g_mask.sweep_last_time < 0.0f ||
                                                 swp_now - g_mask.sweep_last_time > 0.25f;
                            bool swp_reset = swp_gap;

                            if (!swp_reset && g_mask.cast_fov_valid) {
                                swp_reset =
                                    fabsf(f[176] - g_mask.cast_fov[0]) > 0.1f * fabsf(g_mask.cast_fov[0]) + 1e-4f ||
                                    fabsf(f[177] - g_mask.cast_fov[1]) > 0.1f * fabsf(g_mask.cast_fov[1]) + 1e-4f;
                            }

                            if (swp_reset) {
                                g_mask.sweep_settle = 0;
                                g_mask.sweep_need = swp_gap ? 5 : 3;
                            }
                            if (g_mask.sweep_settle < 1000) g_mask.sweep_settle++;

                            g_mask.sweep_last_time = swp_now;

                            memcpy(g_mask.cast_fov, f + 176, sizeof(g_mask.cast_fov));
                            g_mask.cast_fov_valid = true;

                        }

                        {
                            ID3D11RenderTargetView* rtv0 = nullptr;
                            ctx->OMGetRenderTargets(1, &rtv0, nullptr);
                            float khpc_w = 0.0f, khpc_h = 0.0f;
                            bool  khpc_have = false;

                            if (rtv0) {
                                ID3D11Resource* rr = nullptr;
                                rtv0->GetResource(&rr);

                                if (rr) {
                                    ID3D11Texture2D* rt = nullptr;
                                    rr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rt));

                                    if (rt) {
                                        D3D11_TEXTURE2D_DESC rd = {};
                                        rt->GetDesc(&rd);
                                        khpc_w = static_cast<float>(rd.Width);
                                        khpc_h = static_cast<float>(rd.Height);
                                        khpc_have = true;
                                        rt->Release();
                                    }

                                    rr->Release();
                                }

                                bool rtv_mask_like = false;

                                {
                                    ID3D11Resource* mres = nullptr;
                                    rtv0->GetResource(&mres);
                                    ID3D11Texture2D* mtex = nullptr;

                                    if (mres && SUCCEEDED(mres->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&mtex)))) {
                                        D3D11_TEXTURE2D_DESC md = {};
                                        mtex->GetDesc(&md);
                                        const DXGI_FORMAT mf = md.Format;
                                        const bool single =
                                            mf == DXGI_FORMAT_R8_UNORM || mf == DXGI_FORMAT_R8_TYPELESS ||
                                            mf == DXGI_FORMAT_R16_UNORM || mf == DXGI_FORMAT_R16_TYPELESS ||
                                            mf == DXGI_FORMAT_R16_FLOAT || mf == DXGI_FORMAT_R8_UINT;
                                        rtv_mask_like = single && md.Width >= kh_mask_min_w();   // KH_MASK_REL_W.
                                        mtex->Release();
                                    }

                                    if (mres) mres->Release();
                                }

                                if (rtv_mask_like) {
                                    if (khpc_have) {
                                        g_mask.cast_dims[0] = khpc_w;
                                        g_mask.cast_dims[1] = khpc_h;
                                    }
                                }

                                if (rtv_mask_like && g_mask.engine_mask_rtv != rtv0) {
                                    if (g_mask.engine_mask_rtv) {
                                        g_mask.engine_mask_rtv->Release();
                                    }
                                    g_mask.engine_mask_rtv = rtv0;
                                } else {
                                    rtv0->Release();   // Duplicate or failed identity.
                                }
                            }

                            ID3D11ShaderResourceView* d0 = nullptr;
                            ctx->PSGetShaderResources(0, 1, &d0);

                            if (d0) {
                                bool depth_ok = false;
                                ID3D11Resource* dr = nullptr;
                                d0->GetResource(&dr);

                                if (dr) {
                                    ID3D11Texture2D* dt = nullptr;
                                    dr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&dt));

                                    if (dt) {
                                        D3D11_TEXTURE2D_DESC dd2 = {};
                                        dt->GetDesc(&dd2);
                                        const int df = static_cast<int>(dd2.Format);
                                        depth_ok = dd2.Width == static_cast<UINT>(g_mask.cast_dims[0]) &&
                                                   (df == 39 || df == 40 || df == 41 || df == 46 || df == 54);
                                        dt->Release();
                                    }

                                    dr->Release();
                                }

                                if (depth_ok && g_mask.cast_depth != d0) {
                                    if (g_mask.cast_depth) g_mask.cast_depth->Release();
                                    g_mask.cast_depth = d0;
                                } else {
                                    d0->Release();
                                }
                            }
                        }
                        }

                        band_capture(ctx, f, 180, nf);
                    }
                }
            }
        }

        for (int b = 0; b < 18; ++b) { if (bufs[b]) bufs[b]->Release(); }

        return;
    }

}

inline void mask_classify_rt(UINT n, ID3D11RenderTargetView* const* rtvs) {
    g_mask.rt_is_resolve = false;
    if (!n || !rtvs || !rtvs[0]) return;
    ID3D11Resource* res = nullptr;
    rtvs[0]->GetResource(&res);
    if (!res) return;

    uint32_t khcr_fmt = 0;
    UINT khcr_w = 0;
    bool khcr_known = false;

    for (uint32_t khcr_i = 0; khcr_i < g_rt0_desc_n; ++khcr_i) {
        if (g_rt0_desc[khcr_i].res == static_cast<void*>(res)) {
            khcr_fmt = g_rt0_desc[khcr_i].fmt;
            khcr_w = g_rt0_desc[khcr_i].w;
            khcr_known = true;
            break;
        }
    }

    if (!khcr_known) {
        ID3D11Texture2D* tex = nullptr;
        res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));

        if (tex) {
            D3D11_TEXTURE2D_DESC td = {};
            tex->GetDesc(&td);
            khcr_fmt = static_cast<uint32_t>(td.Format);
            khcr_w = td.Width;
            tex->Release();
        }

        const uint32_t khcr_slot = g_rt0_desc_n < 32
            ? g_rt0_desc_n : (g_rt0_desc_next & 31u);
        g_rt0_desc[khcr_slot].res = static_cast<void*>(res);
        g_rt0_desc[khcr_slot].fmt = khcr_fmt;
        g_rt0_desc[khcr_slot].w = khcr_w;

        if (g_rt0_desc_n < 32) g_rt0_desc_n++;
        else g_rt0_desc_next++;
    }

    res->Release();
    for (uint32_t khmc_i = 0; khmc_i < g_rt0_desc_n; ++khmc_i) {
        if (g_rt0_desc[khmc_i].res == static_cast<void*>(res)) {
            break;
        }
    }
    if (khcr_fmt == 0) return;
    g_mask.rt_fmt_ok =
        (khcr_fmt == static_cast<uint32_t>(DXGI_FORMAT_R8_UNORM) ||
         khcr_fmt == static_cast<uint32_t>(DXGI_FORMAT_R8_TYPELESS) ||
         khcr_fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_UNORM) ||
         khcr_fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_TYPELESS) ||
         khcr_fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_FLOAT));
    g_mask.rt_w = khcr_w;
    g_mask.rt_is_resolve = khcr_w >= kh_mask_min_w() &&   // KH_MASK_REL_W.
                           g_mask.rt_fmt_ok;
    if (g_mask.rt_is_resolve) {
        for (uint32_t khmt_i = 0; khmt_i < g_rt0_desc_n; ++khmt_i) {
            if (g_rt0_desc[khmt_i].res == static_cast<void*>(res)) {
                break;
            }
        }
    }
}
}