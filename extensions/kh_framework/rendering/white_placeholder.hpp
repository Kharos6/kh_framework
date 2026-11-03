#pragma once

// The feature half (rendering_integration_features.hpp): the white placeholder (WHITE_PREVIEW): its shaders and its
// draw. Never include it on its own.
namespace RenderIntegration {

// WHITE_PREVIEW: the placeholder pair (white_draw draws the world meshes flat white while the shader pool
// compiles). THREE call sites, all in ensure_resources: the two deferral returns (the async hold and the prewarm
// gate) and the success path, so a warm-cache session that defers nothing still creates it. g_res.white_tried caps
// it at one compile attempt per device. Non-fatal: a failure leaves it null.

inline void white_ensure(ID3D11Device* dev) {
    if (!dev || g_res.white_tried) return;
    g_res.white_tried = true;
    static const D3D_SHADER_MACRO defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr },
    };
    const std::string src = hlsl_src(HLSL_CB) + hlsl_src(HLSL_WHITE).c_str();
    ID3DBlob* vsb = nullptr;
    ID3DBlob* psb = nullptr;
    std::string err = compile_shader(src.c_str(), "VSWhite", "vs_5_0", defines, &vsb);
    if (err.empty())
        err = compile_shader(src.c_str(), "PSWhite", "ps_5_0", defines, &psb);
    if (!err.empty() || !vsb || !psb) {
        if (vsb) vsb->Release();
        if (psb) psb->Release();
        report_error_once_safe("KH RenderIntegration: white preview: " +
                               (err.empty() ? std::string("no blob") : err));
        return;
    }
    // step / hr: the last attempted call and its result, for the one report below.
    const char* step = "none";
    HRESULT hr = S_OK;

    step = "CreateVertexShader";
    hr = dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(),
                                 nullptr, &g_res.vs_white);
    if (SUCCEEDED(hr)) {
        step = "CreatePixelShader";
        hr = dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(),
                                    nullptr, &g_res.ps_white);
    }
    if (SUCCEEDED(hr)) {
        // VSWhite takes VSIn (position + normal); the body reads i.pos alone.
        const D3D11_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        step = "CreateInputLayout";
        hr = dev->CreateInputLayout(il, 2u, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_res.layout_white);
    }
    vsb->Release();
    psb->Release();
    if (SUCCEEDED(hr)) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(CBOBJ_BYTES);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        step = "CreateBuffer(obj)";
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.white_obj_cb);
        if (SUCCEEDED(hr)) {
            bd.ByteWidth = static_cast<UINT>(CBFRAME_BYTES);
            step = "CreateBuffer(frame)";
            hr = dev->CreateBuffer(&bd, nullptr, &g_res.white_frame_cb);
        }
    }
    if (SUCCEEDED(hr)) {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.MultisampleEnable = TRUE;
        rd.DepthClipEnable = FALSE;   // The mesh rasterizer's contract.
        rd.DepthBias = 0;
        rd.SlopeScaledDepthBias = 0.0f;
        rd.DepthBiasClamp = 0.0f;
        step = "CreateRasterizerState";
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_white);
        if (SUCCEEDED(hr)) {
            D3D11_DEPTH_STENCIL_DESC dd = {};
            dd.DepthEnable = TRUE;
            dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            dd.StencilEnable = FALSE;
            step = "CreateDepthStencilState";
            hr = dev->CreateDepthStencilState(&dd, &g_res.dss_white);
        }
    }
    if (FAILED(hr)) {
        KH_SAFE_RELEASE(g_res.vs_white);
        KH_SAFE_RELEASE(g_res.ps_white);
        KH_SAFE_RELEASE(g_res.layout_white);
        KH_SAFE_RELEASE(g_res.white_obj_cb);
        KH_SAFE_RELEASE(g_res.white_frame_cb);
        KH_SAFE_RELEASE(g_res.rast_white);
        KH_SAFE_RELEASE(g_res.dss_white);
        report_error_once_safe(std::string("KH RenderIntegration: white preview: ") + step + " " + hr_str(hr) +
                        " (preview stands down)");
        return;
    }
    g_res.white_ready = true;
}

inline void white_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    if (!dev || !ctx || !g_res.white_ready) return;
    if (fsaa_world_standdown()) return;   // FSAA 1x: the placeholder is a world mesh too.
    // Runs inside the flush's serialized window; a failure costs the objects whose buffers are missing, not the pass.
    ensure_mesh_vbs(dev);
    if (g_res.mesh_vb.empty()) return;

    scene_sync();   // SCENE.
    if (g_scene.alive_n == 0) return;

    const float now = effect_time_seconds();
    // EA_WHITE_CAM: the flush meshes' camera (flush_locked) - the cycle latch, the boundary camera the engine's
    // depth was drawn with; the live bridge only without a usable latch; then the frame view's rotation when fresh.
    // Not mirrored: the flush's live pick on a sparse in-flight cycle (sparse_live, under 16 opaque draws).
    RVExtBridge::ProjectionViewTransform pv = {};
    if (g_ro.cycle_pv_valid && fabsf(g_ro.cycle_pv.projection[2][2]) >= 1e-6f) {
        pv = g_ro.cycle_pv;
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        return;
    }
    adopt_frame_view(pv);

    ConstantData cbf = {};
    mul_4x4(pv.view, pv.projection, cbf.view_proj);
    if (!upload_frame_cb(ctx, g_res.white_frame_cb, cbf)) return;

    StateBackup backup;
    backup.capture(ctx);
    OmSlot0 s0;   // MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    s0.open(ctx);

    UINT stride = sizeof(MeshVertex);
    VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
    const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->IASetInputLayout(g_res.layout_white);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_white, nullptr, 0);
    ctx->PSSetShader(g_res.ps_white, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* cbs[2] = { g_res.white_obj_cb, g_res.white_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->RSSetState(g_res.rast_white);
    ctx->OMSetDepthStencilState(g_res.dss_white, 0);
    // WSEE: the placeholder writes the main depth (in the flush: past any capture).
    wsee_taint(g_topo_cycles);
    ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);

    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (o.fullscreen || obj_in_front(o)) continue;   // INFRONT: never in the world depth.
        // WHITE_NOFX: an effect mesh has no solid look to stand in for and never writes depth (FX_NODEPTH);
        // drawn here it is an opaque white wall - the whole screen for a volume around the camera.
        if (o.effect != 0) continue;
        if (!obj_shown(o)) continue;   // Hidden: no pixel (casterOnly: it casts, in the maps alone).
        bool expired = false;
        if (lifetime_envelope(o, now, expired) <= 0.0f || expired) continue;
        const int mid = mesh_id_clamp(o.mesh);
        if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size()) continue;
        const VbRef vb = mesh_vb_ref(mid, i);   // CLOTH.
        if (!vb) continue;

        ConstantData cbd = {};
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        cbd.size_axes[0] = o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        fill_obj_rot(cbd, o.rot_m);
        // VSWhite takes the absolute branch, matching the unrebased viewProj built above; the pair must agree.
        if (!upload_obj_cb(ctx, g_res.white_obj_cb, cbd)) break;

        if (vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = vb;
        }
        ctx->DrawIndexed(mesh_index_count(mid), 0, 0);
    }
    backup.restore(ctx);
}
}