#include <Effect/PBFSolver.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>
#include <algorithm>

HRESULT PBFSolver::Init(ID3D11Device* device, UINT particleNumss)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_ParticleNums = particleNumss;
    
    return S_OK;
}