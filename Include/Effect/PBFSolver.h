#pragma once

#include "Effects.h"
#include <Graphics/Buffer.h>
#include <Graphics/Texture2D.h>
#include <Graphics/Vertex.h>

class PBFSolver
{
    public:
        template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

        struct PBFParams
        {
                int               subStep;
                int               maxSolverIterations;
                float             deltaTime;
                UINT              particleNums;
                float             particleRadius;
                float             collisionDistance;
                DirectX::XMFLOAT3 gravity;
                float             sphSmoothLength; //h
                float             density;         //p_0
                float             lambdaEps;
                float             delatQ;
                float             scorrK;
                float             scorrN;
                float             vorticityConfinement;
                float             vorticityC;
                float             cellSize;
                float             maxSpeed;
                float             maxVelocityDelta;
                int               maxNeighborPerParticle;
                int               maxContactPlane;
                int               planeNums;
                float             laplacianSmooth;
                float             anisotropyScale;
                float             anisotropyMin;
                float             anisotropyMax;

                float             staticFriction;
                float             dynamicFriction;
        };

    public:
        PBFSolver()                             = default;
        ~PBFSolver()                            = default;
        // 不允许拷贝，允许移动
        PBFSolver(const PBFSolver &)            = delete;
        PBFSolver &operator=(const PBFSolver &) = delete;
        PBFSolver(PBFSolver &&)                 = default;
        PBFSolver &operator=(PBFSolver &&)      = default;

        HRESULT    Init(ID3D11Device *device, UINT particleNums);
        //粒子数量改变
        HRESULT ParticlesNumsResize(ID3D11Device *device, UINT particleNums);

        void    SetBoundary(const std::vector<DirectX::XMFLOAT3> wallpos, const std::vector<DirectX::XMFLOAT3> wallnor);

        void    PredParticlePosition(ID3D11DeviceContext *deviceContext,
                                     PBFSolverEffect     &effect,
                                     ID3D11Buffer        *oldpos,
                                     ID3D11Buffer        *vec);

        void    BeginConstraint(ID3D11DeviceContext *deviceContext,
                                PBFSolverEffect     &effect,
                                ID3D11Buffer        *indexBuffer,
                                ID3D11Buffer        *cellStart,
                                ID3D11Buffer        *cellEnd,
                                ID3D11Buffer        *bound);

        void    SolverConstraint(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect);

        void    EndConstraint(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect);

        void    CalcAnisotropy(ID3D11DeviceContext *deviceContext, PBFSolverEffect &effect);

        ID3D11Buffer *GetPredPosition() { return m_pPredPositionBuffer->GetBuffer(); };
        ID3D11Buffer *GetSolveredPosition() { return m_pSolverPositionBuffer->GetBuffer(); };
        ID3D11Buffer *GetSolveredVelocity() { return m_pSolverVelocityBuffer->GetBuffer(); };
        ID3D11Buffer *GetParticleAnisotropy() { return m_pAnisortopyBuffer->GetBuffer(); };

        void          SetPBFParams(PBFParams params) { m_PBFParams = params; };

        PBFParams     GetPBFParmas() { return m_PBFParams; };

        void          SetDebugName(std::string name);

    private:
        UINT                           m_ParticleNums  = 0; //粒子数目
        UINT                           m_HashTableSize = 0;
        PBFParams                      m_PBFParams{};       //PBFSolver use params
        std::vector<DirectX::XMFLOAT4> m_BoundaryPlanes;

        UINT                           m_BlockNums = 0;

#pragma region
        std::unique_ptr<StructuredBuffer<UINT>>              m_pCellStartBuffer;
        std::unique_ptr<StructuredBuffer<UINT>>              m_pCellEndBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pBoundBuffer;
#pragma endregion

#pragma region Predict Position and Reorder Particle
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pOldPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pOldVelocityBuffer;

        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pPredPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pNewVelocityBuffer;
        std::unique_ptr<StructuredBuffer<UINT>>              m_pParticleIndexBuffer;

        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSortedOldPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSortedNewPostionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSortedNewVelocityBuffer;
#pragma endregion

#pragma region Contact and Collision
        std::unique_ptr<StructuredBuffer<UINT>>              m_pContactsBuffer;
        std::unique_ptr<StructuredBuffer<UINT>>              m_pContactCountsBuffer;
        std::unique_ptr<StructuredBuffer<UINT>>              m_pCollisionCountsBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT4>> m_pCollisionPlanesBuffer;
#pragma endregion

#pragma region PBFSlover
        std::unique_ptr<StructuredBuffer<float>>             m_pLambdaMultiplierBuffer;
        std::unique_ptr<StructuredBuffer<float>>             m_pDensityBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pDeltaPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pUpdatedPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pUpdatedVelocityBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT4>> m_pCurlBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pImpulsesBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSolverPositionBuffer;
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSolverVelocityBuffer;
#pragma endregion

#pragma region anisotropy
        std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pSmoothPositionBuffer;
        std::unique_ptr<StructuredBuffer<Anisotropy>>        m_pAnisortopyBuffer;
#pragma endregion
};
