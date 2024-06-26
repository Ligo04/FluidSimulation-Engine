#include <Effect/EffectHelper.h>
#include <Hierarchy/FluidSystem.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

FluidSystem::FluidSystem() :
    m_pFluidEffect(std::make_unique<FluidEffect>()), m_pFluidRender(std::make_unique<FluidRender>()),
    m_pNeighborSearchEffect(std::make_unique<NeighborSearchEffect>()),
    m_pNeighborSearch(std::make_unique<NeighborSearch>()), m_pPBFSolverEffect(std::make_unique<PBFSolverEffect>()),
    m_pPBFSolver(std::make_unique<PBFSolver>())
{}

FluidSystem::~FluidSystem() {}

bool FluidSystem::InitEffect(ID3D11Device *device)
{
    if (!m_pFluidEffect->Init(device))
        return false;

    if (!m_pNeighborSearchEffect->Init(device))
        return false;

    if (!m_pPBFSolverEffect->Init(device))
        return false;

    return true;
}

HRESULT FluidSystem::OnResize(ID3D11Device *device, int clientWidth, int clientHeight)
{
    return m_pFluidRender->OnResize(device, clientWidth, clientHeight);
}

bool FluidSystem::InitResource(ID3D11Device                  *device,
                               int                            clientWidth,
                               int                            clientHeight,
                               UINT                           particleNums,
                               std::vector<DirectX::XMFLOAT3> pos,
                               std::vector<DirectX::XMFLOAT3> vec,
                               std::vector<UINT>              index)
{
    m_GpuTimer_RadixSort.Init(device);
    m_GpuTimer_NeighBorSearch.Init(device);
    m_GpuTimer_PBF.Init(device);
    m_GpuTimer_Anisotropy.Init(device);
    m_GpuTimer_FluidRender.Init(device);

    Reset(device, particleNums, pos, vec, index);

    m_pNeighborSearch->Init(device, particleNums);
    m_pPBFSolver->Init(device, particleNums);
    m_pFluidRender->Init(device, particleNums, clientWidth, clientHeight);

    return true;
}

void FluidSystem::Reset(ID3D11Device                  *device,
                        UINT                           particleNums,
                        std::vector<DirectX::XMFLOAT3> pos,
                        std::vector<DirectX::XMFLOAT3> vec,
                        std::vector<UINT>              index)
{
    m_pParticlePosBuffer     = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, particleNums, pos.data());
    m_pParticleVecBuffer     = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, particleNums);
    m_pParticleIndexBuffer   = std::make_unique<StructuredBuffer<UINT>>(device, particleNums, index.data());
    m_pParticleDensityBuffer = std::make_unique<StructuredBuffer<float>>(device, particleNums);

    m_pNeighborSearch->ParticlesNumsResize(device, particleNums);
    m_pPBFSolver->ParticlesNumsResize(device, particleNums);
}

void FluidSystem::TickRender(ID3D11DeviceContext        *deviceContext,
                             FluidRender::ParticleParams params,
                             const Camera               &camera,
                             ID3D11RenderTargetView     *sceneRTV,
                             ID3D11DepthStencilView     *sceneDSV,
                             bool                        isFluid)
{
    //更新每帧更新的渲染所需的矩阵
    m_pFluidEffect->SetViewMatrix(camera.GetViewXM());
    m_pFluidEffect->SetProjMatrix(camera.GetProjXM());

    m_pFluidRender->UpdateVertexInputBuffer(deviceContext,
                                            m_pParticlePosBuffer->GetBuffer(),
                                            m_pParticleDensityBuffer->GetBuffer());

    m_GpuTimer_FluidRender.Start();
    if (!isFluid)
    {
        m_pFluidRender->DrawParticle(deviceContext, *m_pFluidEffect, params, 0);
    }
    else
    {
        m_pFluidRender->DrawFluid(deviceContext,
                                  *m_pFluidEffect,
                                  sceneRTV,
                                  sceneDSV,
                                  m_pPBFSolver->GetParticleAnisotropy(),
                                  params,
                                  0);
    }
    m_GpuTimer_FluidRender.Stop();
}

void FluidSystem::TickLogic(ID3D11DeviceContext *deviceContext, PBFSolver::PBFParams params)
{
    m_pPBFSolver->SetPBFParams(params);

    m_GpuTimer_PBF.Start();
    for (int i = 0; i < params.subStep; ++i)
    {
        m_pPBFSolver->PredParticlePosition(deviceContext,
                                           *m_pPBFSolverEffect,
                                           m_pParticlePosBuffer->GetBuffer(),
                                           m_pParticleVecBuffer->GetBuffer());

        //NeighborSearch
        m_GpuTimer_NeighBorSearch.Start();
        m_pNeighborSearch->BeginNeighborSearch(deviceContext,
                                               m_pPBFSolver->GetPredPosition(),
                                               m_pParticleIndexBuffer->GetBuffer(),
                                               params.cellSize);
        m_pNeighborSearch->CalcBounds(deviceContext,
                                      *m_pNeighborSearchEffect,
                                      m_pPBFSolver->GetPredPosition(),
                                      m_pParticleIndexBuffer->GetBuffer(),
                                      params.cellSize);
        m_GpuTimer_RadixSort.Start();
        m_pNeighborSearch->RadixSort(deviceContext, *m_pNeighborSearchEffect);
        m_GpuTimer_RadixSort.Stop();
        m_pNeighborSearch->FindCellStartAndEnd(deviceContext, *m_pNeighborSearchEffect);
        m_pNeighborSearch->EndNeighborSearch();
        m_GpuTimer_NeighBorSearch.Stop();

        // Constraint iter solver
        m_pPBFSolver->BeginConstraint(deviceContext,
                                      *m_pPBFSolverEffect,
                                      m_pNeighborSearch->GetSortedParticleIndex(),
                                      m_pNeighborSearch->GetSortedCellStart(),
                                      m_pNeighborSearch->GetSortedCellEnd(),
                                      m_pNeighborSearch->GetBounds());
        m_pPBFSolver->SolverConstraint(deviceContext, *m_pPBFSolverEffect);
        m_pPBFSolver->EndConstraint(deviceContext, *m_pPBFSolverEffect);

        //update data
        m_pParticlePosBuffer->UpdataBufferGPU(deviceContext, m_pPBFSolver->GetSolveredPosition());
        m_pParticleVecBuffer->UpdataBufferGPU(deviceContext, m_pPBFSolver->GetSolveredVelocity());
    }
    m_GpuTimer_PBF.Stop();

    m_GpuTimer_Anisotropy.Start();
    m_pPBFSolver->CalcAnisotropy(deviceContext, *m_pPBFSolverEffect);
    m_GpuTimer_Anisotropy.Stop();
}

void FluidSystem::TickDebugTextrue(ID3D11DeviceContext        *deviceContext,
                                   FluidRender::ParticleParams params,
                                   float                       aspectRatio,
                                   bool                        isFluid)
{
    if (ImGui::Begin("Depth Texture"))
    {
        m_pFluidRender->CreateParticleDepth(deviceContext, *m_pFluidEffect, params, 0);
        ImVec2 winSize = ImGui::GetWindowSize();
        float  smaller = (std::min)((winSize.x - 20) / aspectRatio, winSize.y - 36);
        ImGui::Image(m_pFluidRender->GetDepthTexture2DSRV(), ImVec2(smaller * aspectRatio, smaller));
    };
    ImGui::End();
    if (ImGui::Begin("Thickness Texture"))
    {
        m_pFluidRender->CreateParticleThickness(deviceContext, *m_pFluidEffect, params, 0);
        ImVec2 winSize = ImGui::GetWindowSize();
        float  smaller = (std::min)((winSize.x - 20) / aspectRatio, winSize.y - 36);
        ImGui::Image(m_pFluidRender->GetThicknessTexture2DSRV(), ImVec2(smaller * aspectRatio, smaller));
    }
    ImGui::End();
    if (isFluid)
    {
        if (ImGui::Begin("Normal Texture"))
        {
            ImVec2 winSize = ImGui::GetWindowSize();
            float  smaller = (std::min)((winSize.x - 20) / aspectRatio, winSize.y - 36);
            ImGui::Image(m_pFluidRender->GetNormalTexture2DSRV(), ImVec2(smaller * aspectRatio, smaller));
        }
        ImGui::End();
    }
}

void FluidSystem::TickGpuTimes()
{
    if (ImGui::Begin("GPU Profile"))
    {
        //ImGui::Text("Particle Numbers: %.3f ms", );

        m_GpuTimer_RadixSort.TryGetTime(nullptr);
        ImGui::Text("Radix Sort: %.3f ms", m_GpuTimer_RadixSort.AverageTime() * 1000);

        m_GpuTimer_NeighBorSearch.TryGetTime(nullptr);
        ImGui::Text("NeighBorSearch: %.3f ms", m_GpuTimer_NeighBorSearch.AverageTime() * 1000);

        m_GpuTimer_PBF.TryGetTime(nullptr);
        ImGui::Text("PBF Total: %.3f ms", m_GpuTimer_PBF.AverageTime() * 1000);

        m_GpuTimer_Anisotropy.TryGetTime(nullptr);
        ImGui::Text("Calc Anisotropy: %.3f ms", m_GpuTimer_Anisotropy.AverageTime() * 1000);

        m_GpuTimer_FluidRender.TryGetTime(nullptr);
        ImGui::Text("Render: %.3f ms", m_GpuTimer_FluidRender.AverageTime() * 1000);
    }
    ImGui::End();
}

void FluidSystem::SetBoundary(std::vector<DirectX::XMFLOAT3> pos, std::vector<DirectX::XMFLOAT3> nor)
{
    m_pPBFSolver->SetBoundary(pos, nor);
}

void FluidSystem::SetDebugObjectName(std::string name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pParticlePosBuffer->SetDebugObjectName(name + ":ParticlePosition");
    m_pParticleVecBuffer->SetDebugObjectName(name + ":ParticleVelocity");
    m_pParticleIndexBuffer->SetDebugObjectName(name + ":ParticleIndex");
    m_pParticleDensityBuffer->SetDebugObjectName(name + ":ParticleDensity");
#else
    UNREFERENCED_PARAMETER(name);
#endif
    m_pFluidRender->SetDebugObjectName("FluidRender");
    m_pNeighborSearch->SetDebugName("NeighborSearch");
    m_pPBFSolver->SetDebugName("PBF");
}
