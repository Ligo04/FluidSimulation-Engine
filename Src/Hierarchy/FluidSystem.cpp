#include<Hierarchy/FluidSystem.h>

FluidSystem::FluidSystem() :
	m_pPointSpriteEffect(std::make_unique<PointSpriteEffect>()),
	m_pPointSpriteRender(std::make_unique<PointSpriteRender>()),
	m_pNeighborSearchEffect(std::make_unique<NeighborSearchEffect>()),
	m_pNeighborSearch(std::make_unique<NeighborSearch>()),
	m_ParticlePos(std::vector<DirectX::XMFLOAT3>()),
	m_ParticleIndex(std::vector<UINT>()),
	m_ParticleDensity(std::vector<float>()),
	m_Params()
{
}

FluidSystem::~FluidSystem()
{
}

bool FluidSystem::InitEffect(ID3D11Device* device)
{
	if (!m_pPointSpriteEffect->Init(device, L"..\\..\\Include\\HLSL\\Fluid\\"))
		return false;

	if (!m_pNeighborSearchEffect->Init(device, L"..\\..\\Include\\HLSL\\Fluid\\"))
		return false;

	return true;
}

void FluidSystem::update(ID3D11DeviceContext* deviceContext,float dt, const Camera& camera)
{
	//更新每帧更新的常量缓冲区
	m_pPointSpriteEffect->SetViewMatrix(camera.GetViewXM());
	m_pPointSpriteEffect->SetProjMatrix(camera.GetProjXM());
}

bool FluidSystem::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, PointSpriteRender::ParticleParams params,
	const DirectionalLight& dirLight, int clientWidth, int clientHeight)
{
	m_Params = params;
	//初始化粒子
	InitParticle(params.radius, 5.0f,DirectX::XMFLOAT3(20.0f,20.0f,20.0f));

	//粒子初始
	m_pPointSpriteRender->Init(device, (UINT)m_ParticlePos.size(), clientWidth, clientHeight);
	m_pPointSpriteEffect->SetDirLight(0, dirLight);
	
	
	m_pPointSpriteRender->UpdateVertexInputBuffer(deviceContext, m_ParticlePos, m_ParticleDensity);


	m_pNeighborSearch->Init(device, (UINT)m_ParticlePos.size(), m_Params.radius);
	return true;
}


void FluidSystem::DrawParticle(ID3D11DeviceContext* deviceContext)
{
	m_pPointSpriteRender->DrawPointSprite(deviceContext, *m_pPointSpriteEffect, m_Params, 0);
}


void FluidSystem::CalcHash(ID3D11DeviceContext* deviceContext)
{
	m_pNeighborSearch->BeginCalcHash(deviceContext, *m_pNeighborSearchEffect, m_ParticlePos.data(),m_ParticleIndex.data());
}

void FluidSystem::BeginRadix(ID3D11DeviceContext* deviceContext)
{
	m_pNeighborSearch->BeginRadixSortByCellid(deviceContext, *m_pNeighborSearchEffect);
	m_pNeighborSearch->GetCellStartAndEnd(deviceContext, *m_pNeighborSearchEffect);
}

ID3D11ShaderResourceView* FluidSystem::GetParticleDepthSRV(ID3D11DeviceContext* deviceContext)
{
	m_pPointSpriteRender->CreatePointSpriteDepth(deviceContext, *m_pPointSpriteEffect, m_Params, 0);
	return m_pPointSpriteRender->GetDepthTexture2DSRV();
}

void FluidSystem::InitParticle(float radius, float bottom, DirectX::XMFLOAT3 wallsSize)
{
	float spacing = radius * 2;
	float jitter = radius * 0.01f;
	srand(1973);

	DirectX::XMINT3 bottomDim = DirectX::XMINT3((int32_t)(bottom / spacing), (int32_t)(bottom / spacing), (int32_t)(bottom / spacing));

	UINT index = 0;
	for (int z = 0; z < bottomDim.z; ++z)
	{
		for (int y = 0; y < bottomDim.y; ++y)
		{
			for (int x = 0; x < bottomDim.x; ++x)
			{
				float posY = spacing * y + radius - 0.5f * wallsSize.y + (frand() * 2.0f - 1.0f) * jitter;
				DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(spacing * x + radius - 0.5f * wallsSize.x + (frand() * 2.0f - 1.0f) * jitter + wallsSize.x*0.25f,
					posY + wallsSize.y * 0.5f,
					spacing * z + radius - 0.5f * wallsSize.z + (frand() * 2.0f - 1.0f) * jitter + wallsSize.z * 0.5f);

				m_ParticlePos.push_back(pos);
				m_ParticleIndex.push_back(index++);
			}
		}
	}

	m_ParticleDensity = std::vector<float>(m_ParticlePos.size());
	return;
}