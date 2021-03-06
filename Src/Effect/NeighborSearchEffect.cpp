#include <Graphics/Vertex.h>
#include <Effect/Effects.h>
#include <Utils/d3dUtil.h>
#include <Effect/EffectHelper.h>	// 必须晚于Effects.h和d3dUtil.h包含
#include <Utils/DXTrace.h>

using namespace DirectX;
# pragma warning(disable: 26812)

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


NeighborSearchEffect::NeighborSearchEffect()
{
	m_pImpl = std::make_unique<NeighborSearchEffect::Impl>();
}


NeighborSearchEffect::~NeighborSearchEffect()
{

}

NeighborSearchEffect::NeighborSearchEffect(NeighborSearchEffect&& moveForm) noexcept
{
	this->m_pImpl.swap(moveForm.m_pImpl);
}

NeighborSearchEffect& NeighborSearchEffect::operator=(NeighborSearchEffect&& moveFrom) noexcept
{
	this->m_pImpl.swap(moveFrom.m_pImpl);
	return *this;
}

bool NeighborSearchEffect::Init(ID3D11Device* device, const std::wstring& effectPath)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	m_pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();


	ComPtr<ID3DBlob> blob;
	EffectPassDesc passDesc;

#pragma region bounds
	std::wstring CalcBoundscsHlsl = effectPath + L"CalcBounds_CS.hlsl";
	std::wstring CalcBoundscsCso = effectPath + L"CalcBounds_CS.cso";
	HR(CreateShaderFromFile(CalcBoundscsCso.c_str(), CalcBoundscsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("CalcBounds_CS", device, blob.Get()));

	//创建通道
	passDesc.nameCS = "CalcBounds_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBounds", device, &passDesc));

	std::wstring CalcBoundsGroupcsHlsl = effectPath + L"CalcBoundsGroup_CS.hlsl";
	std::wstring CalcBoundsGroupcsCso = effectPath + L"CalcBoundsGroup_CS.cso";
	HR(CreateShaderFromFile(CalcBoundsGroupcsCso.c_str(), CalcBoundsGroupcsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("CalcBoundsGroup_CS", device, blob.Get()));

	//创建通道
	passDesc.nameCS = "CalcBoundsGroup_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBoundsGroup", device, &passDesc));


	std::wstring CalcBoundsFinalizecsHlsl = effectPath + L"CalcBoundsFinalize_CS.hlsl";
	std::wstring CalcBoundsFinalizecsCso = effectPath + L"CalcBoundsFinalize_CS.cso";
	HR(CreateShaderFromFile(CalcBoundsFinalizecsCso.c_str(), CalcBoundsFinalizecsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("CalcBoundsFinalize_CS", device, blob.Get()));

	//创建通道
	passDesc.nameCS = "CalcBoundsFinalize_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcBoundsFinalize", device, &passDesc));
#pragma endregion



	//创建计算着色器
	std::wstring calcHashcsHlsl = effectPath + L"CalcHash_CS.hlsl";
	std::wstring calcHashcsCso = effectPath + L"CalcHash_CS.cso";
	HR(CreateShaderFromFile(calcHashcsCso.c_str(), calcHashcsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("CalcHash_CS", device, blob.Get()));

	//创建通道
	passDesc.nameCS = "CalcHash_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("CalcHash", device, &passDesc));



	//创建计算着色器
	std::wstring radixSortCountCsHlsl = effectPath + L"RadixSortCount_CS.hlsl";
	std::wstring radixSortCountCsCso = effectPath + L"RadixSortCount_CS.cso";

	HR(CreateShaderFromFile(radixSortCountCsCso.c_str(), radixSortCountCsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("RadixSortCount_CS", device, blob.Get()));


	passDesc.nameCS = "RadixSortCount_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortCount", device, &passDesc));



	std::wstring radixSortCountersPrefixCsHlsl = effectPath + L"RadixSortCountersPrefix_CS.hlsl";
	std::wstring RadixSortCountersPrefixCsCso = effectPath + L"RadixSortCountersPrefix_CS.cso";

	HR(CreateShaderFromFile(RadixSortCountersPrefixCsCso.c_str(), radixSortCountersPrefixCsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("RadixSortCountersPrefix_CS", device, blob.Get()));


	passDesc.nameCS = "RadixSortCountersPrefix_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortCountersPrefix", device, &passDesc));


	std::wstring radixSortDispatchCsHlsl = effectPath + L"RadixSortDispatch_CS.hlsl";
	std::wstring radixSortDispatchCsCso = effectPath + L"RadixSortDispatch_CS.cso";

	HR(CreateShaderFromFile(radixSortDispatchCsCso.c_str(), radixSortDispatchCsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("RadixSortDispatch_CS", device, blob.Get()));


	passDesc.nameCS = "RadixSortDispatch_CS";
	HR(m_pImpl->m_pEffectHelper->AddEffectPass("RadixSortDispatch", device, &passDesc));

	//find cell start

	std::wstring findCellStartcsHlsl = effectPath + L"FindCellStart_CS.hlsl";
	std::wstring findCellStartcsCso = effectPath + L"FindCellStart_CS.cso";

	HR(CreateShaderFromFile(findCellStartcsCso.c_str(), findCellStartcsHlsl.c_str(), "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pImpl->m_pEffectHelper->AddShader("FindCellStart_CS", device, blob.Get()));


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
	m_pImpl->m_pEffectHelper->SetUnorderedAccessByName(name, uav, 1);
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


