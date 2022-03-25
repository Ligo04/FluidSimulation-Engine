#include "GameApp.h"
#include <Utils/d3dUtil.h>
#include <Utils/DXTrace.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
	: D3DApp(hInstance, windowName, initWidth, initHeight),
	m_pBasicEffect(std::make_unique<BasicEffect>()),
	m_pSkyEffect(std::make_unique<SkyEffect>()),
	m_pFluidSystem(std::make_unique<FluidSystem>()),
	m_DirLight()
{
	
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_pBasicEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pSkyEffect->InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_pFluidSystem->InitEffect(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;


	return true;
}

void GameApp::OnResize()
{

	D3DApp::OnResize();


	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_pBasicEffect->SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{
	// ******************
	// 更新摄像机
	//
	m_FPSCameraController.Update(dt);

	m_pBasicEffect->SetViewMatrix(m_pCamera->GetViewXM());
	m_pBasicEffect->SetEyePos(m_pCamera->GetPosition());

	UpdateFluidSystem(dt);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	

	// ******************
	// 正常绘制场景
	//

	//绘制墙体
	m_pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	for (size_t i = 0; i < m_Walls.size(); ++i)
	{
		m_Walls[i].Draw(m_pd3dImmediateContext.Get(), m_pBasicEffect.get());
	}

	DrawSceneWithFluid();


	// 绘制天空盒
	m_pSkyEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pLakeCube->Draw(m_pd3dImmediateContext.Get(), *m_pSkyEffect, *m_pCamera);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitResource()
{

	// ******************
    // 初始化摄像机和控制器
    //

	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	// 注意：反转Z时需要将近/远平面对调
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(XMFLOAT3(0.0f, 2.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

	m_FPSCameraController.InitCamera(camera.get());
	m_FPSCameraController.SetMoveSpeed(10.0f);
	m_FPSCameraController.SetStrafeSpeed(10.0f);
	// ******************
	// 初始化特效
	//

	m_pBasicEffect->SetTextureUsed(true);
	m_pBasicEffect->SetShadowEnabled(false);
	m_pBasicEffect->SetSSAOEnabled(false);
	m_pBasicEffect->SetViewMatrix(camera->GetViewXM());
	m_pBasicEffect->SetProjMatrix(camera->GetProjXM());


	// ******************
	// 初始化对象
	//
	std::wstring modelPath = L"..\\..\\Assets\\Model\\";
	std::wstring texturePath= L"..\\..\\Assets\\Texture\\";





	// ******************
	// 初始化天空盒相关
	//
	m_pLakeCube = std::make_unique<SkyRender>();
	HR(m_pLakeCube->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{
		(texturePath + L"lake\\right.jpg").c_str(), (texturePath + L"lake\\left.jpg").c_str(),
		(texturePath + L"lake\\top.jpg").c_str(), (texturePath + L"lake\\bottom.jpg").c_str(),
		(texturePath + L"lake\\front.jpg").c_str(), (texturePath + L"lake\\back.jpg").c_str(), },
		5000.0f));

	m_pBasicEffect->SetTextureCube(m_pLakeCube->GetTextureCube());

	// ******************
	// 初始化光照
	//
	DirectionalLight dirLight{};
	dirLight.ambient = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	dirLight.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight.direction =   XMFLOAT3(5.0f, 0.0f, 0.0f);
	m_DirLight = dirLight;
	m_pBasicEffect->SetDirLight(0, m_DirLight);


	//初始化墙
	std::vector<Transform> wallsWorlds =
	{
		Transform{XMFLOAT3(1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f)},        //下面(地面)
		Transform{XMFLOAT3(1.0f,1.0f,1.0f),XMFLOAT3(-XM_PI / 2.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,5.0f)}, //前面
		Transform{XMFLOAT3(1.0f,1.0f,1.0f),XMFLOAT3(XM_PI / 2.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,-5.0f)},    //后面
		Transform{XMFLOAT3(1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,-XM_PI / 2.0f),XMFLOAT3(-5.0f,0.0f,0.0f)},  //左侧
		Transform{XMFLOAT3(1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,XM_PI / 2.0f),XMFLOAT3(5.0f,0.0f,0.0f)},      //右侧
	};

	
	m_Walls.resize(5);
	//地面
	Model model{};
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(50.0f, 50.0f),XMFLOAT2(2.5f,2.5f)));
	model.modelParts[0].material.ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 8.0f);
	model.modelParts[0].material.reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), (texturePath + L"floor.png").c_str(), nullptr,
		model.modelParts[0].texDiffuse.GetAddressOf()));
	m_Walls[0].SetModel(std::move(model));
	m_Walls[0].GetTransform().SetPosition(wallsWorlds[0].GetPosition());



	//墙体
	Model model1{};
	XMFLOAT4 color{};
	XMStoreFloat4(&color, DirectX::Colors::Blue);
	model1.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(50.0f, 50.0f), XMFLOAT2(2.0f, 2.0f), color));
	model1.modelParts[0].material.ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model1.modelParts[0].material.diffuse = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	model1.modelParts[0].material.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 8.0f);
	model1.modelParts[0].material.reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), (texturePath + L"floor.png").c_str(), nullptr,
		model1.modelParts[0].texDiffuse.GetAddressOf()));

	for (size_t i = 1; i < m_Walls.size(); ++i)
	{
		m_Walls[i].SetModel(model1);
		m_Walls[i].GetTransform().SetPosition(wallsWorlds[i].GetPosition());
		m_Walls[i].GetTransform().SetRotation(wallsWorlds[i].GetRotation());
		m_Walls[i].GetTransform().SetScale(wallsWorlds[i].GetScale());
	}



	// ******************
    // 初始化流体系统
	//

	XMMATRIX view = m_pCamera->GetViewXM();
	PointSpriteRender::ParticleParams parmas{};
	parmas.radius = 0.075f;			//世界空间的半径
	float aspect = m_pCamera->GetAspect();
	float fov = m_pCamera->GetFovy();
	//计算出世界空间的长度投影到屏幕空间的长度
	parmas.scale = float(m_ClientWidth) / aspect * (1.0f / tanf(fov * 0.5f));
	parmas.color = DirectX::XMFLOAT4(0.0f, 0.5f, 1.0f,1.0f);

	m_pFluidSystem->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(), parmas, dirLight,m_ClientWidth,m_ClientHeight);
  

	// ******************
	// 设置调试对象名
	//
	m_pLakeCube->SetDebugObjectName("LakeCube");

	return true;
}

void GameApp::UpdateFluidSystem(float dt)
{
	if (ImGui::Begin("Fliud Simulation"))
	{

	}
	ImGui::End();

	//*******************
	//流体系统更新
	m_pFluidSystem->update(m_pd3dImmediateContext.Get(), dt, *m_pCamera);
}

void GameApp::DrawSceneWithFluid()
{
	//*******************
	//最后绘制流体系统
	//
	if (ImGui::Begin("Depth Texture"))
	{
		ImVec2 winSize = ImGui::GetWindowSize();
		float smaller = (std::min)((winSize.x - 20) / AspectRatio(), winSize.y - 36);
		ImGui::Image(m_pFluidSystem->GetParticleDepthSRV(m_pd3dImmediateContext.Get()), ImVec2(smaller * AspectRatio(), smaller));
	}

	//恢复原来输出合并状态
	m_pd3dImmediateContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());



	m_pFluidSystem->DrawParticle(m_pd3dImmediateContext.Get());
	m_pFluidSystem->CalcHash(m_pd3dImmediateContext.Get());
	m_pFluidSystem->BeginRadix(m_pd3dImmediateContext.Get());

	ImGui::End();

	ImGui::Render();
}
