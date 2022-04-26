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

    HRESULT hr;
    hr = ParticlesNumsResize(device, particleNumss);
    
    return hr;
}

HRESULT PBFSolver::ParticlesNumsResize(ID3D11Device* device, UINT particleNums)
{
    if (!device)
    {
        return E_INVALIDARG;
    }

    m_ParticleNums = particleNums;
#pragma region Predict Position and Reorder Particle

    m_pOldPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pOldVelocityBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pNewPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pNewVelocityBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

    m_pParticleIndexBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE);
    m_pSortedOldPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pSortedNewPostionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pSortedNewVelocityBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion

#pragma region Contact and Collision
    m_pContactsBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums * 96, D3D11_BIND_UNORDERED_ACCESS);
    m_pContactCountsBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCollisionCountsBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pCollisionPlanesBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT4>>(device, m_ParticleNums * 6, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion


#pragma region PBFSolver
    m_pLambdaMultiplierBuffer = std::make_unique<StructuredBuffer<float>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pDensityBuffer = std::make_unique<StructuredBuffer<float>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pDeltaPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pUpdatedPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pUpdatedVelocityBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pCurlBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT4>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pImpulsesBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pSolverPositionBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
    m_pSolverVelocityBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, D3D11_BIND_UNORDERED_ACCESS);
#pragma endregion


    return S_OK;
}


void PBFSolver::PredParticlePosition(ID3D11DeviceContext* deviceContext, PBFSolverEffect& effect, ID3D11Buffer* oldpos, ID3D11Buffer* vec,float dt)
{
    m_pOldPositionBuffer->UpdataBufferGPU(deviceContext,oldpos);
    m_pOldVelocityBuffer->UpdataBufferGPU(deviceContext, vec);

    UINT blockNums = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    effect.SetPredictPositionState();
    effect.SetGravity(m_PBFParams.gravity);
    effect.SetDeltaTime(dt / (m_PBFParams.subStep));
    effect.SetInverseDeltaTime(1.0f / (dt / (m_PBFParams.subStep)));
    effect.SetMaxSpeed((0.5f * m_PBFParams.cellSize * m_PBFParams.subStep) / dt);
    effect.SetParticleNums(m_ParticleNums);
    effect.SetInputSRVByName("g_oldPosition", m_pOldPositionBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_oldVelocity", m_pOldVelocityBuffer->GetShaderResource());
    effect.SetOutPutUAVByName("g_PredPosition", m_pNewPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_newVelocity", m_pNewVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blockNums, 1, 1);

    effect.SetOutPutUAVByName("g_PredPosition", nullptr);
    effect.Apply(deviceContext);
}

void PBFSolver::PBFSolverIter(ID3D11DeviceContext* deviceContext, PBFSolverEffect& effect,
    ID3D11ShaderResourceView* cellStart, ID3D11ShaderResourceView* cellEnd,  ID3D11ShaderResourceView* bound)
{
    
    UINT blockNums = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    //constant buffer
    effect.SetCellSize(m_PBFParams.cellSize);
    effect.SetPloy6Coff(315.0f / (64.0f * DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 9.0f)));
    effect.SetSpikyCoff(15.0f / (DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 6.0f)));
    effect.SetSpikyGradCoff(-45.0f / (DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 6.0f)));
    effect.SetDeltaQ(m_PBFParams.sphSmoothLength * 0.3f);
    effect.SetScorrK(m_PBFParams.scorrK);
    effect.SetScorrN(m_PBFParams.scorrN);
    effect.SetLambadEps(m_PBFParams.lambdaEps);
    effect.SetVorticityConfinement(m_PBFParams.vorticityConfinement);
    effect.SetVorticityC(m_PBFParams.vorticityC);
    effect.SetInverseDensity_0(1.0f / m_PBFParams.density);
    effect.SetSphSmoothLength(m_PBFParams.sphSmoothLength);
    effect.SetMaxCollisionPlanes(m_PBFParams.maxContactPlane);
    effect.SetMaxNeighBorPerParticle(m_PBFParams.maxNeighborPerParticle);
    effect.SetCollisionDistance(m_PBFParams.collisionDistance);
    effect.SetCollisionThreshold(m_PBFParams.collisionDistance * 0.5f);
    effect.SetParticleRadiusSq(powf(m_PBFParams.particleRadius, 2.0f));
    effect.SetSOR(0.5f);
    effect.SetRestituion(0.001f);
    effect.SetMaxVelocityDelta(0.83333f);

    effect.SetPlaneNums(m_BoundaryPos.size());
    for (size_t i = 0; i < m_BoundaryPos.size(); ++i)
    {
        effect.SetPlane(i, m_BoundaryPos[i], m_BoundaryNor[i]);
    }
    effect.SetInputSRVByName("g_CellStart", cellStart);
    effect.SetInputSRVByName("g_CellEnd", cellEnd);
    effect.SetInputSRVByName("g_Bounds", bound);
    m_pContactsBuffer->ClearUAV(deviceContext);

    //contact and collision
    effect.SetCollisionParticleState();
    effect.SetOutPutUAVByName("g_Contacts", m_pContactsBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_ContactCounts", m_pContactCountsBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blockNums, 1, 1);

    effect.SetCollisionPlaneState();
    effect.SetOutPutUAVByName("g_CollisionCounts", m_pCollisionCountsBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_CollisionPlanes", m_pCollisionPlanesBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blockNums, 1, 1);

    for (int i = 0; i < m_PBFParams.maxSolverIterations; ++i)
    {
        //calc lagrange multiplier
        effect.SetCalcLagrangeMultiplierState();
        effect.SetOutPutUAVByName("g_LambdaMultiplier", m_pLambdaMultiplierBuffer->GetUnorderedAccess());
        effect.SetOutPutUAVByName("g_Density", m_pDensityBuffer->GetUnorderedAccess());
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);


        //calc Displacement
        effect.SetCalcDisplacementState();
        effect.SetOutPutUAVByName("g_DeltaPosition",m_pDeltaPositionBuffer->GetUnorderedAccess());
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);

        //add deltapos
        effect.SetADDDeltaPositionState();
        effect.SetOutPutUAVByName("g_UpdatedPosition", m_pUpdatedPositionBuffer->GetUnorderedAccess());
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);

        //solver contacts
        effect.SetSolverContactState();
        effect.Apply(deviceContext);
        deviceContext->Dispatch(blockNums, 1, 1);


        m_pSortedNewPostionBuffer->UpdataBufferGPU(deviceContext, m_pUpdatedPositionBuffer->GetBuffer(),m_ParticleNums);
    }
}

void PBFSolver::Finalize(ID3D11DeviceContext* deviceContext, PBFSolverEffect& effect)
{
    UINT blockNums = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    //update velocity
    effect.SetUpdateVelocityState();
    effect.SetOutPutUAVByName("g_sortedOldPosition", m_pSortedOldPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_UpdatedVelocity", m_pUpdatedVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blockNums, 1, 1);

    //calc vorticity
    //effect.SetCalcVorticityState();
    //effect.SetOutPutUAVByName("g_Curl", m_pCurlBuffer->GetUnorderedAccess());
    //effect.Apply(deviceContext);
    //deviceContext->Dispatch(blockNums, 1, 1);

    ////solver Velocities
    //effect.SetSolverVelocitiesState();
    //effect.SetOutPutUAVByName("g_Impulses", m_pImpulsesBuffer->GetUnorderedAccess());
    //effect.Apply(deviceContext);
    //deviceContext->Dispatch(blockNums, 1, 1);

    //PBF Finalize
    effect.SePBFFinalizeState();
    effect.SetInputSRVByName("g_Particleindex", m_pParticleIndexBuffer->GetShaderResource());
    effect.SetOutPutUAVByName("g_SolveredPosition", m_pSolverPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_SolveredVelocity", m_pSolverVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);
    deviceContext->Dispatch(blockNums, 1, 1);

    //clear some buffer
    m_pCollisionCountsBuffer->ClearUAV(deviceContext);
}

void PBFSolver::ReOrderParticle(ID3D11DeviceContext* deviceContext, PBFSolverEffect& effect, ID3D11Buffer* indexBuffer)
{
    m_pParticleIndexBuffer->UpdataBufferGPU(deviceContext, indexBuffer, m_ParticleNums);
    UINT blockNums = static_cast<UINT>(ceil((float)m_ParticleNums / 256));
    effect.SetReorderParticleState();
    effect.SetParticleNums(m_ParticleNums);
    effect.SetInputSRVByName("g_oldPosition", m_pOldPositionBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_newPostion", m_pNewPositionBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_newVelocity", m_pNewVelocityBuffer->GetShaderResource());
    effect.SetInputSRVByName("g_ParticleIndex", m_pParticleIndexBuffer->GetShaderResource());

    effect.SetOutPutUAVByName("g_sortedOldPosition", m_pSortedOldPositionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_sortedNewPosition", m_pSortedNewPostionBuffer->GetUnorderedAccess());
    effect.SetOutPutUAVByName("g_sortedVelocity", m_pSortedNewVelocityBuffer->GetUnorderedAccess());
    effect.Apply(deviceContext);

    deviceContext->Dispatch(blockNums, 1, 1);

    effect.SetOutPutUAVByName("g_sortedOldPosition", nullptr);
    effect.SetOutPutUAVByName("g_sortedVelocity", nullptr);
    effect.Apply(deviceContext);

    effect.SetInputSRVByName("g_newPostion", nullptr);
    effect.Apply(deviceContext);


}

void PBFSolver::SetDebugName(std::string name)
{

#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pOldPositionBuffer->SetDebugObjectName(name + ":OldPosition");
    m_pOldVelocityBuffer->SetDebugObjectName(name + ":OldVelocity");
    m_pNewPositionBuffer->SetDebugObjectName(name + ":NewPosition");
    m_pNewVelocityBuffer->SetDebugObjectName(name + ":NewVelocity");
    m_pParticleIndexBuffer->SetDebugObjectName(name + ":ParticleIndex");
    m_pSortedOldPositionBuffer->SetDebugObjectName(name + ":SortedOldPosition");
    m_pSortedNewPostionBuffer->SetDebugObjectName(name + ":SortedNewPostion");
    m_pSortedNewVelocityBuffer->SetDebugObjectName(name + ":SortedNewVelocity");
    m_pLambdaMultiplierBuffer->SetDebugObjectName(name + ":LambdaMultiplier");
    m_pDeltaPositionBuffer->SetDebugObjectName(name + ":DeltaPosition");
    m_pUpdatedPositionBuffer->SetDebugObjectName(name + ":UpdatedPosition");
    m_pUpdatedVelocityBuffer->SetDebugObjectName(name + ":UpdatedVelocity");
    m_pSolverPositionBuffer->SetDebugObjectName(name + ":SolverPosition");
    m_pSolverVelocityBuffer->SetDebugObjectName(name + ":SolverVelocity");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}


