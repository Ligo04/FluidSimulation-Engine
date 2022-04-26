#ifndef _NEIGHBORSEARCH_H
#define _NEIGHBORSEARCH_H


#include "Effects.h"
#include <Graphics/Vertex.h>
#include <Graphics/Texture2D.h>
#include <Graphics/Buffer.h>

class NeighborSearch
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:

	NeighborSearch() = default;
	~NeighborSearch() = default;
	// 不允许拷贝，允许移动
	NeighborSearch(const NeighborSearch&) = delete;
	NeighborSearch& operator=(const NeighborSearch&) = delete;
	NeighborSearch(NeighborSearch&&) = default;
	NeighborSearch& operator=(NeighborSearch&&) = default;

	HRESULT Init(ID3D11Device* device,UINT particleNums, float radius);
	//粒子数量改变
	HRESULT ParticlesNumsResize(ID3D11Device* device, UINT particleNums);

	void CalcBounds(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect, ID3D11Buffer* pos, ID3D11Buffer* index);
	void BeginCalcHash(ID3D11DeviceContext* deviceContext,NeighborSearchEffect& effect);
	void BeginRadixSortByCellid(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect);
	void FindCellStartAndEnd(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect);

	void SetDebugName(std::string name);


	/*得到数据*/
	ID3D11Buffer* GetSortedCellid() { return m_pDstKeyBuffer->GetBuffer(); };
	ID3D11Buffer* GetSortedParticleIndex() { return m_pDstValBuffer->GetBuffer(); };
	//ID3D11Buffer* GetSortedCellStart() { return m_pCellStartBuffer->GetBuffer(); };
	//ID3D11Buffer* GetSortedCellEnd() { return m_pCellEndBuffer->GetBuffer(); };
	ID3D11ShaderResourceView* GetSortedCellStart() { return m_pCellStartBuffer->GetShaderResource(); };
	ID3D11ShaderResourceView* GetSortedCellEnd() { return m_pCellEndBuffer->GetShaderResource(); };
	ID3D11ShaderResourceView* GetBounds() { return m_pBoundsBuffer->GetShaderResource(); };
private:
	UINT m_ParticleNums = 0;											 //粒子数目
	float m_CellSize = 0;												 //网格大小
	UINT m_HashTableSize = 0;												 // hash table size


	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pParticlePosBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pActiveIndexsBuffer;
#pragma region Bounds
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pReadBoundsLowerBuffer;
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pReadBoundsUpperBuffer;
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pWriteBoundsLowerBuffer;
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pWriteBoundsUpperBuffer;
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>>  m_pBoundsBuffer;

#pragma endregion

		     	     
#pragma region calchashBuffer

	std::unique_ptr<StructuredBuffer<UINT>> m_pCellHashBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pCellIndexsBuffer;
#pragma endregion

#pragma region RadixSortBuffer
	std::unique_ptr<StructuredBuffer<UINT>> m_pPrefixSumBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pSrcCountersBuffers;
	std::unique_ptr<StructuredBuffer<UINT>> m_pDstCountersBuffers;

	std::unique_ptr<StructuredBuffer<UINT>> m_pDstKeyBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pDstValBuffer;
#pragma endregion

#pragma region FindCellStart
	std::unique_ptr<StructuredBuffer<UINT>> m_pCellStartBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pCellEndBuffer;
#pragma endregion


};


#endif // !_NEIGHBORSEARCH_H
