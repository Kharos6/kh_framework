#pragma once

// The injection half (rendering_integration_injection.hpp): the shadow band and pair captures and the mask
// classification. Never include it on its own.
namespace RenderIntegration {

// BAND_RECV_SKIP: a band seal is read by ShadowBandFactor only for mesh pixels whose view-z falls in [near, far), so
// a resolve whose band no live mesh reaches seals nothing anyone reads. Replays the shader's range test on the CPU over
// the scene grid's loose cell bounds (conservative); a capture is skipped only when NO cell reaches the range under
// every view the seal could carry. A skipped slot keeps its seal and its freshness clock is touched (the expiry must
// not move the far fade onto the next band). The sun-raw derivation (rendering/engine_lighting.hpp) still samples every
// resolve.
static constexpr float BAND_RECV_SLACK_M = 1.0f;   // On top of the cells' loose bounds.
// The view's z column as (x, y, z, t): vz = dot(p, xyz) + t. Column layout (vcol / stage_view): [8..11]; row layout
// (frame_view): [2], [6], [10], [14].
inline bool band_cells_reach(const float z[4], float br_near, float br_far) {
    for (const auto& kv : g_scene.grid) {
        const SceneCell& c = kv.second;
        if (c.slots.empty()) continue;
        const int32_t cx = static_cast<int32_t>(static_cast<uint32_t>(kv.first >> 32));
        const int32_t cz = static_cast<int32_t>(static_cast<uint32_t>(kv.first & 0xFFFFFFFFu));
        const float r = c.rmax + BAND_RECV_SLACK_M;
        const float x0 = static_cast<float>(cx) * SCENE_CELL_M - r;
        const float x1 = static_cast<float>(cx + 1) * SCENE_CELL_M + r;
        const float z0 = static_cast<float>(cz) * SCENE_CELL_M - r;
        const float z1 = static_cast<float>(cz + 1) * SCENE_CELL_M + r;
        const float y0 = c.ymin - r;
        const float y1 = c.ymax + r;
        const float ctr[3] = { 0.5f * (x0 + x1), 0.5f * (y0 + y1), 0.5f * (z0 + z1) };
        const float hx = 0.5f * (x1 - x0);
        const float hy = 0.5f * (y1 - y0);
        const float hz = 0.5f * (z1 - z0);
        const float vz = ctr[0] * z[0] + ctr[1] * z[1] + ctr[2] * z[2] + z[3];
        const float ext = fabsf(z[0]) * hx + fabsf(z[1]) * hy + fabsf(z[2]) * hz;
        if (vz + ext >= br_near && vz - ext < br_far) return true;
    }

    return false;
}

inline void band_capture(ID3D11DeviceContext* ctx, const float* cb, uint32_t off, uint32_t nf) {
    if (!g_ro.cycle_pv_valid || !g_ls.atlas_tex) {  return; }
    if (off + 20 > nf) {  return; }   // Need matrix + border.
    const float* sm = cb + off;
    const float* border = cb + off + 16;

    if (!(border[1] > border[0]) || border[0] < 0.0f || border[1] > 4000.0f) return;
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
        // SUN_RAW: the raw cascade axis of every capture, ahead of the seal - the light direction the engine shadows
        // this frame with. Folded through the fresh frame view when live, the bridge PV otherwise.
        {
            const bool sr_fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                               now - g_ls.frame_view_time < 0.05f &&
                               g_ls.pub_fresh_ms != 0 &&
                               steady_now_ms() - g_ls.pub_fresh_ms <= 1000;
            const float* r2 = sm + 8;
            float w[3];
            for (int sr_c = 0; sr_c < 3; ++sr_c) {
                w[sr_c] = sr_fv
                    ? g_ls.frame_view[sr_c * 4 + 0] * r2[0] +
                      g_ls.frame_view[sr_c * 4 + 1] * r2[1] +
                      g_ls.frame_view[sr_c * 4 + 2] * r2[2]
                    : g_ro.cycle_pv.view[sr_c][0] * r2[0] +
                      g_ro.cycle_pv.view[sr_c][1] * r2[1] +
                      g_ro.cycle_pv.view[sr_c][2] * r2[2];
            }
            const float n = sqrtf(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
            if (n > 1.0e-6f) {
                w[0] /= n; w[1] /= n; w[2] /= n;
                if (w[1] < 0.0f) { w[0] = -w[0]; w[1] = -w[1]; w[2] = -w[2]; }
                sun_raw_note(w);
            }
        }
        // Every capture reseals; no throttle (with one back buffer per band, per-frame reseals cost 0.16 ms GPU / 0.17
        // ms CPU and never miss the pool).

        bool foreign = false;

        {
            const float* r2 = sm + 8;
            const bool bx_fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                               now - g_ls.frame_view_time < 0.05f;

            // The bridge PV is always there (the entry gate), so the fold below always has a view.
            if (!bx_fv && !g_ls.view_src_valid && b.last_time > 0.0f) {
                foreign = true;
            } else {
                float w[3];

                for (int c = 0; c < 3; ++c) {
                    w[c] = bx_fv
                        ? g_ls.frame_view[c * 4 + 0] * r2[0] +
                          g_ls.frame_view[c * 4 + 1] * r2[1] +
                          g_ls.frame_view[c * 4 + 2] * r2[2]
                        : g_ro.cycle_pv.view[c][0] * r2[0] +
                          g_ro.cycle_pv.view[c][1] * r2[1] +
                          g_ro.cycle_pv.view[c][2] * r2[2];
                }

                float deg = -1.0f;

                if (sun_axis_foreign(w[0], w[1], w[2], &deg)) {
                    foreign = true;
                }
            }
        }

        // BAND_RECV_SKIP (see band_cells_reach).
        bool skip = false;

        if (!foreign && g_ls.receive_warmed) {
            scene_sync();   // SCENE: the live grid, this frame's adds included.
            bool reach = false;
            const bool fvf = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                             now - g_ls.frame_view_time < 0.05f;

            if (fvf) {   // The incoming capture, through the fresh frame view.
                const float z[4] = { g_ls.frame_view[2], g_ls.frame_view[6], g_ls.frame_view[10], g_ls.frame_view[14] };
                reach = band_cells_reach(z, border[0], border[1]);
            }

            if (!reach) {   // ...and through the bridge PV (the fallback view).
                const float z[4] = { g_ro.cycle_pv.view[0][2], g_ro.cycle_pv.view[1][2],
                                     g_ro.cycle_pv.view[2][2], g_ro.cycle_pv.view[3][2] };
                reach = band_cells_reach(z, border[0], border[1]);
            }

            if (!reach && b.valid) {   // The standing seal the table keeps serving.
                reach = band_cells_reach(b.vcol + 8, b.border[0], b.border[1]);
            }

            if (!reach && b.stage_valid && b.stage_view_ok) {   // The pending stage.
                reach = band_cells_reach(b.stage_view + 8, b.stage_border[0], b.stage_border[1]);
            }

            skip = !reach;

            if (skip) {
                if (b.valid) b.last_time = now;   // A seal the meshes left is not a stale seal.
            }
        }

        if (!foreign && !skip) {

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
                    sd.Format = atlas_view_fmt(g_ls.atlas_fmt);   // The atlas's own format (a band is its copy).
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
            const int n = band_pool_n();   // Only create what is used.
            for (int bp_k = 0; bp_k < n; ++bp_k) {
                if (g_band_pool[bp_k].tex) continue;

                if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_band_pool[bp_k].tex)) &&
                    g_band_pool[bp_k].tex) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
                    sd.Format = atlas_view_fmt(g_ls.atlas_fmt);   // As the band slots above.
                    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    sd.Texture2D.MipLevels = 1;

                    if (FAILED(dev->CreateShaderResourceView(g_band_pool[bp_k].tex,
                                                             &sd, &g_band_pool[bp_k].srv))) {
                        g_band_pool[bp_k].srv = nullptr;
                        g_band_pool[bp_k].tex->Release();
                        g_band_pool[bp_k].tex = nullptr;
                    }
                }

                g_band_pool[bp_k].busy = false;
            }

            dev->Release();
            if (!b.tex || !b.srv) {  return; }
        }

        bool staged = false;
        {

            if (b.valid) {
                // A valid slot re-seals through the staging pool: on a pool miss its existing complete seal stands and
                // the next sweep retries - except one miss a flush (g_band_fall_flush), which takes the immediate path
                // below.
                staged = true;
                int p = b.stage_pool;   // re-sweep of a slot already staged.

                if (p < 0) {
                    const int n = band_pool_n();
                    band_pool_ensure_n(n);   // Creation follows the count.
                    for (int k = 0; k < n; ++k) {
                        if (!g_band_pool[k].busy && g_band_pool[k].tex && g_band_pool[k].srv) {
                            p = k;
                            break;
                        }
                    }
                }

                if (p < 0 || !g_band_pool[p].tex) {
                    const uint64_t fs = g_flush_serial.load(std::memory_order_relaxed);
                    if (g_band_fall_flush != fs) {
                        g_band_fall_flush = fs;
                        staged = false;   // The immediate path below.
                    }
                } else {
                    g_band_pool[p].busy = true;
                    b.stage_pool = p;
                    memcpy(b.stage_sm, sm, sizeof(b.stage_sm));
                    memcpy(b.stage_border, border, sizeof(b.stage_border));
                    b.stage_view_ok = false;
                    b.stage_pair_ok = false;   // Fresh stage, fresh pair.
                    b.stage_first_ok = false;
                    {
                        const bool fvf = g_ls.frame_view_valid &&
                            g_ls.frame_view_time >= 0.0f &&
                            now - g_ls.frame_view_time < 0.05f;
                        if (fvf) {
                            for (int sv_c = 0; sv_c < 3; ++sv_c)
                                for (int sv_r = 0; sv_r < 4; ++sv_r)
                                    b.stage_view[sv_c * 4 + sv_r] =
                                        g_ls.frame_view[sv_r * 4 + sv_c];
                            b.stage_view_ok = true;
                        } else if (g_ro.cycle_pv_valid) {
                            for (int sv_c = 0; sv_c < 3; ++sv_c)
                                for (int sv_r = 0; sv_r < 4; ++sv_r)
                                    b.stage_view[sv_c * 4 + sv_r] =
                                        g_ro.cycle_pv.view[sv_r][sv_c];
                            b.stage_view_ok = true;
                        }
                    }
                    b.stage_since = now;
                    b.stage_valid = true;
                    b.last_time = now;
                    g_ls.stage_any = true;
                    // The copy happens here, at the resolve that paired this sm with this atlas content.
                    ctx->CopyResource(g_band_pool[p].tex, g_ls.atlas_tex);
                }
            }
        }

        // A staged capture skips only the immediate seal below; the sun derivation (SUN_RAW) already ran above, and
        // it is measurement, not consumption.
        if (!staged) {

        memcpy(b.sm, sm, sizeof(b.sm));
        memcpy(b.border, border, sizeof(b.border));

        const bool fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                        now - g_ls.frame_view_time < 0.05f;
        const bool nat = !fv || band_view_native(g_ls.frame_view);
        // A seal born on it still goes out pending (pending_view below), and the frame view's completion loop
        // (shadow_view_scan) completes it or drops it, as it does every seal.
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 4; ++r) {
                b.vcol[c * 4 + r] = (fv && nat) ? g_ls.frame_view[r * 4 + c]
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

        }   // End !staged (immediate path).

        }   // End !foreign (seal section).

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

                if (nf >= REG_READ_MIN && nf <= 512) {   // REG_SLICE: floats 176..199 alone are kept.
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
                            float w = 0.0f, h = 0.0f;
                            bool  have = false;

                            if (rtv0) {
                                ID3D11Resource* rr = nullptr;
                                rtv0->GetResource(&rr);

                                if (rr) {
                                    ID3D11Texture2D* rt = nullptr;
                                    rr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rt));

                                    if (rt) {
                                        D3D11_TEXTURE2D_DESC rd = {};
                                        rt->GetDesc(&rd);
                                        w = static_cast<float>(rd.Width);
                                        h = static_cast<float>(rd.Height);
                                        have = true;
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
                                        rtv_mask_like = single && md.Width >= mask_min_w();   // MASK_REL_W.
                                        mtex->Release();
                                    }

                                    if (mres) mres->Release();
                                }

                                if (rtv_mask_like) {
                                    if (have) {
                                        g_mask.cast_dims[0] = w;
                                        g_mask.cast_dims[1] = h;
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
                                        // Single-channel views only (R32 / R24 / R16_FLOAT): the cast-depth reads
                                        // take .x alone (static.hlsl KhCastZPick) - a two-plane format needs it too.
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

    uint32_t fmt = 0;
    UINT w = 0;
    bool known = false;

    for (uint32_t i = 0; i < g_rt0_desc_n; ++i) {
        if (g_rt0_desc[i].res == static_cast<void*>(res)) {
            fmt = g_rt0_desc[i].fmt;
            w = g_rt0_desc[i].w;
            known = true;
            break;
        }
    }

    if (!known) {
        ID3D11Texture2D* tex = nullptr;
        res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));

        if (tex) {
            D3D11_TEXTURE2D_DESC td = {};
            tex->GetDesc(&td);
            fmt = static_cast<uint32_t>(td.Format);
            w = td.Width;
            tex->Release();
        }

        const uint32_t slot = g_rt0_desc_n < 32
            ? g_rt0_desc_n : (g_rt0_desc_next & 31u);
        g_rt0_desc[slot].res = static_cast<void*>(res);
        g_rt0_desc[slot].fmt = fmt;
        g_rt0_desc[slot].w = w;

        if (g_rt0_desc_n < 32) g_rt0_desc_n++;
        else g_rt0_desc_next++;
    }

    res->Release();
    if (fmt == 0) return;
    g_mask.rt_fmt_ok =
        (fmt == static_cast<uint32_t>(DXGI_FORMAT_R8_UNORM) ||
         fmt == static_cast<uint32_t>(DXGI_FORMAT_R8_TYPELESS) ||
         fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_UNORM) ||
         fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_TYPELESS) ||
         fmt == static_cast<uint32_t>(DXGI_FORMAT_R16_FLOAT));
    g_mask.rt_w = w;
    g_mask.rt_is_resolve = w >= mask_min_w() &&   // MASK_REL_W.
                           g_mask.rt_fmt_ok;
}
}