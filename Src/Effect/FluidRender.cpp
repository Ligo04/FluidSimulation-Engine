#include <Effect/FluidRender.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>
#include <Graphics/Texture2D.h>

HRESULT FluidRender::Init(ID3D11Device* device,UINT particleNums,int clientWidth,int clientHeight)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    HRESULT hr{};
    hr = ParticlesNumsResize(device,particleNums);
    if (FAILED(hr))
        return hr;
    hr = OnResize(device, clientWidth, clientHeight);
    return hr;
}

HRESULT FluidRender::OnResize(ID3D11Device* device, int clientWidth, int clientHeight)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_ClientWidth = clientWidth;
    m_ClientHeigh = clientHeight;
    
    m_pDepthTextrue.reset();
    m_pThicknessTextrue.reset();
    m_pBlurDepthTexture.reset();
    m_pCurrSeceneTextrue.reset();
    m_pReCalcNormalTextrue.reset();
    m_pEllipsoidDepthTexture.reset();

    
    //创建深度纹理贴图
    m_pDepthTextrue = std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pThicknessTextrue= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pBlurDepthTexture= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pCurrSeceneTextrue= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pReCalcNormalTextrue = std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pEllipsoidDepthTexture = std::make_unique<Depth2D>(device, m_ClientWidth, m_ClientHeigh);

    return S_OK;
}

HRESULT FluidRender::ParticlesNumsResize(ID3D11Device* device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    //重新初始化
    m_pParticlePostionVB.Reset();
    m_pParticleDensityVB.Reset();
    m_pParticleAnsotropyVB.Reset();
    m_pParticleIB.Reset();


    m_ParticleNums = particleNums;
    //创建缓冲区
    HRESULT hr{};

    //用于粒子绘制
    hr = CreateVertexBuffer(device, nullptr, sizeof(DirectX::XMFLOAT3) * m_ParticleNums, m_pParticlePostionVB.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;
    hr = CreateVertexBuffer(device, nullptr, sizeof(float) * m_ParticleNums, m_pParticleDensityVB.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    Anisotropy ani{};
    ani.q1 = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.04f);
    ani.q2 = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.04f);
    ani.q3 = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.04f);
    std::vector<Anisotropy> anis = std::vector<Anisotropy>(m_ParticleNums, ani);
    hr = CreateVertexBuffer(device, anis.data(), sizeof(Anisotropy) * m_ParticleNums, m_pParticleAnsotropyVB.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    //apply blur depth
    MeshVertexIn vertices[4] =
    {
        { {-1.0f, -1.0f, 0.0f},  {0, 1, 0}, {0.0f, 0.0f}, {1, 1, 1, 1}},
        { { 1.0f, -1.0f, 0.0f},  {0, 1, 0}, {1.0f, 0.0f}, {1, 1, 1, 1}},
        { { 1.0f,  1.0f, 0.0f},  {0, 1, 0}, {1.0f, 1.0f}, {1, 1, 1, 1}},
        { {-1.0f,  1.0f, 0.0f},  {0, 1, 0}, {0.0f, 1.0f}, {1, 1, 1, 1}},
    };
    hr = CreateVertexBuffer(device, vertices, sizeof(MeshVertexIn) * 4, m_pQuadVB.GetAddressOf(), false, false);
    if (FAILED(hr))
        return hr;

    //*********************
    //创建粒子顶点索引缓冲区
    //索引数组
    std::vector<DWORD> indices{};
    indices.resize(m_ParticleNums);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        indices[i] = DWORD(i);
    }
    hr = CreateIndexBuffer(device, indices.data(), sizeof(DWORD) * m_ParticleNums, m_pParticleIB.GetAddressOf(), false);

    DWORD quad_indices[4] = { 0, 1, 3, 2 };
    hr = CreateIndexBuffer(device, quad_indices, sizeof(DWORD) * 4, m_pQuadIB.GetAddressOf(), false);
    return hr;
}

void FluidRender::UpdateVertexInputBuffer(ID3D11DeviceContext* deviceContext,ID3D11Buffer* updatePos,
    ID3D11Buffer* updateDensity)
{   
    deviceContext->CopyResource(m_pParticlePostionVB.Get(), updatePos);
    deviceContext->CopyResource(m_pParticleDensityVB.Get(), updateDensity);
}

void FluidRender::CreateParticleDepth(ID3D11DeviceContext* deviceContext,
    FluidEffect& effect, ParticleParams params, int offset)
{

    deviceContext->ClearRenderTargetView(m_pDepthTextrue->GetRenderTarget(), DirectX::Colors::Black);
    effect.SetPointSpriteDepthState(deviceContext);
    //设置深度模板状态
    effect.SetRasterizerState(RenderStates::RSNoCullWithFrontCCW.Get());
    deviceContext->OMSetRenderTargets(1, m_pDepthTextrue->GetRenderTargetComPtr().GetAddressOf(), nullptr);
    
    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);
    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticlePostionVB.GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleDensityVB.GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    //绘制粒子的深度图
    deviceContext->DrawIndexed(m_ParticleNums,offset,0);
}

void FluidRender::CreateEllipsoidDepth(ID3D11DeviceContext* deviceContext, FluidEffect& effect,ID3D11Buffer* anisotropy,ParticleParams params, int offset)
{
    deviceContext->ClearRenderTargetView(m_pDepthTextrue->GetRenderTarget(), DirectX::Colors::Black);
    deviceContext->ClearDepthStencilView(m_pEllipsoidDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    effect.SeEllipsoidDepthState(deviceContext);
    effect.SetRasterizerState(RenderStates::RSNoCullWithFrontCCW.Get());
    effect.SetDepthStencilState(RenderStates::DSSLessEqual.Get(),1);
    ID3D11RenderTargetView* pRTVs[1] = { m_pDepthTextrue->GetRenderTarget() };
    deviceContext->OMSetRenderTargets(1, pRTVs, m_pEllipsoidDepthTexture->GetDepthStencil());

    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);
    effect.SetInvViewPort(params.invViewPort);
    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(Anisotropy) };
    UINT offsets[2] = { 0 ,0 };

   // deviceContext->CopyResource(m_pAnsotropyVB.Get(), anisotropy);
    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticlePostionVB.GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleAnsotropyVB.GetAddressOf(), &strides[1], &offsets[1]);

    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    //绘制粒子的深度图
    deviceContext->DrawIndexed(m_ParticleNums, offset, 0);
}

void FluidRender::CreateParticleThickness(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ParticleParams params, int offset)
{
    deviceContext->ClearRenderTargetView(m_pThicknessTextrue->GetRenderTarget(), DirectX::Colors::Black);
    effect.SetPointSpriteThicknessState(deviceContext);

    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);
    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
    //设置混合状态
    effect.SetRasterizerState(RenderStates::RSNoCullWithFrontCCW.Get());
    static float factor[4] = { 1.0f,1.0f,1.0f,1.0f };
    effect.SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 1);
    effect.SetBlendState(RenderStates::BSAdditive.Get(), factor, 0xffffffff);
    ID3D11RenderTargetView* pRTVs[1] = { m_pThicknessTextrue->GetRenderTarget() };
    deviceContext->OMSetRenderTargets(1, pRTVs, nullptr);

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticlePostionVB.GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleDensityVB.GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    effect.Apply(deviceContext);
    //绘制粒子的厚度图
    deviceContext->DrawIndexed(m_ParticleNums, offset, 0);
}

void FluidRender::DrawParticle(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ParticleParams params, int offset)
{
    effect.SetParticleRenderState(deviceContext);
    effect.SetRasterizerState(RenderStates::RSNoCull.Get());

    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);
    effect.SetDirLight(0, params.dirLight[0]);

    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticlePostionVB.GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleDensityVB.GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    //绘制粒子
    deviceContext->DrawIndexed(m_ParticleNums, offset, 0);
}

void FluidRender::DrawFluid(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ID3D11RenderTargetView* sceneRTV, ID3D11DepthStencilView* sceneDSV,
    ID3D11Buffer* anisotropy,ParticleParams params, int offset)
{
    CreateParticleThickness(deviceContext, effect, params, offset);
    CreateEllipsoidDepth(deviceContext, effect,anisotropy, params,offset);
  
    BlurDepth(deviceContext, effect, params, offset);
    Composite(deviceContext, effect, sceneRTV, sceneDSV, params, offset);
}

void FluidRender::BlurDepth(ID3D11DeviceContext* deviceContext, FluidEffect& effect,ParticleParams params, int offset)
{
    deviceContext->ClearRenderTargetView(m_pBlurDepthTexture->GetRenderTarget(), DirectX::Colors::Black);
    effect.SetBlurDepthState(deviceContext);
    effect.SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 1);
    effect.SetRasterizerState(RenderStates::RSNoCullWithFrontCCW.Get());
    effect.SetTextureDepth(m_pDepthTextrue->GetShaderResource());
    effect.SetBlurRadiusWorld(params.blurRadiusWorld);
    effect.SetBlurScale(params.blurScale);
    effect.SetBlurFalloff(params.blurFalloff);
    effect.SetPointColor(DirectX::XMFLOAT4(0.113f, 0.425f, 0.55f, 1.0f));

    deviceContext->OMSetRenderTargets(1, m_pBlurDepthTexture->GetRenderTargetComPtr().GetAddressOf(), nullptr);

    UINT stride = sizeof(MeshVertexIn);
    UINT offset1 = 0;
    deviceContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset1);
    deviceContext->IASetIndexBuffer(m_pQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    
    effect.Apply(deviceContext);
    deviceContext->DrawIndexed(4, 0, 0);
    //清除状态
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    effect.Apply(deviceContext);
}

void FluidRender::Composite(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ID3D11RenderTargetView* sceneRTV, ID3D11DepthStencilView* sceneDSV,
    ParticleParams params, int offset)
{
    deviceContext->ClearRenderTargetView(m_pReCalcNormalTextrue->GetRenderTarget(), DirectX::Colors::Black);

    ComPtr<ID3D11Resource> res;
    sceneRTV->GetResource(res.GetAddressOf());
    deviceContext->CopyResource(m_pCurrSeceneTextrue->GetTexture(), res.Get());

    effect.SetCompositeState(deviceContext);
    effect.SetRasterizerState(RenderStates::RSNoCullWithFrontCCW.Get());
    effect.SetDepthStencilState(RenderStates::DSSLessEqual.Get(),1);
    effect.SetTextureDepth(m_pBlurDepthTexture->GetShaderResource());
    effect.SetTextureThickness(m_pThicknessTextrue->GetShaderResource());
    effect.SetTextureScene(m_pCurrSeceneTextrue->GetShaderResource());
    effect.SetIor(params.ior);
    effect.SetDirLight(0, params.dirLight[0]);
    effect.SetClipPosToEye(params.clipToEye);
    effect.SetInvTexScale(params.invTexScale);

    ID3D11RenderTargetView* piexlOut[2] =
    {
        sceneRTV,
        m_pReCalcNormalTextrue->GetRenderTarget()
    };

    deviceContext->OMSetRenderTargets(2, piexlOut, sceneDSV);
    UINT stride = sizeof(MeshVertexIn);
    UINT offset1 = 0;
    deviceContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset1);
    deviceContext->IASetIndexBuffer(m_pQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    deviceContext->DrawIndexed(4, 0, 0);
}

void FluidRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pDepthTextrue->SetDebugObjectName(name+":DepthTextrue");
    m_pBlurDepthTexture->SetDebugObjectName(name + ":BlurDepthTexture");
    m_pThicknessTextrue->SetDebugObjectName(name + ":ThicknessTextrue");
    m_pCurrSeceneTextrue->SetDebugObjectName(name + ":CurrSeceneTextrue");
    m_pReCalcNormalTextrue->SetDebugObjectName(name + ":ReCalcNormalTextrue");
    m_pEllipsoidDepthTexture->SetDebugObjectName(name + ":EllipsoidDepthTexture");

    D3D11SetDebugObjectName(m_pParticlePostionVB.Get(), name + ".ParticlePostionVB");
    D3D11SetDebugObjectName(m_pParticleDensityVB.Get(), name + ".ParticleDensityVB");
    D3D11SetDebugObjectName(m_pParticleAnsotropyVB.Get(), name + ".ParticleAnsotropyVB");
    D3D11SetDebugObjectName(m_pParticleIB.Get(), name + ".ParticleIB");
    D3D11SetDebugObjectName(m_pQuadVB.Get(), name + ".QuadVB");
    D3D11SetDebugObjectName(m_pQuadIB.Get(), name + ".QuadIB");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

