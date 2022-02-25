#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <Graphics/d3dApp.h>
#include <Component/Camera.h>
#include <Hierarchy/GameObject.h>
#include <Utils/ObjReader.h>
#include <Effect/SkyRender.h>
#include <Effect/ParticleRender.h>
#include <Utils/Collision.h>

class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // 单色笔刷
	ComPtr<IDWriteFont> m_pFont;								// 字体
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// 文本格式

	ObjReader m_ObjReader;
	GameObject m_Ground;										// 地面
	std::vector<Transform> m_InstancedData;						// 树的实例数据

	std::unique_ptr<BasicEffect> m_pBasicEffect;				// 基础特效
	std::unique_ptr<ParticleEffect> m_pRainEffect;				// 雨水粒子系统
	std::unique_ptr<ParticleEffect> m_pFireEffect;				// 火焰粒子系统
	std::unique_ptr<SkyEffect> m_pSkyEffect;					// 天空盒特效

	std::unique_ptr<SkyRender> m_pGrassCube;					// 草地天空盒
	std::unique_ptr<ParticleRender> m_pRain;					// 雨水粒子系统
	std::unique_ptr<ParticleRender> m_pFire;					// 火焰粒子系统

	std::shared_ptr<Camera> m_pCamera;						    // 摄像机
};


#endif