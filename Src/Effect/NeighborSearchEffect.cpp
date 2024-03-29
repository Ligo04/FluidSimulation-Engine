#include <Effect/EffectHelper.h> // 必须晚于Effects.h和d3dUtil.h包含
#include <Effect/Effects.h>
#include <Graphics/Vertex.h>
#include <Utils/DXTrace.h>
#include <Utils/d3dUtil.h>

using namespace DirectX;
#pragma warning(disable : 26812)

class NeighborSearchEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;
};

NeighborSearchEffect::NeighborSearchEffect() { m_pImpl = std::make_unique<NeighborSearchEffect::Impl>(); }

NeighborSearchEffect::~NeighborSearchEffect() {}

NeighborSearchEffect::NeighborSearchEffect(NeighborSearchEffect&& moveForm) noexcept
{
    this->m_pImpl.swap(moveForm.m_pImpl);
}

NeighborSearchEffect& NeighborSearchEffect::operator=(NeighborSearchEffect&& moveFrom) noexcept
{
    this->m_pImpl.swap(moveFrom.m_pImpl);
    return *this;
}

bool NeighborSearchEffect::Init(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    m_pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	m_pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"..\\shaders\\generated");
    std::wstring     shader_path = L"..\\shaders\\Fulid\\";
    ComPtr<ID3DBlob> blob;
    EffectPassDesc   passDesc;

#pragma region bounds
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcBounds_CS",
                                                      shader_path + L"CalcBounds_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "CalcBounds_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBounds", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcBoundsGroup_CS",
                                                      shader_path + L"CalcBoundsGroup_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "CalcBoundsGroup_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBoundsGroup", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcBoundsFinalize_CS",
                                                      shader_path + L"CalcBoundsFinalize_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "CalcBoundsFinalize_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBoundsFinalize", device, &passDesc));
#pragma endregion

    // 创建计算着色器
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("CalcHash_CS", shader_path + L"CalcHash_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    // 创建通道
    passDesc.nameCS = "CalcHash_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcHash", device, &passDesc));

    // 创建计算着色器
    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("RadixSortCount_CS",
                                                      shader_path + L"RadixSortCount_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    passDesc.nameCS = "RadixSortCount_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortCount", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("RadixSortCountersPrefix_CS",
                                                      shader_path + L"RadixSortCountersPrefix_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    passDesc.nameCS = "RadixSortCountersPrefix_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortCountersPrefix", device, &passDesc));

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("RadixSortDispatch_CS",
                                                      shader_path + L"RadixSortDispatch_CS.hlsl ",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    passDesc.nameCS = "RadixSortDispatch_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortDispatch", device, &passDesc));

    // find cell start

    HR(m_pImpl->m_pEffectHelper->CreateShaderFromFile("FindCellStart_CS",
                                                      shader_path + L"FindCellStart_CS.hlsl",
                                                      device,
                                                      "CS",
                                                      "cs_5_0",
                                                      nullptr,
                                                      blob.ReleaseAndGetAddressOf()));

    passDesc.nameCS = "FindCellStart_CS";
    HR(m_pImpl->m_pEffectHelper->AddEffectPass("FindCellStart", device, &passDesc));

    return true;
}

void NeighborSearchEffect::SetCalcBoundsState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcBounds");
}

void NeighborSearchEffect::SetCalcBoundsGroupState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcBoundsGroup");
}

void NeighborSearchEffect::SetCalcBoundsFinalizeState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcBoundsFinalize");
}

void NeighborSearchEffect::SetCalcHashState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("CalcHash");
}

void NeighborSearchEffect::SetRadixSortCountState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("RadixSortCount");
}

void NeighborSearchEffect::SetRadixSortCountPrefixState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("RadixSortCountersPrefix");
}

void NeighborSearchEffect::SetRadixSortDispatchState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("RadixSortDispatch");
}

void NeighborSearchEffect::SetFindCellStartAndEndState()
{
    m_pImpl->m_pCurrEffectPass = m_pImpl->m_pEffectHelper->GetEffectPass("FindCellStart");
}

void NeighborSearchEffect::SetInputSRVByName(LPCSTR name, ID3D11ShaderResourceView* srv)
{
    m_pImpl->m_pEffectHelper->SetShaderResourceByName(name, srv);
}

void NeighborSearchEffect::SetOutPutUAVByName(LPCSTR name, ID3D11UnorderedAccessView* uav)
{
    m_pImpl->m_pEffectHelper->SetUnorderedAccessByName(name, uav, 0);
}

void NeighborSearchEffect::SetBoundsGroupNum(UINT boundsGroupNum)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BoundsGroups")->SetUInt(boundsGroupNum);
}

void NeighborSearchEffect::SetBoundsFinalizeNum(UINT boundsGroupNum)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BoundsFinalizeNums")->SetUInt(boundsGroupNum);
}

void NeighborSearchEffect::SetCellSize(float CellSize)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CellSize")->SetFloat(CellSize);
}

void NeighborSearchEffect::SetParticleNums(UINT particleNum)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ParticleNums")->SetUInt(particleNum);
}

void NeighborSearchEffect::SetCurrIteration(int currIteration)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CurrIteration")->SetSInt(currIteration);
}

void NeighborSearchEffect::SetCounterNums(UINT counterNum)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CounterNums")->SetUInt(counterNum);
}

void NeighborSearchEffect::SetKeyNums(UINT keyNums)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_KeyNums")->SetUInt(keyNums);
}

void NeighborSearchEffect::SetBlocksNums(UINT blocksNums)
{
    m_pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlocksNums")->SetUInt(blocksNums);
}

void NeighborSearchEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    if (m_pImpl->m_pCurrEffectPass)
        m_pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
