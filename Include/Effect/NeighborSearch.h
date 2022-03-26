#ifndef _NEIGHBORSEARCH_H
#define _NEIGHBORSEARCH_H


#include "Effects.h"
#include <Graphics/Vertex.h>
#include <Graphics/Texture2D.h>

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

	void BeginCalcHash(ID3D11DeviceContext* deviceContext,NeighborSearchEffect& effect, DirectX::XMFLOAT3* pos,UINT* index);
	void BeginRadixSortByCellid(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect);
	void GetCellStartAndEnd(ID3D11DeviceContext* deviceContext, NeighborSearchEffect& effect);

private:
	UINT m_ParticleNums = 0;											 //粒子数目
	float m_CellSize = 0;												 //网格大小
	UINT m_HashTableSize = 0;												 // hash table size
		     	     
#pragma region calchashBuffer
	ComPtr<ID3D11Buffer> m_pParticlePosBuffer;							 //顶点数据缓冲区
	ComPtr<ID3D11ShaderResourceView> m_pParticlePosSRV;
	ComPtr<ID3D11Buffer> m_pParticleIndexsBuffer;							    //粒子索引数据缓冲区
	ComPtr<ID3D11ShaderResourceView> m_pParticleIndexsSRV;

	ComPtr<ID3D11Buffer> m_pCellHashBuffer;					    //
	ComPtr<ID3D11UnorderedAccessView> m_pCellHashUAV;
	ComPtr<ID3D11Buffer> m_pCellIndexsBuffer;					    //
	ComPtr<ID3D11UnorderedAccessView> m_pCellIndexsUAV;
#pragma endregion


#pragma region RadixSortBuffer
	ComPtr<ID3D11Buffer> m_pSrcKeysBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pSrcKeysSRV;
	ComPtr<ID3D11Buffer> m_pCountersBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pCountersUAV;
	ComPtr<ID3D11Buffer> m_pPrefixBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pPrefixUAV;

	ComPtr<ID3D11Buffer> m_pSrcCountersBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pSrcCountersSRV;
	ComPtr<ID3D11Buffer> m_pDstCountersBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pDstCountersUAV;


	ComPtr<ID3D11Buffer> m_pSrcValBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pSrcValSRV;
	ComPtr<ID3D11Buffer> m_pDispatchCountersBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pDispatchCountersSRV;
	ComPtr<ID3D11Buffer> m_pSrcPrefixLocalBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pSrcPrefixLocalSRV;
	ComPtr<ID3D11Buffer> m_pDstKeyBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pDstKeyUAV;
	ComPtr<ID3D11Buffer> m_pDstValBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pDstValUAV;
#pragma endregion

#pragma region FindCellStart
	ComPtr<ID3D11Buffer> m_pCellStartBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pCellStartUAV;
	ComPtr<ID3D11Buffer> m_pCellEndBuffer;
	ComPtr<ID3D11UnorderedAccessView> m_pCellEndUAV;
#pragma endregion


};


#endif // !_NEIGHBORSEARCH_H
