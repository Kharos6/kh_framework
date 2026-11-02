#pragma once

// The feature half (rendering_integration_features.hpp): the white placeholder (KH_WHITE_PREVIEW): its shaders and its
// draw. Never include it on its own.
namespace RenderIntegration {

// KH_WHITE_PREVIEW: the placeholder pair (kh_white_draw draws the world meshes flat white while the shader pool
// compiles). THREE call sites, all in ensure_resources: the two deferral returns (the async hold and the prewarm
// gate) and the success path, so a warm-cache session that defers nothing still creates it. g_res.white_tried caps
// it at one compile attempt per device. Non-fatal: a failure leaves it null.

inline void kh_white_ensure(ID3D11Device* dev) {
    if (!dev || g_res.white_tried) return;
    g_res.white_tried = true;
    static const D3D_SHADER_MACRO khw_defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr },
    };
    const std::string khw_src = kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_WHITE).c_str();
    ID3DBlob* khw_vsb = nullptr;
    ID3DBlob* khw_psb = nullptr;
    std::string khw_err = compile_shader(khw_src.c_str(), "VSWhite", "vs_5_0", khw_defines, &khw_vsb);
    if (khw_err.empty())
        khw_err = compile_shader(khw_src.c_str(), "PSWhite", "ps_5_0", khw_defines, &khw_psb);
    if (!khw_err.empty() || !khw_vsb || !khw_psb) {
        if (khw_vsb) khw_vsb->Release();
        if (khw_psb) khw_psb->Release();
        report_error_once_safe("KH white preview: " + (khw_err.empty() ? std::string("no blob") : khw_err));
        return;
    }
    // khw_step / khw_hr: the last attempted call and its result, for the one report below.
    const char* khw_step = "none";
    HRESULT khw_hr = S_OK;

    khw_step = "CreateVertexShader";
    khw_hr = dev->CreateVertexShader(khw_vsb->GetBufferPointer(), khw_vsb->GetBufferSize(),
                                     nullptr, &g_res.vs_white);
    if (SUCCEEDED(khw_hr)) {
        khw_step = "CreatePixelShader";
        khw_hr = dev->CreatePixelShader(khw_psb->GetBufferPointer(), khw_psb->GetBufferSize(),
                                        nullptr, &g_res.ps_white);
    }
    if (SUCCEEDED(khw_hr)) {
        // VSWhite takes VSIn (position + normal); the body reads i.pos alone.
        const D3D11_INPUT_ELEMENT_DESC khw_il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        khw_step = "CreateInputLayout";
        khw_hr = dev->CreateInputLayout(khw_il, 2u, khw_vsb->GetBufferPointer(), khw_vsb->GetBufferSize(), &g_res.layout_white);
    }
    khw_vsb->Release();
    khw_psb->Release();
    if (SUCCEEDED(khw_hr)) {
        D3D11_BUFFER_DESC khw_bd = {};
        khw_bd.ByteWidth = static_cast<UINT>(KH_CBOBJ_BYTES);
        khw_bd.Usage = D3D11_USAGE_DYNAMIC;
        khw_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        khw_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        khw_step = "CreateBuffer(obj)";
        khw_hr = dev->CreateBuffer(&khw_bd, nullptr, &g_res.white_obj_cb);
        if (SUCCEEDED(khw_hr)) {
            khw_bd.ByteWidth = static_cast<UINT>(KH_CBFRAME_BYTES);
            khw_step = "CreateBuffer(frame)";
            khw_hr = dev->CreateBuffer(&khw_bd, nullptr, &g_res.white_frame_cb);
        }
    }
    if (SUCCEEDED(khw_hr)) {
        D3D11_RASTERIZER_DESC khw_rd = {};
        khw_rd.FillMode = D3D11_FILL_SOLID;
        khw_rd.CullMode = D3D11_CULL_NONE;
        khw_rd.MultisampleEnable = TRUE;
        khw_rd.DepthClipEnable = FALSE;   // The mesh rasterizer's contract.
        khw_rd.DepthBias = 0;
        khw_rd.SlopeScaledDepthBias = 0.0f;
        khw_rd.DepthBiasClamp = 0.0f;
        khw_step = "CreateRasterizerState";
        khw_hr = dev->CreateRasterizerState(&khw_rd, &g_res.rast_white);
        if (SUCCEEDED(khw_hr)) {
            D3D11_DEPTH_STENCIL_DESC khw_dd = {};
            khw_dd.DepthEnable = TRUE;
            khw_dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            khw_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            khw_dd.StencilEnable = FALSE;
            khw_step = "CreateDepthStencilState";
            khw_hr = dev->CreateDepthStencilState(&khw_dd, &g_res.dss_white);
        }
    }
    if (FAILED(khw_hr)) {
        KH_SAFE_RELEASE(g_res.vs_white);
        KH_SAFE_RELEASE(g_res.ps_white);
        KH_SAFE_RELEASE(g_res.layout_white);
        KH_SAFE_RELEASE(g_res.white_obj_cb);
        KH_SAFE_RELEASE(g_res.white_frame_cb);
        KH_SAFE_RELEASE(g_res.rast_white);
        KH_SAFE_RELEASE(g_res.dss_white);
        report_error_once_safe(std::string("KH white preview: ") + khw_step + " " + hr_str(khw_hr) +
                        " (preview stands down)");
        return;
    }
    g_res.white_ready = true;
}

inline void kh_white_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    if (!dev || !ctx || !g_res.white_ready) return;
    if (kh_fsaa_world_standdown()) return;   // FSAA 1x: the placeholder is a world mesh too.
    // Runs inside the flush's serialized window; a failure costs the objects whose buffers are missing, not the pass.
    ensure_mesh_vbs(dev);
    if (g_res.mesh_vb.empty()) return;

    kh_scene_sync();   // KH_SCENE.
    if (g_scene.alive_n == 0) return;

    const float khw_now = effect_time_seconds();
    RVExtBridge::ProjectionViewTransform khw_pv = {};
    if (!RVExtBridge::get_projection_view_transform(khw_pv)) return;

    ConstantData khw_cbf = {};
    mul_4x4(khw_pv.view, khw_pv.projection, khw_cbf.view_proj);
    if (!kh_upload_frame_cb(ctx, g_res.white_frame_cb, khw_cbf)) return;

    StateBackup khw_backup;
    khw_backup.capture(ctx);
    KhOmSlot0 khw_s0;   // KH_MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    khw_s0.open(ctx);

    UINT khw_stride = sizeof(MeshVertex);
    KhVbRef khw_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
    const FLOAT khw_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->IASetInputLayout(g_res.layout_white);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_white, nullptr, 0);
    ctx->PSSetShader(g_res.ps_white, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* khw_cbs[2] = { g_res.white_obj_cb, g_res.white_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khw_cbs);
    ctx->PSSetConstantBuffers(0, 2, khw_cbs);
    ctx->RSSetState(g_res.rast_white);
    ctx->OMSetDepthStencilState(g_res.dss_white, 0);
    // KH_WSEE: the placeholder writes the main depth (in the flush: past any capture).
    kh_wsee_taint(g_topo_cycles);
    ctx->OMSetBlendState(nullptr, khw_bf, 0xFFFFFFFF);

    for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
        if (!g_scene.alive[khsc_i]) continue;
        const RenderObject& khw_o = g_scene.objs[khsc_i];
        if (khw_o.fullscreen || kh_in_front(khw_o)) continue;   // KH_INFRONT: never in the world depth.
        // KH_WHITE_NOFX: an effect mesh has no solid look to stand in for and never writes depth (KH_FX_NODEPTH);
        // drawn here it is an opaque white wall - the whole screen for a volume around the camera.
        if (khw_o.effect != 0) continue;
        if (!kh_obj_shown(khw_o)) continue;   // Hidden: no pixel (casterOnly: it casts, in the maps alone).
        bool khw_expired = false;
        if (lifetime_envelope(khw_o, khw_now, khw_expired) <= 0.0f || khw_expired) continue;
        const int khw_mid = mesh_id_clamp(khw_o.mesh);
        if (khw_mid < 0 || static_cast<size_t>(khw_mid) >= g_res.mesh_vb.size()) continue;
        const KhVbRef khw_vb = kh_mesh_vb_ref(khw_mid, khsc_i);   // KH_CLOTH.
        if (!khw_vb) continue;

        ConstantData khw_cbd = {};
        khw_cbd.center_size[0] = khw_o.pos[0];
        khw_cbd.center_size[1] = khw_o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        khw_cbd.center_size[2] = khw_o.pos[1];
        khw_cbd.size_axes[0] = khw_o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        khw_cbd.size_axes[1] = khw_o.size[2];
        khw_cbd.size_axes[2] = khw_o.size[1];
        kh_fill_obj_rot(khw_cbd, khw_o.rot_m);
        // VSWhite takes the absolute branch, matching the unrebased viewProj built above; the pair must agree.
        if (!kh_upload_obj_cb(ctx, g_res.white_obj_cb, khw_cbd)) break;

        if (khw_vb != khw_bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khw_vb.vb, &khw_stride, &khw_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[khw_mid], DXGI_FORMAT_R32_UINT, 0);
            khw_bound_vb = khw_vb;
        }
        ctx->DrawIndexed(mesh_index_count(khw_mid), 0, 0);
    }
    khw_backup.restore(ctx);
}
}