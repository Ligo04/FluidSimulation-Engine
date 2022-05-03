#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <Graphics/d3dApp.h>
#include <Component/Camera.h>
#include <Hierarchy/GameObject.h>
#include <Utils/ObjReader.h>
#include <Effect/SkyRender.h>
#include <Utils/Collision.h>
#include <Hierarchy/FluidSystem.h>
#include <Hierarchy/CameraController.h>



class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
public:
	GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth = 1280, int initHeight = 720);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	void DrawSceneWithFluid();
	void CreateParticle(ID3D11Device* device, DirectX::XMFLOAT3 lower, DirectX::XMINT3 dim, float radius, float jitter);


private :

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

	ObjReader m_ObjReader;

	std::vector<GameObject> m_Walls;							//墙体

	DirectionalLight m_DirLight;								//方向光源
	std::unique_ptr<BasicEffect> m_pBasicEffect;				// 基础特效
	std::unique_ptr<SkyEffect> m_pSkyEffect;					// 天空盒特效

	std::unique_ptr<SkyRender> m_pLakeCube;					    // 湖泊天空盒

	std::shared_ptr<Camera> m_pCamera;						    // 摄像机
	FirstPersonCameraController m_FPSCameraController;			// 摄像机控制器

	std::unique_ptr<FluidSystem> m_pFluidSystem;				//流体模拟系统
	FluidRender::ParticleParams m_ParticleParmas;			   //粒子的参数
	PBFSolver::PBFParams m_PBFParams;							//


	std::vector<DirectX::XMFLOAT3> m_ParticlePos;
	std::vector<DirectX::XMFLOAT3> m_ParticleVec;
	std::vector<UINT> m_ParticleIndex;
	

	bool m_DebugDepth = false;
	bool m_PBFRun = true;
	bool m_Step = false;
	bool m_FirstRun = true;
	bool m_DrawFluid = false;

};


#endif