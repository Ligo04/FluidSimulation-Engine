#include<Hierarchy/FluidSystem.h>
#include<Effect/EffectHelper.h>

FluidSystem::FluidSystem() :
	m_pFluidEffect(std::make_unique<FluidEffect>()),
	m_pFluidRender(std::make_unique<FluidRender>()),
	m_pNeighborSearchEffect(std::make_unique<NeighborSearchEffect>()),
	m_pNeighborSearch(std::make_unique<NeighborSearch>()),
	m_pPBFSolverEffect(std::make_unique<PBFSolverEffect>()),
	m_pPBFSolver(std::make_unique<PBFSolver>()),
	m_Params()
{
	RandInit();
}

FluidSystem::~FluidSystem()
{
}

bool FluidSystem::InitEffect(ID3D11Device* device)
{
	if (!m_pFluidEffect->Init(device, L"..\\..\\Include\\HLSL\\Fluid\\"))
		return false;

	if (!m_pNeighborSearchEffect->Init(device, L"..\\..\\Include\\HLSL\\Fluid\\"))
		return false;

	if (!m_pPBFSolverEffect->Init(device, L"..\\..\\Include\\HLSL\\Fluid\\"))
		return false;

	return true;
}

HRESULT FluidSystem::OnResize(ID3D11Device* device, int clientWidth, int clientHeight)
{
	
	return m_pFluidRender->OnResize(device, clientWidth, clientHeight);
}

void FluidSystem::UpdateFluid(ID3D11DeviceContext* deviceContext, float dt)
{
	FluidPBFSolver(deviceContext, dt);
	m_pFluidRender->UpdateVertexInputBuffer(deviceContext, m_pParticlePosBuffer->GetBuffer(), m_pParticleDensityBuffer->GetBuffer());

}

void FluidSystem::UpdateCamera(ID3D11DeviceContext* deviceContext, const Camera& camera)
{
	//更新每帧更新的常量缓冲区
	m_pFluidEffect->SetViewMatrix(camera.GetViewXM());
	m_pFluidEffect->SetProjMatrix(camera.GetProjXM());
}

bool FluidSystem::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, FluidRender::ParticleParams params,
	const DirectionalLight& dirLight, int clientWidth, int clientHeight)
{

	m_Params = params;
	//初始化粒子
	//DirectX::XMINT3 dim = DirectX::XMINT3(int(ceilf(1.0f / m_Params.RestDistance)), int(ceilf(2.0f / m_Params.RestDistance)), int(ceilf(1.0f / m_Params.RestDistance)));
	CreateParticle(device, DirectX::XMFLOAT3(0.0f, m_Params.RestDistance, 0.0f),DirectX::XMINT3(24,48,24), m_Params.RestDistance, m_Params.RestDistance * 0.01f);
	//CreateParticle(device, DirectX::XMFLOAT3(0.0f, m_Params.RestDistance, 0.0f), dim, m_Params.RestDistance, m_Params.RestDistance * 0.01f);

	//粒子初始
	m_pFluidRender->Init(device, m_ParticleNums, clientWidth, clientHeight);
	m_pFluidEffect->SetDirLight(0, dirLight);
	
	//初始化
    m_pFluidRender->UpdateVertexInputBuffer(deviceContext,m_pParticlePosBuffer->GetBuffer(), m_pParticleDensityBuffer->GetBuffer());


	m_pNeighborSearch->Init(device, m_ParticleNums, m_Params.radius);
	m_pPBFSolver->Init(device, m_ParticleNums);
	return true;
}

void FluidSystem::Reset(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	//DirectX::XMINT3 dim = DirectX::XMINT3(int(ceilf(1.0f / m_Params.RestDistance)), int(ceilf(2.0f / m_Params.RestDistance)), int(ceilf(1.0f / m_Params.RestDistance)));
	CreateParticle(device, DirectX::XMFLOAT3(0.0f, m_Params.RestDistance, 0.0f),DirectX::XMINT3(24,48,24), m_Params.RestDistance, m_Params.RestDistance * 0.01f);
	//CreateParticle(device, DirectX::XMFLOAT3(0.0f, m_Params.RestDistance, 0.0f), dim, m_Params.RestDistance, m_Params.RestDistance*0.01f);
	m_pFluidRender->UpdateVertexInputBuffer(deviceContext, m_pParticlePosBuffer->GetBuffer(), m_pParticleDensityBuffer->GetBuffer());
}


void FluidSystem::DrawParticle(ID3D11DeviceContext* deviceContext)
{
	m_pFluidRender->DrawParticle(deviceContext, *m_pFluidEffect, m_Params, 0);
}

void FluidSystem::DrawFluid(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* sceneRTV)
{
	m_pFluidRender->DrawFluid(deviceContext, *m_pFluidEffect, sceneRTV, m_pPBFSolver->GetParticleAnisotropy(), m_Params, 0);
}

void FluidSystem::SetPBFBoundaryWalls(std::vector<DirectX::XMFLOAT3> pos, std::vector<DirectX::XMFLOAT3> nor)
{
	m_pPBFSolver->SetBoundary(pos, nor);
}

void FluidSystem::FluidPBFSolver(ID3D11DeviceContext* deviceContext, float dt)
{
	for (int i = 0; i < m_pPBFSolver->GetPBFParmas().subStep; ++i)
	{
		m_pPBFSolver->PredParticlePosition(deviceContext, *m_pPBFSolverEffect,
			m_pParticlePosBuffer->GetBuffer(), m_pParticleVecBuffer->GetBuffer(), dt);

		//calc hash
		m_pNeighborSearch->CalcBounds(deviceContext, *m_pNeighborSearchEffect, m_pPBFSolver->GetPredPosition(), m_pParticleIndexBuffer->GetBuffer());
		m_pNeighborSearch->BeginCalcHash(deviceContext, *m_pNeighborSearchEffect);

		//radix sort
		m_pNeighborSearch->BeginRadixSortByCellid(deviceContext, *m_pNeighborSearchEffect);
		m_pNeighborSearch->FindCellStartAndEnd(deviceContext, *m_pNeighborSearchEffect);

		m_pPBFSolver->ReOrderParticle(deviceContext, *m_pPBFSolverEffect, m_pNeighborSearch->GetSortedParticleIndex());

		//iter solver
		m_pPBFSolver->PBFSolverIter(deviceContext, *m_pPBFSolverEffect,
			m_pNeighborSearch->GetSortedCellStart(), m_pNeighborSearch->GetSortedCellEnd(),m_pNeighborSearch->GetBounds());

		m_pPBFSolver->Finalize(deviceContext, *m_pPBFSolverEffect);
		//update data
		m_pParticlePosBuffer->UpdataBufferGPU(deviceContext, m_pPBFSolver->GetSolveredPosition(), m_ParticleNums );
		m_pParticleVecBuffer->UpdataBufferGPU(deviceContext, m_pPBFSolver->GetSolveredVelocity(), m_ParticleNums );
	}

	//m_pPBFSolver->CalcAnisotropy(deviceContext, *m_pPBFSolverEffect);
}


ID3D11ShaderResourceView* FluidSystem::GetParticleDepthSRV(ID3D11DeviceContext* deviceContext)
{
	m_pFluidRender->CreateParticleDepth(deviceContext, *m_pFluidEffect, m_Params, 0);
	return m_pFluidRender->GetDepthTexture2DSRV();
}

ID3D11ShaderResourceView* FluidSystem::GetParticleThicknessSRV(ID3D11DeviceContext* deviceContext)
{
	m_pFluidRender->CreateParticleThickness(deviceContext, *m_pFluidEffect, m_Params, 0);
	return m_pFluidRender->GetThicknessTexture2DSRV();
}


void FluidSystem::SetDebugObjectName()
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	m_pParticlePosBuffer->SetDebugObjectName("ParticlePosition");
	m_pParticleVecBuffer->SetDebugObjectName("ParticleVelocity");
	m_pParticleIndexBuffer->SetDebugObjectName("ParticleIndex");
	m_pParticleDensityBuffer->SetDebugObjectName("ParticleDensity");
#endif
	m_pNeighborSearch->SetDebugName("NeighborSearch");
	m_pPBFSolver->SetDebugName("PBF");
}

void FluidSystem::CreateParticle(ID3D11Device* device, DirectX::XMFLOAT3 lower, DirectX::XMINT3 dim, float radius, float jitter)
{
	std::vector<DirectX::XMFLOAT3> m_ParticlePos;						 //初始粒子位置
	std::vector<UINT> m_ParticleIndex;						              //初始粒子索引
	std::vector<float> m_ParticleDensity;								 //
	UINT index = 0;
	for (int x = 0; x < dim.x; ++x)
	{
		for (int y = 0; y < dim.y; ++y)
		{
			for (int z = 0; z < dim.z; ++z)
			{
				DirectX::XMFLOAT3 ran = RandomUnitVector();
				DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(lower.x + float(x) * radius + ran.x * jitter,
					lower.y + float(y) * radius + ran.y * jitter,
					lower.z + float(z) * radius + ran.z * jitter);

				m_ParticlePos.push_back(pos);
				m_ParticleIndex.push_back(index++);
			}
		}
	}


	m_ParticleDensity = std::vector<float>(m_ParticlePos.size());

	m_ParticleNums = static_cast<UINT>(m_ParticlePos.size());


	//创建缓冲区数据
	m_pParticlePosBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums, m_ParticlePos.data());
	m_pParticleVecBuffer = std::make_unique<StructuredBuffer<DirectX::XMFLOAT3>>(device, m_ParticleNums);
	m_pParticleIndexBuffer = std::make_unique<StructuredBuffer<UINT>>(device, m_ParticleNums, m_ParticleIndex.data());
	m_pParticleDensityBuffer = std::make_unique<StructuredBuffer<float>>(device, m_ParticleNums, m_ParticleDensity.data());
}



inline  DirectX::XMFLOAT3 FluidSystem::RandomUnitVector()
{
	float phi = Randf(DirectX::XM_PI * 2.0f);
	float theta = Randf(DirectX::XM_PI * 2.0f);

	float cosTheta = cosf(theta);
	float sinTheta = sinf(theta);

	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	return DirectX::XMFLOAT3(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi);
}
