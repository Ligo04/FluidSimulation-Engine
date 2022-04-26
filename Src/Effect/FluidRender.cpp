#include <Effect/FluidRender.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>


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
    //创建深度纹理贴图
    m_pDepthTextrue = std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pThicknessTextrue= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pBlurDepthTexture= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R32G32B32A32_FLOAT);
    m_pCurrSeceneTextrue= std::make_unique<Texture2D>(device, m_ClientWidth, m_ClientHeigh, DXGI_FORMAT_R8G8B8A8_UNORM);
    return S_OK;
}

HRESULT FluidRender::ParticlesNumsResize(ID3D11Device* device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    //重新初始化
    m_pParticleVB[0].Reset();
    m_pParticleVB[1].Reset();
    m_pParticleIB.Reset();
    m_ParticleNums = particleNums;
    //创建缓冲区
    HRESULT hr{};

    //用于粒子绘制
    hr = CreateVertexBuffer(device, nullptr, sizeof(DirectX::XMFLOAT3) * m_ParticleNums, m_pParticleVB[0].GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;
    hr = CreateVertexBuffer(device, nullptr, sizeof(float) * m_ParticleNums, m_pParticleVB[1].GetAddressOf(), true, false);
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
    deviceContext->CopyResource(m_pParticleVB[0].Get(), updatePos);
    deviceContext->CopyResource(m_pParticleVB[1].Get(), updateDensity);
}

void FluidRender::CreateParticleDepth(ID3D11DeviceContext* deviceContext,
    FluidEffect& effect, ParticleParams params, int offset)
{

    deviceContext->ClearRenderTargetView(m_pDepthTextrue->GetRenderTarget(), DirectX::Colors::Black);
    effect.SetPointSpriteDepthState(deviceContext);
    //设置深度模板状态
    effect.SetRasterizerState(RenderStates::RSCullClockWise.Get());
    deviceContext->OMSetRenderTargets(1, m_pDepthTextrue->GetRenderTargetComPtr().GetAddressOf(), nullptr);
    
    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);
    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticleVB[0].GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleVB[1].GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    //绘制粒子的深度图
    deviceContext->DrawIndexed(m_ParticleNums,offset,0);
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
    effect.SetRasterizerState(RenderStates::RSCullClockWise.Get());
    static float factor[4] = { 1.0f,1.0f,1.0f,1.0f };
    effect.SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 1);
    effect.SetBlendState(RenderStates::BSAdditive.Get(), factor, 0xffffffff);
    deviceContext->OMSetRenderTargets(1, m_pThicknessTextrue->GetRenderTargetComPtr().GetAddressOf(), nullptr);

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticleVB[0].GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleVB[1].GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    effect.Apply(deviceContext);
    //绘制粒子的厚度图
    deviceContext->DrawIndexed(m_ParticleNums, offset, 0);
}

void FluidRender::DrawParticle(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ParticleParams params, int offset)
{
    effect.SetParticleRenderState(deviceContext);
    effect.SetRasterizerState(RenderStates::RSCullClockWise.Get());

    effect.SetPointColor(params.color);
    effect.SetPointRadius(params.RestDistance * 0.5f);
    effect.SetPointScale(params.scale);

    effect.SetWorldMatrix(DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));

    UINT strides[2] = { sizeof(DirectX::XMFLOAT3),sizeof(float) };
    UINT offsets[2] = { 0 ,0 };

    //顶点缓冲区
    deviceContext->IASetVertexBuffers(0, 1, m_pParticleVB[0].GetAddressOf(), &strides[0], &offsets[0]);
    deviceContext->IASetVertexBuffers(1, 1, m_pParticleVB[1].GetAddressOf(), &strides[1], &offsets[1]);
    //索引缓冲区
    deviceContext->IASetIndexBuffer(m_pParticleIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    //绘制粒子
    deviceContext->DrawIndexed(m_ParticleNums, offset, 0);
}

void FluidRender::DrawFluid(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ID3D11RenderTargetView* sceneRTV, ParticleParams params, int offset)
{
    CreateParticleThickness(deviceContext, effect, params, offset);
    CreateParticleDepth(deviceContext, effect, params, offset);
    BlurDepth(deviceContext, effect, params, offset);
    Composite(deviceContext, effect, sceneRTV, params, offset);
}

void FluidRender::BlurDepth(ID3D11DeviceContext* deviceContext, FluidEffect& effect,ParticleParams params, int offset)
{
    deviceContext->ClearRenderTargetView(m_pBlurDepthTexture->GetRenderTarget(), DirectX::Colors::White);
    effect.SetBlurDepthState(deviceContext);
    effect.SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 1);
    effect.SetRasterizerState(RenderStates::RSNoCull.Get());
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

void FluidRender::Composite(ID3D11DeviceContext* deviceContext, FluidEffect& effect, ID3D11RenderTargetView* sceneRTV, ParticleParams params, int offset)
{
    ComPtr<ID3D11Resource> res;
    sceneRTV->GetResource(res.GetAddressOf());
    deviceContext->CopyResource(m_pCurrSeceneTextrue->GetTexture(), res.Get());

    effect.SetCompositeState(deviceContext);
    effect.SetRasterizerState(RenderStates::RSCullClockWise.Get());
    effect.SetTextureDepth(m_pBlurDepthTexture->GetShaderResource());
    effect.SetTextureThickness(m_pThicknessTextrue->GetShaderResource());
    effect.SetTextureScene(m_pCurrSeceneTextrue->GetShaderResource());
    effect.SetIor(params.ior);
    effect.SetClipPosToEye(params.clipToEye);
    effect.SetInvTexScale(params.invTexScale);

    deviceContext->OMSetRenderTargets(1, &sceneRTV, nullptr);
    UINT stride = sizeof(MeshVertexIn);
    UINT offset1 = 0;
    deviceContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset1);
    deviceContext->IASetIndexBuffer(m_pQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);

    effect.Apply(deviceContext);
    deviceContext->DrawIndexed(4, 0, 0);

}

