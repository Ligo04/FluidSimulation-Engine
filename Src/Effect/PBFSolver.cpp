#include <Effect/PBFSolver.h>
#include <Graphics/Vertex.h>
#include <Utils/d3dUtil.h>

HRESULT PBFSolver::Init(ID3D11Device *device, UINT particleNumss)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    hr = ParticlesNumsResize(device, particleNumss);

    return hr;
}

HRESULT PBFSolver::ParticlesNumsResize(ID3D11Device *device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_ParticleNums  = particleNums;
    m_BlockNums     = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    m_HashTableSize = 2 * m_ParticleNums;
#pragma region
    m_pCellStartBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_HashTableSize, D3D11_BIND_SHADER_RESOURCE);
    m_pCellEndBuffer   = std::make_unique<StructuredBuffer<UINT>>(device, m_HashTableSize, D3D11_BIND_SHADER_RESOURCE);
    m_pBoundBuffer     = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, 2, D3D11_BIND_SHADER_RESOURCE);
#pragma endregion

#pragma region Predict Position and Reorder Particle

    m_pOldPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pOldVelocityBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pPredPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pNewVelocityBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);

    m_pParticleIndexBuffer
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pSortedOldPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSortedNewPostionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSortedNewVelocityBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region Contact and Collision
    m_pContactsBuffer
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums * 96, D3D11_BIND_UNORDERED_ACCESS);
    m_pContactCountsBuffer
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCollisionCountsBuffer
        = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCollisionPlanesBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT4>>(device,
                                                                                     m_ParticleNums * 6,
                                                                                     D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region PBFSolver
    m_pLambdaMultiplierBuffer
        = std::make_unique<StructuredBuffer<float>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pDensityBuffer = std::make_unique<StructuredBuffer<float>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pDeltaPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pUpdatedPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pUpdatedVelocityBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCurlBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT4>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pImpulsesBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSolverPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSolverVelocityBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region anisotropy
    m_pSmoothPositionBuffer
        = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pAnisortopyBuffer
        = std::make_unique<StructuredBuffer<Anisotropy>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

    return S_OK;
}

void PBFSolver::SetBoundary(const std::vector<DirectX::XMFLOAT3> wallpos, const std::vector<DirectX::XMFLOAT3> wallnor)
{
    m_BoundaryPlanes.clear();
    for (size_t i = 0; i < wallpos.size(); ++i)
    {
        float w = 0.0f;
        if (abs(wallnor[i].x) == 1.0f)
        {
            w = wallpos[i].x;
        }
        else if (abs(wallnor[i].y) == 1.0f)
        {
            w = wallpos[i].y;
        }
        else
        {
            w = wallpos[i].z;
        }

        m_BoundaryPlanes.push_back(DirectX::XMFLOAT4(wallnor[i].x, wallnor[i].y, wallnor[i].z, abs(w)));
    }
}

void PBFSolver::PredParticlePosition(ID3D11DeviceContext *deviceContext,
                                     PBFSolverEffect     &effect,
                                     ID3D11Buffer        *oldpos,
                                     ID3D11Buffer        *vec)
{
    m_pOldPositionBuffer->UpdataBufferGPU(deviceContext, oldpos);
    m_pOldVelocityBuffer->UpdataBufferGPU(deviceContext, vec);

    effect.SetPredictPositionState();
    effect.SetGravity(m_PBFParams.gravity);
    effect.SetDeltaTime(m_PBFParams.deltaTime);
    effect.SetInverseDeltaTime(1.0f / m_PBFParams.deltaTime);
    effect.SetMaxSpeed(m_PBFParams.maxSpeed);
    effect.SetParticleNums(m_ParticleNums);
    effect.SetInputSRVByName("g_oldPosition", m_pOldPositionBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_oldVelocity", m_pOldVelocityBuffer->GetShaderResource());
    effect.SetOutPutUAVByName("g_PredPosition", m_pPredPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_newVelocity", m_pNewVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);
}

void PBFSolver::BeginConstraint(ID3D11DeviceContext *deviceContext,
                                PBFSolverEffect     &effect,
                                ID3D11Buffer        *indexBuffer,
                                ID3D11Buffer        *cellStart,
                                ID3D11Buffer        *cellEnd,
                                ID3D11Buffer        *bound)
{
    //reorder particle
    m_pParticleIndexBuffer->UpdataBufferGPU(deviceContext, indexBuffer, m_ParticleNums);
    effect.SetReorderParticleState();
    effect.SetParticleNums(m_ParticleNums);
    effect.SetInputSRVByName("g_oldPosition", m_pOldPositionBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_newVelocity", m_pNewVelocityBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_ParticleIndex", m_pParticleIndexBuffer->GetShaderResource());

    effect.SetOutPutUAVByName("g_sortedOldPosition", m_pSortedOldPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_sortedNewPosition", m_pSortedNewPostionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_sortedVelocity", m_pSortedNewVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);

    deviceContext->Dispatch(m_BlockNums, 1, 1);

    m_pCellStartBuffer->UpdataBufferGPU(deviceContext, cellStart);
    m_pCellEndBuffer->UpdataBufferGPU(deviceContext, cellEnd);
    m_pBoundBuffer->UpdataBufferGPU(deviceContext, bound);

    //constant buffer
    effect.SetCellSize(m_PBFParams.cellSize);
    effect.SetPloy6Coff(315.0f / (64.0f * DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 3.0f)));
    effect.SetSpikyCoff(15.0f / (DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 3.0f)));
    effect.SetSpikyGradCoff(-45.0f / (DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 4.0f)));
    effect.SetDeltaQ(m_PBFParams.sphSmoothLength * m_PBFParams.delatQ);
    effect.SetScorrK(m_PBFParams.scorrK);
    effect.SetScorrN(m_PBFParams.scorrN);
    effect.SetLambadEps(m_PBFParams.lambdaEps);
    effect.SetVorticityConfinement(m_PBFParams.vorticityConfinement);
    effect.SetVorticityC((float)(m_PBFParams.vorticityC * 1000 * 1.50523E-07));
    effect.SetInverseDensity_0(1.0f / m_PBFParams.density);
    effect.SetSphSmoothLength(m_PBFParams.sphSmoothLength);
    effect.SetMaxCollisionPlanes(m_PBFParams.maxContactPlane);
    effect.SetMaxNeighBorPerParticle(m_PBFParams.maxNeighborPerParticle);
    effect.SetCollisionDistance(m_PBFParams.collisionDistance);
    effect.SetCollisionThreshold(m_PBFParams.collisionDistance * 0.5f);
    effect.SetParticleRadiusSq(powf(m_PBFParams.particleRadius, 2.0f));
    effect.SetSOR(0.5f);
    effect.SetRestituion(0.001f);
    effect.SetMaxVelocityDelta(m_PBFParams.maxVelocityDelta);
    effect.SetStaticFriction(m_PBFParams.staticFriction);
    effect.SetDynamicFriction(m_PBFParams.dynamicFriction);
    effect.SetPlaneNums(m_PBFParams.planeNums);
    for (size_t i = 0; i < m_PBFParams.planeNums; ++i)
    {
        effect.SetPlane(i, m_BoundaryPlanes[i]);
    }

    effect.SetInputSRVByName("g_CellStart", m_pCellStartBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_CellEnd", m_pCellEndBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_Bounds", m_pBoundBuffer->GetShaderResource());

    m_pContactsBuffer->ClearUAV(deviceContext);
    m_pCollisionCountsBuffer->ClearUAV(deviceContext);

    //contact and collision
    effect.SetCollisionParticleState();
    effect.SetOutPutUAVByName("g_Contacts", m_pContactsBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_ContactCounts", m_pContactCountsBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    effect.SetCollisionPlaneState();
    effect.SetOutPutUAVByName("g_CollisionCounts", m_pCollisionCountsBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_CollisionPlanes", m_pCollisionPlanesBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);
}

void PBFSolver::SolverConstraint(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect)
{
    effect.SetOutPutUAVByName("g_LambdaMultiplier", m_pLambdaMultiplierBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_Density", m_pDensityBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_DeltaPosition", m_pDeltaPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_UpdatedPosition", m_pUpdatedPositionBuffer->GetUnorderedAccess());
    for (int i = 0; i < m_PBFParams.maxSolverIterations; ++i)
    {
        //calc lagrange multiplier
        effect.SetCalcLagrangeMultiplierState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        //calc Displacement
        effect.SetCalcDisplacementState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        //add deltapos
        effect.SetADDDeltaPositionState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        //solver contacts
        effect.SetSolverContactState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(m_BlockNums, 1, 1);

        m_pSortedNewPostionBuffer->UpdataBufferGPU(deviceContext,
                                                   m_pUpdatedPositionBuffer->GetBuffer(),
                                                   m_ParticleNums);
    }
}

void PBFSolver::EndConstraint(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect)
{
    //update velocity
    effect.SetUpdateVelocityState();
    effect.SetOutPutUAVByName("g_sortedOldPosition", m_pSortedOldPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_UpdatedVelocity", m_pUpdatedVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    //calc vorticity
    effect.SetCalcVorticityState();
    effect.SetOutPutUAVByName("g_Curl", m_pCurlBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    //solver Velocities
    effect.SetSolverVelocitiesState();
    effect.SetOutPutUAVByName("g_Impulses", m_pImpulsesBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    //PBF Finalize
    effect.SetPBFFinalizeState();
    effect.SetInputSRVByName("g_Particleindex", m_pParticleIndexBuffer->GetShaderResource());
    effect.SetOutPutUAVByName("g_SolveredPosition", m_pSolverPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_SolveredVelocity", m_pSolverVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    //clear some buffer
    m_pCollisionCountsBuffer->ClearUAV(deviceContext);
}

void PBFSolver::CalcAnisotropy(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect)
{
    effect.SetLaplacianSmooth(m_PBFParams.laplacianSmooth);
    effect.SetAnisotropyScale(m_PBFParams.anisotropyScale);
    effect.SetAnisotropyMin(m_PBFParams.anisotropyMin);
    effect.SetAnisotropyMax(m_PBFParams.anisotropyMax);

    effect.SetSmoothPositionState();
    effect.SetOutPutUAVByName("g_SmoothPosition", m_pSmoothPositionBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);

    effect.SetCalcAnisotropyState();
    effect.SetOutPutUAVByName("g_Anisortopy", m_pAnisortopyBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(m_BlockNums, 1, 1);
}

void PBFSolver::SetDebugName(std::string name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pCellStartBuffer->SetDebugObjectName(name + ":CellStart");
    m_pCellEndBuffer->SetDebugObjectName(name + ":CellEnd");
    m_pBoundBuffer->SetDebugObjectName(name + ":pBound");

    m_pOldPositionBuffer->SetDebugObjectName(name + ":OldPosition");
    m_pOldVelocityBuffer->SetDebugObjectName(name + ":OldVelocity");
    m_pPredPositionBuffer->SetDebugObjectName(name + ":PredPosition");
    m_pNewVelocityBuffer->SetDebugObjectName(name + ":NewVelocity");
    m_pParticleIndexBuffer->SetDebugObjectName(name + ":ParticleIndex");
    m_pSortedOldPositionBuffer->SetDebugObjectName(name + ":SortedOldPosition");
    m_pSortedNewPostionBuffer->SetDebugObjectName(name + ":SortedNewPostion");
    m_pSortedNewVelocityBuffer->SetDebugObjectName(name + ":SortedNewVelocity");

    m_pContactsBuffer->SetDebugObjectName(name + ":Contacts");
    m_pContactCountsBuffer->SetDebugObjectName(name + ":ContactCounts");
    m_pCollisionCountsBuffer->SetDebugObjectName(name + ":CollisionCounts");
    m_pCollisionPlanesBuffer->SetDebugObjectName(name + ":CollisionPlanes");

    m_pLambdaMultiplierBuffer->SetDebugObjectName(name + ":LambdaMultiplier");
    m_pDensityBuffer->SetDebugObjectName(name + ":Density");
    m_pDeltaPositionBuffer->SetDebugObjectName(name + ":DeltaPosition");
    m_pUpdatedPositionBuffer->SetDebugObjectName(name + ":UpdatedPosition");
    m_pUpdatedVelocityBuffer->SetDebugObjectName(name + ":UpdatedVelocity");

    m_pCurlBuffer->SetDebugObjectName(name + ":Curl");
    m_pImpulsesBuffer->SetDebugObjectName(name + ":Impulses");

    m_pSolverPositionBuffer->SetDebugObjectName(name + ":SolverPosition");
    m_pSolverVelocityBuffer->SetDebugObjectName(name + ":SolverVelocity");

    m_pSmoothPositionBuffer->SetDebugObjectName(name + ":SmoothPosition");
    m_pAnisortopyBuffer->SetDebugObjectName(name + ":Anisortopy");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
