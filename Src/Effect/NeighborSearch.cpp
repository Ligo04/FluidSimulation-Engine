#include <Effect/NeighborSearch.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>
#include <algorithm>

HRESULT NeighborSearch::Init(ID3D11Device* device, UINT particleNums, float radius)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_CellSize = radius;
    HRESULT hr{};

    hr = ParticlesNumsResize(device, particleNums);
    return hr;
}

HRESULT NeighborSearch::ParticlesNumsResize(ID3D11Device* device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    // 防止重复初始化造成内存泄漏
    m_pParticlePosBuffer.Reset();							 
    m_pParticlePosSRV.Reset();
    m_pParticleIndexsBuffer.Reset();							  
    m_pParticleIndexsSRV.Reset();
    m_pCellHashBuffer.Reset();					    
    m_pCellHashUAV.Reset();
    m_pCellIndexsBuffer.Reset();			    
    m_pCellIndexsUAV.Reset();


    m_pSrcKeysBuffer.Reset();
    m_pSrcKeysSRV.Reset();
    m_pCountersBuffer.Reset();
    m_pCountersUAV.Reset();
    m_pPrefixBuffer.Reset();
    m_pPrefixUAV.Reset();
    m_pSrcCountersBuffer.Reset();
    m_pSrcCountersSRV.Reset();
    m_pDstCountersBuffer.Reset();
    m_pDstCountersUAV.Reset();
    m_pSrcValBuffer.Reset();
    m_pSrcValSRV.Reset();
    m_pDispatchCountersBuffer.Reset();
    m_pDispatchCountersSRV.Reset();
    m_pSrcPrefixLocalBuffer.Reset();
    m_pSrcPrefixLocalSRV.Reset();
    m_pDstKeyBuffer.Reset();
    m_pDstKeyUAV.Reset();
    m_pDstValBuffer.Reset();
    m_pDstValUAV.Reset();

    m_pCellStartBuffer.Reset();
    m_pCellStartUAV.Reset();
    m_pCellEndBuffer.Reset();
    m_pCellEndUAV.Reset();




    m_ParticleNums = particleNums;
    m_HashTableSize = 2 * m_ParticleNums;

    //块的数量
    UINT blockNums = static_cast<UINT> (ceil((float)m_ParticleNums / 1024));
    //处理总数
    UINT totalNums = blockNums * 1024;

    HRESULT hr{};
    D3D11_SHADER_RESOURCE_VIEW_DESC  srvDesc{};
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

#pragma region calcHashBuffer
    //创建结构化缓冲区(输入)
    hr = CreateStructuredBuffer(device, nullptr, sizeof(DirectX::XMFLOAT3) * totalNums, sizeof(DirectX::XMFLOAT3), m_pParticlePosBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;
    hr = CreateStructuredBuffer(device, nullptr, sizeof(UINT) * totalNums, sizeof(UINT), m_pParticleIndexsBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = totalNums;
    hr = device->CreateShaderResourceView(m_pParticlePosBuffer.Get(), &srvDesc, m_pParticlePosSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = device->CreateShaderResourceView(m_pParticleIndexsBuffer.Get(), &srvDesc, m_pParticleIndexsSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    //创建有类型缓冲区(输出)
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pCellHashBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;

    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pCellIndexsBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;

    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = totalNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pCellHashBuffer.Get(), &uavDesc, m_pCellHashUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = device->CreateUnorderedAccessView(m_pCellIndexsBuffer.Get(), &uavDesc, m_pCellIndexsUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;
#pragma endregion

#pragma region Radix Sort Buffer
    //*************************
    //1-phase
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pSrcKeysBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = totalNums;

    hr = device->CreateShaderResourceView(m_pSrcKeysBuffer.Get(), &srvDesc, m_pSrcKeysSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;
    
    UINT counterNums = (static_cast<UINT>(ceil((float)totalNums / 1024))) * 16;  
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * counterNums, m_pCountersBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = counterNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pCountersBuffer.Get(), &uavDesc, m_pCountersUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pPrefixBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = totalNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pPrefixBuffer.Get(), &uavDesc, m_pPrefixUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    //*************************
    //2-phase
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * counterNums, m_pSrcCountersBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = counterNums ;

    hr = device->CreateShaderResourceView(m_pSrcCountersBuffer.Get(), &srvDesc, m_pSrcCountersSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;


    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * counterNums, m_pDstCountersBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = counterNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pDstCountersBuffer.Get(), &uavDesc, m_pDstCountersUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    //*************************
    //3-phase
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pSrcValBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = totalNums;

    hr = device->CreateShaderResourceView(m_pSrcValBuffer.Get(), &srvDesc, m_pSrcValSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;


    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pSrcPrefixLocalBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = totalNums;

    hr = device->CreateShaderResourceView(m_pSrcPrefixLocalBuffer.Get(), &srvDesc, m_pSrcPrefixLocalSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;



    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * counterNums, m_pDispatchCountersBuffer.GetAddressOf(), true, false);
    if (FAILED(hr))
        return hr;

    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = counterNums;

    hr = device->CreateShaderResourceView(m_pDispatchCountersBuffer.Get(), &srvDesc, m_pDispatchCountersSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pDstKeyBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = totalNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pDstKeyBuffer.Get(), &uavDesc, m_pDstKeyUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;


    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * totalNums, m_pDstValBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = totalNums;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pDstValBuffer.Get(), &uavDesc, m_pDstValUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

#pragma endregion

#pragma region FindCellStartAndEnd
    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * m_HashTableSize, m_pCellStartBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;

    hr = CreateTypedBuffer(device, nullptr, sizeof(UINT) * m_HashTableSize, m_pCellEndBuffer.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;

    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;			// 起始元素的索引
    uavDesc.Buffer.Flags = 0;
    uavDesc.Buffer.NumElements = m_HashTableSize;	// 元素数目

    hr = device->CreateUnorderedAccessView(m_pCellStartBuffer.Get(), &uavDesc, m_pCellStartUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = device->CreateUnorderedAccessView(m_pCellEndBuffer.Get(), &uavDesc, m_pCellEndUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;
#pragma endregion


    return hr;
}

void NeighborSearch::BeginCalcHash(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect, 
    DirectX::XMFLOAT3* pos, UINT* index)
{

    //更新粒子的初始位置缓冲区
    D3D11_MAPPED_SUBRESOURCE mappedData{};
    deviceContext->Map(m_pParticlePosBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
    memcpy(mappedData.pData, pos, sizeof(DirectX::XMFLOAT3) * m_ParticleNums);
    deviceContext->Unmap(m_pParticlePosBuffer.Get(), 0);

    deviceContext->Map(m_pParticleIndexsBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
    memcpy(mappedData.pData, index, sizeof(UINT) * m_ParticleNums);
    deviceContext->Unmap(m_pParticleIndexsBuffer.Get(), 0);


    effect.SetCalcHashState();
    effect.SetCellFactor(1.0f / m_CellSize);
    effect.SetParticleNums(m_ParticleNums);
    effect.SetInputSRVByName("g_ParticlePosition",m_pParticlePosSRV.Get());
    effect.SetInputSRVByName("g_acticeIndexs",m_pParticleIndexsSRV.Get());
    effect.SetOutPutUAVByName("g_cellHash", m_pCellHashUAV.Get());
    effect.SetOutPutUAVByName("g_cellIndexs", m_pCellIndexsUAV.Get());

    
    effect.Apply(deviceContext);
    deviceContext->Dispatch((m_ParticleNums - 1) / 256 + 1, 1, 1);
}

void NeighborSearch::BeginRadixSortByCellid(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect)
{
    //获取cellid数据
    deviceContext->CopyResource(m_pSrcKeysBuffer.Get(), m_pCellHashBuffer.Get());
    //得到当前哈希表大小的最高有效位1
    int bits = high_bit(m_HashTableSize) + 1;
    int iteratorTotalNums = static_cast<int>(ceil((float)bits / 4));           //算出迭代次数
    //块的数量
    UINT blockNums =static_cast<int>(ceil((float)(m_ParticleNums) / 1024));
    //处理总数
    UINT totalNums = blockNums * 1024;
    UINT counterNums = blockNums * 16;

    effect.SetCounterNums(counterNums);
    effect.SetKeyNums(totalNums);
    effect.SetBlocksNums(blockNums);
    effect.SetInputSRVByName("g_SrcKey", m_pSrcKeysSRV.Get());
    effect.SetOutPutUAVByName("g_Counters", m_pCountersUAV.Get());
    effect.SetOutPutUAVByName("g_Prefix", m_pPrefixUAV.Get());

    effect.SetInputSRVByName("g_SrcCounters", m_pSrcCountersSRV.Get());
    effect.SetOutPutUAVByName("g_DstCounters", m_pDstCountersUAV.Get());

    effect.SetInputSRVByName("g_SrcKey", m_pSrcKeysSRV.Get());
    effect.SetInputSRVByName("g_SrcVal", m_pSrcValSRV.Get());
    effect.SetInputSRVByName("g_SrcPrefixLocal", m_pSrcPrefixLocalSRV.Get());
    effect.SetInputSRVByName("g_DispatchCounters", m_pDispatchCountersSRV.Get());
    effect.SetOutPutUAVByName("g_DstKey", m_pDstKeyUAV.Get());
    effect.SetOutPutUAVByName("g_DstVal", m_pDstValUAV.Get());

    for (int i = 0; i < iteratorTotalNums; ++i)
    {
        //1-pahse
        effect.SetRadixSortCountState();
        effect.SetCurrIteration(i);
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);

        //2-phase
        deviceContext->CopyResource(m_pSrcCountersBuffer.Get(), m_pCountersBuffer.Get());
        effect.SetRadixSortCountPrefixState();
        effect.Apply(deviceContext);

        deviceContext->Dispatch((UINT)ceil((float)counterNums / 1024), 1, 1);

        //3-phase
        deviceContext->CopyResource(m_pDispatchCountersBuffer.Get(), m_pDstCountersBuffer.Get());
        deviceContext->CopyResource(m_pSrcValBuffer.Get(), m_pParticleIndexsBuffer.Get());
        deviceContext->CopyResource(m_pSrcPrefixLocalBuffer.Get(), m_pPrefixBuffer.Get());
        effect.SetRadixSortDispatchState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);

        //排序后交换输出输入
        deviceContext->CopyResource(m_pSrcKeysBuffer.Get(), m_pDstKeyBuffer.Get());
    }
    deviceContext->CopyResource(m_pSrcValBuffer.Get(), m_pDstValBuffer.Get());
}

void NeighborSearch::GetCellStartAndEnd(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect)
{
    UINT blocks = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    effect.SetInputSRVByName("g_SrcKey", m_pSrcKeysSRV.Get());
    effect.SetOutPutUAVByName("g_CellStart", m_pCellStartUAV.Get());
    effect.SetOutPutUAVByName("g_CellEnd", m_pCellEndUAV.Get());
    effect.SetFindCellStartAndEndState();
    effect.Apply(deviceContext);

    deviceContext->Dispatch(blocks,1,1);

}


