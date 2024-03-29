#include <Effect/EffectHelper.h> // 必须晚于Effects.h和d3dUtil.h包含
#include <Effect/Effects.h>
#include <Graphics/Vertex.h>
#include <Utils/DXTrace.h>
#include <Utils/d3dUtil.h>

using namespace DirectX;
#pragma warning(disable : 26812)

class PBFSolverEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;
};

PBFSolverEffect::PBFSolverEffect() { m_pImpl = std::make_unique<PBFSolverEffect::Impl>(); }

PBFSolverEffect::~PBFSolverEffect() {}

PBFSolverEffect::PBFSolverEffect(PBFSolverEffect&& moveForm) noexcept { this->m_pImpl.swap(moveForm.m_pImpl); }

PBFSolverEffect& PBFSolverEffect::operator=(PBFSolverEffect&& moveFrom) noexcept
{
    this->m_pImpl.swap(moveFrom.m_pImpl);
    return *this;
}

bool PBFSolverEffect::Init(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    m_pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    m_pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"..\\shaders\\generated");
    std::wstring     shader_path = L"..\\shaders\\Fulid";
    ComPtr<ID3DBlob> blob;
    EffectPassDesc   passDesc;
    // 创建计算着色器
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PredictPosition_CS",
                                                      shader_path + L"PredictPosition_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "PredictPosition_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("PredictPosition", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("ReorderParticle_CS",
                                                      shader_path + L"ReorderParticle_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "ReorderParticle_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("ReorderParticle", device, &passDesc));

#pragma region PBF CS State
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcLagrangeMultiplier_CS",
                                                      shader_path + L"CalcLagrangeMultiplier_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CalcLagrangeMultiplier_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcLagrangeMultiplier", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcDisplacement_CS",
                                                      shader_path + L"CalcDisplacement_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CalcDisplacement_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcDisplacement", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("AddDeltaPosition_CS",
                                                      shader_path + L"AddDeltaPosition_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "AddDeltaPosition_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("AddDeltaPosition", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("UpdateVelocity_CS",
                                                      shader_path + L"UpdateVelocity_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "UpdateVelocity_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("UpdateVelocity", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcVorticity_CS",
                                                      shader_path + L"CalcVorticity_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CalcVorticity_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcVorticity", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("SolverVelocities_CS",
                                                      shader_path + L"SolverVelocities_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "SolverVelocities_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("SolverVelocities", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("PBFFinalize_CS",
                                                      shader_path + L"PBFFinalize_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "PBFFinalize_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("PBFFinalize", device, &passDesc));
#pragma endregion

#pragma region collision
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CollisionParticle_CS",
                                                      shader_path + L"CollisionParticle_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CollisionParticle_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CollisionParticle", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CollisionPlane_CS",
                                                      shader_path + L"CollisionPlane_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CollisionPlane_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CollisionPlane", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("SolveContact_CS",
                                                      shader_path + L"SolveContact_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "SolveContact_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("SolveContact", device, &passDesc));
#pragma endregion

#pragma region Anisotropy
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("SmoothPosition_CS",
                                                      shader_path + L"SmoothPosition_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "SmoothPosition_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("SmoothPosition", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcAnisotropy_CS",
                                                      shader_path + L"CalcAnisotropy_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));
    // 创建通道
    passDesc.nameCS = "CalcAnisotropy_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcAnisotropy", device, &passDesc));
#pragma endregion

    return true;
}

void PBFSolverEffect::SetInputSRVByName(LPCSTR name, ID3D11ShaderResourceView* srv)
{
    m_pImpl->m_pEffectHelper->SetShaderResourceByName(name, srv);
}

void PBFSolverEffect::SetOutPutUAVByName(LPCSTR name, ID3D11UnorderedAccessView* uav)
{
    m_pImpl->m_pEffectHelper->SetUnorderedAccessByName(name, uav, 0);
}

void PBFSolverEffect::SetPredictPositionState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("PredictPosition");
}

void PBFSolverEffect::SetReorderParticleState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("ReorderParticle");
}

void PBFSolverEffect::SetCollisionParticleState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CollisionParticle");
}

void PBFSolverEffect::SetCollisionPlaneState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CollisionPlane");
}

void PBFSolverEffect::SetCalcLagrangeMultiplierState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcLagrangeMultiplier");
}

void PBFSolverEffect::SetCalcDisplacementState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcDisplacement");
}

void PBFSolverEffect::SetADDDeltaPositionState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("AddDeltaPosition");
}

void PBFSolverEffect::SetCalcVorticityState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcVorticity");
}

void PBFSolverEffect::SetSolverVelocitiesState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("SolverVelocities");
}

void PBFSolverEffect::SetPBFFinalizeState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("PBFFinalize");
}

void PBFSolverEffect::SetUpdateVelocityState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("UpdateVelocity");
}

void PBFSolverEffect::SetSolverContactState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("SolveContact");
}

void PBFSolverEffect::SetSmoothPositionState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("SmoothPosition");
}

void PBFSolverEffect::SetCalcAnisotropyState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcAnisotropy");
}

void PBFSolverEffect::SetPlaneNums(int wallNums)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PlaneNums")->SetSInt(wallNums);
}

void PBFSolverEffect::SetPlane(size_t pos, const DirectX::XMFLOAT4& plane)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Plane")->SetRaw(
        &plane, (UINT)pos * sizeof(plane), sizeof(plane));
}

void PBFSolverEffect::SetParticleNums(UINT particleNums)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ParticleNums")->SetUInt(particleNums);
}

void PBFSolverEffect::SetCollisionDistance(float distance)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CollisionDistance")->SetFloat(distance);
}

void PBFSolverEffect::SetCollisionThreshold(float threshold)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CollisionThreshold")->SetFloat(threshold);
}

void PBFSolverEffect::SetDeltaTime(float deltatime)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DeltaTime")->SetFloat(deltatime);
}

void PBFSolverEffect::SetCellSize(float CellSize)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CellSize")->SetFloat(CellSize);
}

void PBFSolverEffect::SetInverseDeltaTime(float invDt)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InverseDeltaTime")->SetFloat(invDt);
}

void PBFSolverEffect::SetPloy6Coff(float ploy6Coff)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Poly6Coff")->SetFloat(ploy6Coff);
}

void PBFSolverEffect::SetSpikyCoff(float spikyCoff)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SpikyCoff")->SetFloat(spikyCoff);
}

void PBFSolverEffect::SetSpikyGradCoff(float spikyGardCoff)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SpikyGradCoff")->SetFloat(spikyGardCoff);
}

void PBFSolverEffect::SetViscosityCoff(float viscosityCoff)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViscosityCoff")->SetFloat(viscosityCoff);
}

void PBFSolverEffect::SetLambadEps(float lambdaEps)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LambdaEps")->SetFloat(lambdaEps);
}

void PBFSolverEffect::SetDeltaQ(float deltaQ)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DeltaQ")->SetFloat(deltaQ);
}

void PBFSolverEffect::SetScorrK(float scorrK)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ScorrK")->SetFloat(scorrK);
}

void PBFSolverEffect::SetScorrN(float scorrN)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ScorrN")->SetFloat(scorrN);
}

void PBFSolverEffect::SetVorticityConfinement(float vorticityConfinement)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VorticityConfinement")->SetFloat(vorticityConfinement);
}

void PBFSolverEffect::SetVorticityC(float vorticityC)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VorticityC")->SetFloat(vorticityC);
}

void PBFSolverEffect::SetInverseDensity_0(float inverseDensity_0)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InverseDensity_0")->SetFloat(inverseDensity_0);
}

void PBFSolverEffect::SetSphSmoothLength(float sphSmoothLength)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_sphSmoothLength")->SetFloat(sphSmoothLength);
}

void PBFSolverEffect::SetGravity(const DirectX::XMFLOAT3& gravity)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Gravity")->SetFloatVector(3, (FLOAT*)&gravity);
}

void PBFSolverEffect::SetMaxCollisionPlanes(int max)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxCollisionPlanes")->SetSInt(max);
}

void PBFSolverEffect::SetMaxNeighBorPerParticle(int max)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxNeighborPerParticle")->SetSInt(max);
}

void PBFSolverEffect::SetParticleRadiusSq(float radiussq)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ParticleRadiusSq")->SetFloat(radiussq);
}

void PBFSolverEffect::SetSOR(float sor) { m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SOR")->SetFloat(sor); }

void PBFSolverEffect::SetMaxSpeed(float maxspeed)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxSpeed")->SetFloat(maxspeed);
}

void PBFSolverEffect::SetMaxVelocityDelta(float elocityDelta)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxVeclocityDelta")->SetFloat(elocityDelta);
}

void PBFSolverEffect::SetRestituion(float restituion)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Restituion")->SetFloat(restituion);
}

void PBFSolverEffect::SetLaplacianSmooth(float smooth)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LaplacianSmooth")->SetFloat(smooth);
}

void PBFSolverEffect::SetAnisotropyScale(float anisotropyScale)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AnisotropyScale")->SetFloat(anisotropyScale);
}

void PBFSolverEffect::SetAnisotropyMin(float anisotropyMin)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AnisotropyMin")->SetFloat(anisotropyMin);
}

void PBFSolverEffect::SetAnisotropyMax(float anisotropyMax)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AnisotropyMax")->SetFloat(anisotropyMax);
}

void PBFSolverEffect::SetStaticFriction(float staticFriction)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_StaticFriction")->SetFloat(staticFriction);
}

void PBFSolverEffect::SetDynamicFriction(float dynamicFriction)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DynamicFriction")->SetFloat(dynamicFriction);
}

void PBFSolverEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    if (m_pImpl->m_pCurrEffectPass)
        m_pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
