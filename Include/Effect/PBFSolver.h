#ifndef _PBFSOLVER_H
#define _PBFSOLVER_H

#include "Effects.h"
#include <Graphics/Vertex.h>
#include <Graphics/Texture2D.h>

class PBFSolver
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:

	PBFSolver() = default;
	~PBFSolver() = default;
	// 不允许拷贝，允许移动
	PBFSolver(const PBFSolver&) = delete;
	PBFSolver& operator=(const PBFSolver&) = delete;
	PBFSolver(PBFSolver&&) = default;
	PBFSolver& operator=(PBFSolver&&) = default;

	HRESULT Init(ID3D11Device* device, UINT particleNums);


private:
	UINT m_ParticleNums = 0;											 //粒子数目

};

#endif // !_PBFSOLVER_H

