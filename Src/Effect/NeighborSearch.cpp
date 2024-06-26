#include <Effect/NeighborSearch.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>
#include <algorithm>

HRESULT NeighborSearch::Init(ID3D11Device *device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    HRESULT hr{};
    hr = ParticlesNumsResize(device, particleNums);
    return hr;
}

HRESULT NeighborSearch::ParticlesNumsResize(ID3D11Device *device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_ParticleNums     = particleNums;
    m_HashTableSize    = 2 * m_ParticleNums; // 2097152;

    //bound group nums
    m_BoundsGroup      = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    m_BoundFinalizeNum = static_cast<UINT>(ceil((float)m_BoundsGroup / 256));

    //块的数量
    m_BlockNums        = static_cast<UINT>(ceil((float)m_ParticleNums / 1024));
    //处理总数
    m_TotalNums        = m_BlockNums * 1024;
    m_ConuterNums      = m_BlockNums * 16;

    m_pParticlePosBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pActiveIndexsBuffer
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);

#pragma region Bounds
    m_pReadBoundsLowerBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_BoundsGroup, D3D11_BIND_UNORDERED_ACCESS);
    m_pReadBoundsUpperBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_BoundsGroup, D3D11_BIND_UNORDERED_ACCESS);

    m_pWriteBoundsLowerBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device,
                                                                                      m_BoundFinalizeNum,
                                                                                      D3D11_BIND_UNORDERED_ACCESS);
    m_pWriteBoundsUpperBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device,
                                                                                      m_BoundFinalizeNum,
                                                                                      D3D11_BIND_UNORDERED_ACCESS);
    m_pBoundsBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, 2, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region calcHashBuffer

    m_pCellHashBuffer   = std::make_unique<StructuredBuffer<UINT>>(device, m_TotalNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCellIndexsBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_TotalNums, D3D11_BIND_UNORDERED_ACCESS);

#pragma endregion

#pragma region Radix Sort Buffer
    m_pPrefixSumBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_TotalNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSrcCountersBuffers
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ConuterNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pDstCountersBuffers
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ConuterNums, D3D11_BIND_UNORDERED_ACCESS);

    m_pDstKeyBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_TotalNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pDstValBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_TotalNums, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region FindCellStartAndEnd
    //size is two particleNum
    m_pCellStartBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_HashTableSize, D3D11_BIND_UNORDERED_ACCESS);
    m_pCellEndBuffer   = std::make_unique<StructuredBuffer<UINT>>(device, m_HashTableSize, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

    return S_OK;
}

void NeighborSearch::BeginNeighborSearch(ID3D11DeviceContext *deviceContext,
                                         ID3D11Buffer        *pos,
                                         ID3D11Buffer        *index,
                                         float                cellSize)
{
    m_pParticlePosBuffer->UpdataBufferGPU(deviceContext, pos, m_ParticleNums);
    m_pActiveIndexsBuffer->UpdataBufferGPU(deviceContext, index, m_ParticleNums);

    //块的数量
    UINT boundsGroup      = static_cast<int>(ceil((float)(m_ParticleNums) / 256));
    UINT boundFinalizeNum = static_cast<UINT>(ceil((float)boundsGroup / 256));

    m_Cellsize            = cellSize;
}

void NeighborSearch::CalcBounds(ID3D11DeviceContext  *deviceContext,
                                NeighborSearchEffect &effect,
                                ID3D11Buffer         *pos,
                                ID3D11Buffer         *index,
                                float                 cellSize)
{
    effect.SetCalcBoundsState();
    effect.SetParticleNums(m_ParticleNums);
    effect.SetCellSize(cellSize);
    effect.SetInputSRVByName("g_ParticlePosition", m_pParticlePosBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_acticeIndexs", m_pActiveIndexsBuffer->GetShaderResource());
    effect.SetOutPutUAVByName("g_ReadBoundsLower", m_pReadBoundsLowerBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_ReadBoundsUpper", m_pReadBoundsUpperBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BoundsGroup, 1, 1);

    effect.SetCalcBoundsGroupState();
    effect.SetBoundsGroupNum(m_BoundsGroup);
    effect.SetOutPutUAVByName("g_WriteBoundsLower", m_pWriteBoundsLowerBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_WriteBoundsUpper", m_pWriteBoundsUpperBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BoundFinalizeNum, 1, 1);

    effect.SetCalcBoundsFinalizeState();
    effect.SetBoundsFinalizeNum(m_BoundFinalizeNum);
    effect.SetOutPutUAVByName("g_Bounds", m_pBoundsBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(1, 1, 1);
}

void NeighborSearch::RadixSort(ID3D11DeviceContext *deviceContext, NeighborSearchEffect &effect)
{
    //calc hash
    effect.SetCalcHashState();
    effect.SetOutPutUAVByName("g_cellHash", m_pCellHashBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_cellIndexs", m_pCellIndexsBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    //获取cellid数据
    //得到当前哈希表大小的最高有效位1
    int bits              = high_bit(m_HashTableSize) + 1;
    int iteratorTotalNums = static_cast<int>(ceil((float)bits / 4)); //算出迭代次数
    effect.SetCounterNums(m_ConuterNums);
    effect.SetKeyNums(m_TotalNums);
    effect.SetBlocksNums(m_BlockNums);

    effect.SetOutPutUAVByName("g_SrcCounters", m_pSrcCountersBuffers->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_SrcPrefix", m_pPrefixSumBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_DstCounters", m_pDstCountersBuffers->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_DstKey", m_pDstKeyBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_DstVal", m_pDstValBuffer->GetUnorderedAccess());

    for (int i = 0; i < iteratorTotalNums; ++i)
    {
        effect.SetCurrIteration(i);
        //1-pahse
        effect.SetRadixSortCountState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        //2-phase
        effect.SetRadixSortCountPrefixState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch((UINT)ceil((float)m_ConuterNums / 1024), 1, 1);
        ////3-phase

        effect.SetRadixSortDispatchState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        //排序后交换输出输入
        m_pCellHashBuffer->UpdataBufferGPU(deviceContext, m_pDstKeyBuffer->GetBuffer());
        m_pCellIndexsBuffer->UpdataBufferGPU(deviceContext, m_pDstValBuffer->GetBuffer());
    }
}

void NeighborSearch::FindCellStartAndEnd(ID3D11DeviceContext *deviceContext, NeighborSearchEffect &effect)
{
    m_pCellStartBuffer->ClearUAV(deviceContext);
    m_pCellEndBuffer->ClearUAV(deviceContext);

    UINT blocks = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    effect.SetFindCellStartAndEndState();
    effect.SetOutPutUAVByName("g_CellStart", m_pCellStartBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_CellEnd", m_pCellEndBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blocks, 1, 1);
}

void NeighborSearch::EndNeighborSearch()
{
    //标示结束
}

void NeighborSearch::SetDebugName(std::string name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pParticlePosBuffer->SetDebugObjectName(name + ":ParticlePos");
    m_pActiveIndexsBuffer->SetDebugObjectName(name + ":ActiveIndexs");
    m_pReadBoundsLowerBuffer->SetDebugObjectName(name + ":ReadBoundsLower");
    m_pReadBoundsUpperBuffer->SetDebugObjectName(name + ":ReadBoundsUpper");
    m_pWriteBoundsLowerBuffer->SetDebugObjectName(name + ":WriteBoundsLower");
    m_pWriteBoundsUpperBuffer->SetDebugObjectName(name + ":WriteBoundsUpper");
    m_pBoundsBuffer->SetDebugObjectName(name + ":Bounds");

    m_pCellHashBuffer->SetDebugObjectName(name + ":CellHash");
    m_pCellIndexsBuffer->SetDebugObjectName(name + ":CellIndexs");
    m_pPrefixSumBuffer->SetDebugObjectName(name + ":PrefixSum");
    m_pSrcCountersBuffers->SetDebugObjectName(name + ":SrcCounters");
    m_pDstCountersBuffers->SetDebugObjectName(name + ":DstCounters");
    m_pDstKeyBuffer->SetDebugObjectName(name + ":DstKey");
    m_pDstValBuffer->SetDebugObjectName(name + ":DstVal");
    m_pCellStartBuffer->SetDebugObjectName(name + ":CellStart");
    m_pCellEndBuffer->SetDebugObjectName(name + ":CellEnd");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
