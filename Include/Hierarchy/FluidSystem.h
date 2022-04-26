#ifndef _FLUIDSYSTEM_H
#define _FLUIDSYSTEM_H

#include<Component/Camera.h>
#include<Component/Transform.h>
#include<Component/LightHelper.h>
#include<Effect/FluidRender.h>
#include<Effect/NeighborSearch.h>
#include<Effect/PBFSolver.h>
#include<Graphics/Vertex.h>
#include<Graphics/Texture2D.h>
#include<vector>

//****************
//流体系统类
class FluidSystem 
{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:

	FluidSystem();
	~FluidSystem();

	bool InitEffect(ID3D11Device* device);

	//窗口大小变化
	HRESULT OnResize(ID3D11Device* device, int clientWidth, int clientHeight);

	void UpdateFluid(ID3D11DeviceContext* deviceContext,float dt);
	void UpdateCamera(ID3D11DeviceContext* deviceContext,const Camera& camera);

	bool InitResource(ID3D11Device* device,ID3D11DeviceContext* deviceContext, FluidRender::ParticleParams params,
		const DirectionalLight& dirLight,int clientWidth,int clientHeigh);

	void Reset(ID3D11Device* device, ID3D11DeviceContext* deviceContext);


	void DrawParticle(ID3D11DeviceContext* deviceContext);
	void DrawFluid(ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* sceneRTV);
	
	void SetPBFBoundaryWalls(std::vector<DirectX::XMFLOAT3> pos, std::vector<DirectX::XMFLOAT3> nor);
	void FluidPBFSolver(ID3D11DeviceContext* deviceContext, float dt);


	ID3D11ShaderResourceView* GetParticleDepthSRV(ID3D11DeviceContext* deviceContext);
	ID3D11ShaderResourceView* GetParticleThicknessSRV(ID3D11DeviceContext* deviceContext);

	void SetFluidPBFParams(PBFSolver::PBFParams params) { m_pPBFSolver->SetPBFParams(params); };

	void SetDebugObjectName();
	
private:
	void CreateParticle(ID3D11Device* device, DirectX::XMFLOAT3 lower, DirectX::XMINT3 dim, float radius, float jitter);

	DirectX::XMFLOAT3 RandomUnitVector();


	void RandInit()
	{
		seed1 = 315645664;
		seed2 = seed1 ^ 0x13ab45fe;
	}

	inline uint32_t Rand()
	{
		seed1 = (seed2 ^ ((seed1 << 5) | (seed1 >> 27))) ^ (seed1 * seed2);
		seed2 = seed1 ^ ((seed2 << 12) | (seed2 >> 20));

		return seed1;
	}
	inline float Randf()
	{
		uint32_t value = Rand();
		uint32_t limit = 0xffffffff;

		return (float)value * (1.0f / (float)limit);
	}

	inline float Randf(float max)
	{
		return Randf() * max;
	}

private:
	uint32_t seed1;
	uint32_t seed2;

	UINT m_ParticleNums = 0;
	FluidRender::ParticleParams m_Params;						     //粒子参数

	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pParticlePosBuffer;
	std::unique_ptr<StructuredBuffer<DirectX::XMFLOAT3>> m_pParticleVecBuffer;
	std::unique_ptr<StructuredBuffer<UINT>> m_pParticleIndexBuffer;
	std::unique_ptr<StructuredBuffer<float>> m_pParticleDensityBuffer;



	std::unique_ptr<FluidEffect> m_pFluidEffect;			 //点精灵特效
	std::unique_ptr<FluidRender> m_pFluidRender;			 //点精灵渲染类


	
	std::unique_ptr<NeighborSearchEffect> m_pNeighborSearchEffect;			
	std::unique_ptr<NeighborSearch> m_pNeighborSearch;			         

	std::unique_ptr<PBFSolverEffect> m_pPBFSolverEffect;
	std::unique_ptr<PBFSolver> m_pPBFSolver;

};
#endif // !_FLUIDSYSTEM_H
