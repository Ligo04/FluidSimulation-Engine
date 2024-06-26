#ifndef _FLUIDSYSTEM_H
#define _FLUIDSYSTEM_H

#include <Component/Camera.h>
#include <Component/LightHelper.h>
#include <Component/Transform.h>
#include <Effect/FluidRender.h>
#include <Effect/NeighborSearch.h>
#include <Effect/PBFSolver.h>
#include <Graphics/Texture2D.h>
#include <Graphics/Vertex.h>
#include <Utils/GpuTimer.h>
#include <vector>

//****************
//流体系统类
class FluidSystem
{
    public:
        template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        FluidSystem();
        ~FluidSystem();

        bool InitEffect(ID3D11Device *device);

        //窗口大小变化
        HRESULT OnResize(ID3D11Device *device, int clientWidth, int clientHeight);

        bool    InitResource(ID3D11Device                  *device,
                             int                            clientWidth,
                             int                            clientHeight,
                             UINT                           particleNums,
                             std::vector<DirectX::XMFLOAT3> pos,
                             std::vector<DirectX::XMFLOAT3> vec,
                             std::vector<UINT>              index);

        void    TickRender(ID3D11DeviceContext        *deviceContext,
                           FluidRender::ParticleParams params,
                           const Camera               &camera,
                           ID3D11RenderTargetView     *sceneRTV,
                           ID3D11DepthStencilView     *sceneDSV,
                           bool                        isFluid);
        void    TickLogic(ID3D11DeviceContext *deviceContext, PBFSolver::PBFParams params);
        void    TickDebugTextrue(ID3D11DeviceContext        *deviceContext,
                                 FluidRender::ParticleParams params,
                                 float                       aspectRatio,
                                 bool                        isFluid);
        void    TickGpuTimes();

        void    SetBoundary(std::vector<DirectX::XMFLOAT3> pos, std::vector<DirectX::XMFLOAT3> nor);
        void    Reset(ID3D11Device                  *device,
                      UINT                           particleNums,
                      std::vector<DirectX::XMFLOAT3> pos,
                      std::vector<DirectX::XMFLOAT3> vec,
                      std::vector<UINT>              index);

        void    SetDebugObjectName(std::string name);

    private:
        GpuTimer                                             m_GpuTimer_RadixSort;
        GpuTimer                                             m_GpuTimer_NeighBorSearch;
        GpuTimer                                             m_GpuTimer_PBF;
        GpuTimer                                             m_GpuTimer_Anisotropy;
        GpuTimer                                             m_GpuTimer_FluidRender;

        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pParticlePosBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pParticleVecBuffer;
        std::unique_ptr<StructuredBuffer<UINT>>              m_pParticleIndexBuffer;
        std::unique_ptr<StructuredBuffer<float>>             m_pParticleDensityBuffer;

        std::unique_ptr<FluidEffect>                         m_pFluidEffect; //点精灵特效
        std::unique_ptr<FluidRender>                         m_pFluidRender; //点精灵渲染类

        std::unique_ptr<NeighborSearchEffect>                m_pNeighborSearchEffect;
        std::unique_ptr<NeighborSearch>                      m_pNeighborSearch;

        std::unique_ptr<PBFSolverEffect>                     m_pPBFSolverEffect;
        std::unique_ptr<PBFSolver>                           m_pPBFSolver;
};
#endif // !_FLUIDSYSTEM_H
