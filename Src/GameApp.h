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
	void UpdateFluidSystem(float dt);
	void DrawSceneWithFluid();

private:
	ObjReader m_ObjReader;
	std::vector<GameObject> m_Walls;							//墙体

	DirectionalLight m_DirLight;								//方向光源
	std::unique_ptr<BasicEffect> m_pBasicEffect;				// 基础特效
	std::unique_ptr<SkyEffect> m_pSkyEffect;					// 天空盒特效

	std::unique_ptr<SkyRender> m_pLakeCube;					    // 湖泊天空盒

	std::shared_ptr<Camera> m_pCamera;						    // 摄像机
	FirstPersonCameraController m_FPSCameraController;			// 摄像机控制器

	std::unique_ptr<FluidSystem> m_pFluidSystem;				//流体模拟系统
};


#endif