#include <Effect/EffectHelper.h> // 必须晚于Effects.h和d3dUtil.h包含
#include <Effect/Effects.h>
#include <Graphics/Vertex.h>
#include <Utils/DXTrace.h>
#include <Utils/d3dUtil.h>

using namespace DirectX;
#pragma warning(disable : 26812)

class FluidEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;

    ComPtr<ID3D11InputLayout> m_pParticleLayout;
    ComPtr<ID3D11InputLayout> m_pPassThoughLayout;
    ComPtr<ID3D11InputLayout> m_pFluidVertexInLayout;

    XMFLOAT4X4 m_World {}, m_View {}, m_Proj {};
    UINT       m_MsaaSamples = 1;
};

FluidEffect::FluidEffect() { m_pImpl = std::make_unique<FluidEffect::Impl>(); }

FluidEffect::~FluidEffect() {}

FluidEffect::FluidEffect(FluidEffect&& moveForm) noexcept { this->m_pImpl.swap(moveForm.m_pImpl); }

FluidEffect& FluidEffect::operator=(FluidEffect&& moveFrom) noexcept
{
    this->m_pImpl.swap(moveFrom.m_pImpl);
    return *this;
}

bool FluidEffect::Init(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    m_pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();
    m_pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"..\\shaders\\generated");
    std::wstring     shader_path = L"..\\shaders\\";
    ComPtr<ID3DBlob> blob;
    EffectPassDesc   passDesc;

    //******************
    // 创建顶点着色器

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PointSprite_VS",
                                                      shader_path + L"PointSprite_VS.hlsl",
                                                      device,
                                                      "VS",
                                                      "vs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(PointVertexIn::inputLayout,
                                 ARRAYSIZE(PointVertexIn::inputLayout),
                                 blob->GetBufferPointer(),
                                 blob->GetBufferSize(),
                                 m_pImpl->m_pParticleLayout.GetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PassThrough_VS",
                                                      shader_path + L"PassThrough_VS.hlsl",
                                                      device,
                                                      "VS",
                                                      "vs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(MeshVertexIn::inputLayout,
                                 ARRAYSIZE(MeshVertexIn::inputLayout),
                                 blob->GetBufferPointer(),
                                 blob->GetBufferSize(),
                                 m_pImpl->m_pPassThoughLayout.GetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("EllipsoidDepth_VS",
                                                      shader_path + L"EllipsoidDepth_VS.hlsl",
                                                      device,
                                                      "VS",
                                                      "vs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(FluidVertexIn::inputLayout,
                                 ARRAYSIZE(FluidVertexIn::inputLayout),
                                 blob->GetBufferPointer(),
                                 blob->GetBufferSize(),
                                 m_pImpl->m_pFluidVertexInLayout.GetAddressOf()));

    //******************
    // 创建几何着色器(不用于流输出)
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PointSprite_GS",
                                                      shader_path + L"PointSprite_GS.hlsl",
                                                      device,
                                                      "GS",
                                                      "gs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("EllipsoidDepth_GS",
                                                      shader_path + L"EllipsoidDepth_GS.hlsl",
                                                      device,
                                                      "GS",
                                                      "gs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    //******************
    // 创建像素着色器
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PointSpriteDepth_PS",
                                                      shader_path + L"PointSpriteDepth_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("EllipsoidDepth_PS",
                                                      shader_path + L"EllipsoidDepth_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PointSpriteThickness_PS",
                                                      shader_path + L"PointSpriteThickness_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PointSprite_PS",
                                                      shader_path + L"PointSprite_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("BlurDepth_PS",
                                                      shader_path + L"BlurDepth_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("Composite_PS",
                                                      shader_path + L"Composite_PS.hlsl",
                                                      device,
                                                      "PS",
                                                      "ps_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    m_pImpl->m_pEffectHelper->SetSamplerStateByName("g_TexSampler", RenderStates::SSClampWrap.Get());

    // ******************
    // 创建通道
    //
    passDesc.nameVS = "PointSprite_VS";
    passDesc.nameGS = "PointSprite_GS";
    passDesc.namePS = "PointSpriteDepth_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("PointSpriteDepth", device, &passDesc));

    passDesc.nameVS = "PointSprite_VS";
    passDesc.nameGS = "PointSprite_GS";
    passDesc.namePS = "PointSpriteThickness_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("PointSpriteThickness", device, &passDesc));

    passDesc.nameVS = "PointSprite_VS";
    passDesc.nameGS = "PointSprite_GS";
    passDesc.namePS = "PointSprite_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("PointSpriteRender", device, &passDesc));

    passDesc.nameVS = "PassThrough_VS";
    passDesc.nameGS = "";
    passDesc.namePS = "BlurDepth_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("BlurDepth", device, &passDesc));

    passDesc.nameVS = "PassThrough_VS";
    passDesc.nameGS = "";
    passDesc.namePS = "Composite_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("Composite", device, &passDesc));

    passDesc.nameVS = "EllipsoidDepth_VS";
    passDesc.nameGS = "EllipsoidDepth_GS";
    passDesc.namePS = "EllipsoidDepth_PS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("EllipsoidDepth", device, &passDesc));
    return true;
}

void FluidEffect::SetPointSpriteDepthState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("PointSpriteDepth");
    deviceContext->IASetInputLayout(m_pImpl->m_pParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void FluidEffect::SeEllipsoidDepthState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("EllipsoidDepth");
    deviceContext->IASetInputLayout(m_pImpl->m_pFluidVertexInLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void FluidEffect::SetPointSpriteThicknessState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("PointSpriteThickness");
    deviceContext->IASetInputLayout(m_pImpl->m_pParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void FluidEffect::SetParticleRenderState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("PointSpriteRender");
    deviceContext->IASetInputLayout(m_pImpl->m_pParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void FluidEffect::SetBlurDepthState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("BlurDepth");
    deviceContext->IASetInputLayout(m_pImpl->m_pPassThoughLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void FluidEffect::SetCompositeState(ID3D11DeviceContext* deviceContext)
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("Composite");
    deviceContext->IASetInputLayout(m_pImpl->m_pPassThoughLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void FluidEffect::SetTextureDepth(ID3D11ShaderResourceView* textureDepth)
{
    m_pImpl->m_pEffectHelper->SetShaderResourceByName("g_DepthTexture", textureDepth);
}

void FluidEffect::SetTextureThickness(ID3D11ShaderResourceView* textureThickness)
{
    m_pImpl->m_pEffectHelper->SetShaderResourceByName("g_ThickenessTexture", textureThickness);
}

void FluidEffect::SetTextureScene(ID3D11ShaderResourceView* textureScene)
{
    m_pImpl->m_pEffectHelper->SetShaderResourceByName("g_SceneTexture", textureScene);
}

void FluidEffect::SetDepthStencilState(ID3D11DepthStencilState* depthStenState, UINT stencileValue)
{
    m_pImpl->m_pCurrEffectPass->SetDepthStencilState(depthStenState, stencileValue);
}

void FluidEffect::SetRasterizerState(ID3D11RasterizerState* rsState)
{
    m_pImpl->m_pCurrEffectPass->SetRasterizerState(rsState);
}

void FluidEffect::SetBlendState(ID3D11BlendState* blendState, const float* factor, UINT mask)
{
    m_pImpl->m_pCurrEffectPass->SetBlendState(blendState, factor, mask);
}

void FluidEffect::SetDirLight(size_t pos, const DirectionalLight& dirLight)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight")
        ->SetRaw(&dirLight, (UINT)pos * sizeof(dirLight), sizeof(dirLight));
}

void XM_CALLCONV FluidEffect::SetWorldMatrix(DirectX::FXMMATRIX W) { XMStoreFloat4x4(&m_pImpl->m_World, W); }

void XM_CALLCONV FluidEffect::SetViewMatrix(DirectX::FXMMATRIX V) { XMStoreFloat4x4(&m_pImpl->m_View, V); }

void XM_CALLCONV FluidEffect::SetProjMatrix(DirectX::FXMMATRIX P) { XMStoreFloat4x4(&m_pImpl->m_Proj, P); }

void FluidEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (FLOAT*)&eyePos);
}

void FluidEffect::SetPointColor(const DirectX::XMFLOAT4& color)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Color")->SetFloatVector(4, (FLOAT*)&color);
}

void FluidEffect::SetClipPosToEye(const DirectX::XMFLOAT4& clipPosToEye)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_clipPosToEye")->SetFloatVector(4, (FLOAT*)&clipPosToEye);
}

void FluidEffect::SetInvTexScale(const DirectX::XMFLOAT4& invTexScale)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InvTexScale")->SetFloatVector(4, (FLOAT*)&invTexScale);
}

void FluidEffect::SetInvViewPort(const DirectX::XMFLOAT4& invViewPort)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InvViewport")->SetFloatVector(4, (FLOAT*)&invViewPort);
}

void FluidEffect::SetPointRadius(float radius)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PointRadius")->SetFloat(radius);
}

void FluidEffect::SetPointScale(float scale)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PointScale")->SetFloat(scale);
}

void FluidEffect::SetBlurRadiusWorld(float blurRaiudsWorld)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurRadiusWorld")->SetFloat(blurRaiudsWorld);
}

void FluidEffect::SetBlurScale(float blurScale)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurScale")->SetFloat(blurScale);
}

void FluidEffect::SetBlurFalloff(float blurFalloff)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurFalloff")->SetFloat(blurFalloff);
}

void FluidEffect::SetIor(float ior) { m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Ior")->SetFloat(ior); }

void FluidEffect::SetDebugObjectName(const std::string& name) {}

void FluidEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&m_pImpl->m_World);
    XMMATRIX V = XMLoadFloat4x4(&m_pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&m_pImpl->m_Proj);

    XMMATRIX MV    = W * V;
    XMMATRIX MVP   = W * V * P;
    XMMATRIX PINV  = XMMatrixInverse(nullptr, P);
    XMMATRIX MVINV = XMMatrixInverse(nullptr, MV);

    MV    = XMMatrixTranspose(MV);
    MVINV = XMMatrixTranspose(MVINV);
    W     = XMMatrixTranspose(W);
    V     = XMMatrixTranspose(V);
    P     = XMMatrixTranspose(P);
    PINV  = XMMatrixTranspose(PINV);
    MVP   = XMMatrixTranspose(MVP);

    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_View")->SetFloatMatrix(4, 4, (FLOAT*)&V);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (FLOAT*)&P);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ProjInv")->SetFloatMatrix(4, 4, (FLOAT*)&PINV);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (FLOAT*)&MV);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewInv")->SetFloatMatrix(4, 4, (FLOAT*)&MVINV);
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WVP")->SetFloatMatrix(4, 4, (FLOAT*)&MVP);

    if (m_pImpl->m_pCurrEffectPass)
        m_pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
