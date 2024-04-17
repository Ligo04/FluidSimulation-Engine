#include "GameApp.h"
#include <Utils/DXTrace.h>
#include <Utils/d3dUtil.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
using namespace DirectX;

#pragma warning(disable : 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring &windowName, int initWidth, int initHeight) :
    D3DApp(hInstance, windowName, initWidth, initHeight), m_pBasicEffect(std::make_unique<BasicEffect>()),
    m_pSkyEffect(std::make_unique<SkyEffect>()), m_pFluidSystem(std::make_unique<FluidSystem>()), m_DirLight(),
    m_ParticleParmas{}, m_PBFParams{}
{
    RandInit();
}

GameApp::~GameApp() {}

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
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.01f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_pBasicEffect->SetProjMatrix(m_pCamera->GetProjXM());
    }

    m_pFluidSystem->OnResize(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
}

void GameApp::UpdateScene(float dt)
{
    // ******************
    // 更新摄像机
    //
    m_FPSCameraController.Update(dt);

    m_pBasicEffect->SetViewMatrix(m_pCamera->GetViewXM());
    m_pBasicEffect->SetEyePos(m_pCamera->GetPosition());

    //绘制完再更新
    static bool isfirst = true;
    if (isfirst)
    {
        isfirst = false;
    }
    else
    {
        if (m_PBFRun || m_Step)
        {
            //固定时间步长
            //*******************
            //流体系统更新
            m_pFluidSystem->TickLogic(m_pd3dImmediateContext.Get(), m_PBFParams);
            m_Step = false;
        }
    }
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(),
                                                  reinterpret_cast<const float *>(&Colors::Silver));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(),
                                                  D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                                  1.0f,
                                                  0);

    // ******************
    // 正常绘制场景
    //

    //绘制墙体
    m_pBasicEffect->SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
    m_Walls[0].Draw(m_pd3dImmediateContext.Get(), m_pBasicEffect.get());
    for (size_t i = 0; i < m_Walls.size(); ++i)
    {
        m_Walls[i].Draw(m_pd3dImmediateContext.Get(), m_pBasicEffect.get());
    }

    // 绘制天空盒
    m_pSkyEffect->SetRenderDefault(m_pd3dImmediateContext.Get());
    m_pLakeCube->Draw(m_pd3dImmediateContext.Get(), *m_pSkyEffect, *m_pCamera);

    //最后绘制流体
    DrawSceneWithFluid();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitResource()
{
    // ******************
    // 初始化摄像机和控制器
    //

    auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
    m_pCamera   = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    // 注意：反转Z时需要将近/远平面对调
    camera->SetFrustum(XM_PI / 3.0f, AspectRatio(), 0.01f, 1000.0f);
    camera->LookTo(XMFLOAT3(3.0f, 2.0f, -4.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

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
    std::wstring modelPath   = L"..\\Assets\\Model\\";
    std::wstring texturePath = L"..\\Assets\\Texture\\";

    // ******************
    // 初始化天空盒相关
    //
    m_pLakeCube              = std::make_unique<SkyRender>();
    HR(m_pLakeCube->InitResource(m_pd3dDevice.Get(),
                                 m_pd3dImmediateContext.Get(),
                                 std::vector<std::wstring>{
                                     (texturePath + L"lake\\right.jpg").c_str(),
                                     (texturePath + L"lake\\left.jpg").c_str(),
                                     (texturePath + L"lake\\top.jpg").c_str(),
                                     (texturePath + L"lake\\bottom.jpg").c_str(),
                                     (texturePath + L"lake\\front.jpg").c_str(),
                                     (texturePath + L"lake\\back.jpg").c_str(),
                                 },
                                 5000.0f));

    m_pBasicEffect->SetTextureCube(m_pLakeCube->GetTextureCube());

    // ******************
    // 初始化光照
    //
    DirectionalLight dirLight{};
    dirLight.ambient      = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    dirLight.diffuse      = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
    dirLight.specular     = XMFLOAT4(0.6f, 0.6f, 0.45f, 1.0f);
    DirectX::XMFLOAT3 dir = XMFLOAT3(-5.0f, -15.0f, -7.5f);
    //DirectX::XMFLOAT3 dir = XMFLOAT3(0.0f, -1.0f, 0.0f);
    XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&dir)));
    dirLight.direction = dir;
    m_DirLight         = dirLight;
    m_pBasicEffect->SetDirLight(0, m_DirLight);

    //PointLight pointLight{};
    //pointLight.position = XMFLOAT3(3.50f, 5.0f, 0.25f);
    //pointLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    //pointLight.diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    //pointLight.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    //pointLight.att = XMFLOAT3(0.005f, 0.005f, 0.005f);
    //pointLight.range = 100.0f;
    //m_pBasicEffect->SetPointLight(0, pointLight);

    //初始化墙
    std::vector<Transform> wallsWorlds = {
        Transform{XMFLOAT3(1.0f, 1.0f, 1.0f),XMFLOAT3(0.0f, 0.0f,          0.0f),XMFLOAT3(0.0f, 0.0f,  0.0f)                                                                                           }, //下面(地面)
        Transform{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(-XM_PI / 2.0f, 0.0f,          0.0f), XMFLOAT3(0.0f, 0.0f,  2.5f)}, //前面
        Transform{XMFLOAT3(1.0f, 1.0f, 1.0f),  XMFLOAT3(XM_PI / 2.0f, 0.0f,          0.0f), XMFLOAT3(0.0f, 0.0f, -0.1f)}, //后面
        Transform{XMFLOAT3(1.0f, 1.0f, 1.0f),
                  XMFLOAT3(0.0f, 0.0f, -XM_PI / 2.0f),
                  XMFLOAT3(-0.1f, 0.0f,  0.0f)                                                                         }, //左侧
        Transform{XMFLOAT3(1.0f, 1.0f, 1.0f),          XMFLOAT3(0.0f, 0.0f,  XM_PI / 2.0f), XMFLOAT3(5.5f, 0.0f,  0.0f)}, //右侧
    };

    m_Walls.resize(5);
    //地面
    Model model{};
    model.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(50.0f, 50.0f), XMFLOAT2(2.5f, 2.5f)));
    model.modelParts[0].material.ambient  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    model.modelParts[0].material.diffuse  = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    model.modelParts[0].material.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 8.0f);
    model.modelParts[0].material.reflect  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    HR(CreateWICTextureFromFile(m_pd3dDevice.Get(),
                                (texturePath + L"floor.png").c_str(),
                                nullptr,
                                model.modelParts[0].texDiffuse.GetAddressOf()));
    m_Walls[0].SetModel(std::move(model));
    m_Walls[0].GetTransform().SetPosition(wallsWorlds[0].GetPosition());

    //墙体
    Model    model1{};
    XMFLOAT4 color{};
    XMStoreFloat4(&color, DirectX::Colors::Blue);
    model1.SetMesh(m_pd3dDevice.Get(), Geometry::CreatePlane(XMFLOAT2(50.0f, 50.0f), XMFLOAT2(2.0f, 2.0f), color));
    model1.modelParts[0].material.ambient  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    model1.modelParts[0].material.diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    model1.modelParts[0].material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 8.0f);
    model1.modelParts[0].material.reflect  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    HR(CreateWICTextureFromFile(m_pd3dDevice.Get(),
                                (texturePath + L"floor.png").c_str(),
                                nullptr,
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
    std::vector<DirectX::XMFLOAT3> wallPos;
    std::vector<DirectX::XMFLOAT3> wallNor;
    for (auto &p : wallsWorlds)
    {
        wallPos.push_back(p.GetPosition());
    }
    wallNor.push_back(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));  //下面(地面)
    wallNor.push_back(DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f)); //前面
    wallNor.push_back(DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f));  //后面
    wallNor.push_back(DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f));  //左侧
    wallNor.push_back(DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f)); //右侧
    m_pFluidSystem->SetBoundary(wallPos, wallNor);

    XMMATRIX view                    = m_pCamera->GetViewXM();
    float    aspect                  = m_pCamera->GetAspect();
    float    fov                     = m_pCamera->GetFovy();

    //计算出世界空间的长度投影到屏幕空间的长度
    m_ParticleParmas.radius          = 0.10f;
    m_ParticleParmas.RestDistance    = m_ParticleParmas.radius * 0.75f;
    m_ParticleParmas.scale           = float(m_ClientWidth) / aspect * (1.0f / (tanf(fov * 0.5f)));
    m_ParticleParmas.color           = DirectX::XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);

    m_ParticleParmas.blurRadiusWorld = m_ParticleParmas.RestDistance * 0.5f * 0.5f;
    m_ParticleParmas.blurScale       = m_ParticleParmas.scale;
    m_ParticleParmas.blurFalloff     = 1.0f;
    m_ParticleParmas.ior             = 1.0f;

    m_ParticleParmas.clipToEye       = DirectX::XMFLOAT4(tanf(fov * 0.5f) * aspect,
                                                   tanf(fov * 0.5f),
                                                   tanf(fov * 0.5f) * aspect,
                                                   tanf(fov * 0.5f) * aspect);
    m_ParticleParmas.invTexScale     = DirectX::XMFLOAT4(1.0f / m_ClientWidth, aspect / m_ClientWidth, 0.0f, 0.0f);
    m_ParticleParmas.invViewPort     = DirectX::XMFLOAT4(1.0f / m_ClientWidth, aspect / m_ClientWidth, 1.0f, 0.0f);
    m_ParticleParmas.dirLight[0]     = m_DirLight;

    CreateParticle(m_pd3dDevice.Get(),
                   DirectX::XMFLOAT3(0.0f, m_ParticleParmas.RestDistance, 0.0f),
                   DirectX::XMINT3(24, 48, 24),
                   m_ParticleParmas.RestDistance,
                   m_ParticleParmas.RestDistance * 0.01f);
    m_ParticleParmas.particleNums = (UINT)m_ParticlePos.size();

    m_pFluidSystem->InitResource(m_pd3dDevice.Get(),
                                 m_ClientWidth,
                                 m_ClientHeight,
                                 m_ParticleParmas.particleNums,
                                 m_ParticlePos,
                                 m_ParticleVec,
                                 m_ParticleIndex);

    m_ParticleParmas.particleNums    = m_ParticleParmas.particleNums;
    m_PBFParams.particleRadius       = m_ParticleParmas.radius;
    m_PBFParams.collisionDistance    = m_ParticleParmas.RestDistance * 0.5f;
    m_PBFParams.cellSize             = m_ParticleParmas.radius;
    m_PBFParams.subStep              = 2;
    m_PBFParams.maxSolverIterations  = 3;
    m_PBFParams.deltaTime            = 1.0f / (m_PBFParams.subStep * 60);
    m_PBFParams.gravity              = XMFLOAT3(0.0f, -9.8f, 0.0f);
    m_PBFParams.sphSmoothLength      = m_ParticleParmas.radius;
    m_PBFParams.lambdaEps            = 1000.0f;
    m_PBFParams.vorticityConfinement = 40.0f;
    m_PBFParams.vorticityC           = 0.001f;
    m_PBFParams.delatQ               = 0.1f;
    m_PBFParams.scorrK               = 0.001f;
    m_PBFParams.scorrN               = 4;
    m_PBFParams.density
        = 315.0f / (64.0f * DirectX::XM_PI * powf(m_PBFParams.sphSmoothLength, 3.0f)) * (6643.09717f / 4774.64795f);
    m_PBFParams.maxNeighborPerParticle = 96;
    m_PBFParams.maxSpeed               = FLT_MAX; //0.5f * m_ParticleParmas.radius * m_PBFParams.subStep / (1.0f/60.0f);
    m_PBFParams.maxVelocityDelta       = 1.0f / 6.0f;
    m_PBFParams.maxContactPlane        = 6;
    m_PBFParams.planeNums              = (int)m_Walls.size();
    m_PBFParams.laplacianSmooth        = 0.4f;
    m_PBFParams.anisotropyScale        = 1.0f;
    m_PBFParams.anisotropyMin          = 0.1f * m_ParticleParmas.radius;
    m_PBFParams.anisotropyMax          = 2.0f * m_ParticleParmas.radius;
    m_PBFParams.staticFriction         = 0.0f;
    m_PBFParams.dynamicFriction        = 0.01f;
    // ******************
    // 设置调试对象名
    //
    m_pLakeCube->SetDebugObjectName("LakeCube");
    m_pFluidSystem->SetDebugObjectName("FluidSystem");
    return true;
}

void GameApp::DrawSceneWithFluid()
{
    //*******************
    //最后绘制流体系统
    //
    if (ImGui::Begin("Fliud Simulation"))
    {
        ImGui::Checkbox("Debug Information", &m_DebugDepth);
        if (ImGui::Button("Reset"))
        {
            CreateParticle(m_pd3dDevice.Get(),
                           DirectX::XMFLOAT3(0.0f, m_ParticleParmas.RestDistance, 0.0f),
                           DirectX::XMINT3(24, 48, 24),
                           m_ParticleParmas.RestDistance,
                           m_ParticleParmas.RestDistance * 0.01f);
            m_ParticleParmas.particleNums = (UINT)m_ParticlePos.size();
            m_ParticleParmas.particleNums = m_ParticleParmas.particleNums;
            ;
            m_pFluidSystem->Reset(m_pd3dDevice.Get(),
                                  m_ParticleParmas.particleNums,
                                  m_ParticlePos,
                                  m_ParticleVec,
                                  m_ParticleIndex);

            m_FirstRun = true;
            m_PBFRun   = false;
        }
        ImGui::Checkbox("Run", &m_PBFRun);
        if (ImGui::Button("Next"))
        {
            m_Step = true;
        }

        ImGui::Checkbox("DrawFluid", &m_DrawFluid);
        ImGui::Text("SubStep: %d", m_PBFParams.subStep);
        ImGui::SliderInt("##0", &m_PBFParams.subStep, 1, 10, "");
        ImGui::Text("MaxSolverIterations: %d", m_PBFParams.maxSolverIterations);
        ImGui::SliderInt("##1", &m_PBFParams.maxSolverIterations, 1, 10, "");
        ImGui::Text("Gravity");
        ImGui::SliderFloat("Gravity X", (float *)&m_PBFParams.gravity.x, -20.0f, 20.0f);
        ImGui::SliderFloat("Gravity Y", (float *)&m_PBFParams.gravity.y, -20.0f, 20.0f);
        ImGui::SliderFloat("Gravity Z", (float *)&m_PBFParams.gravity.z, -20.0f, 20.0f);
        ImGui::Text("LambdaEps: %f", m_PBFParams.lambdaEps);
        ImGui::SliderFloat("##2", &m_PBFParams.lambdaEps, 0, 2000.0f, "");
        ImGui::Text("corrK: %f", m_PBFParams.scorrK);
        ImGui::SliderFloat("##10", &m_PBFParams.scorrK, 0, 0.1f, "");
        ImGui::Text("Fixed distance: %f", m_PBFParams.delatQ);
        ImGui::SliderFloat("DeltaQ", &m_PBFParams.delatQ, 0, 0.3f, "");
        ImGui::Text("Viscosity Confinement: %f", m_PBFParams.vorticityConfinement);
        ImGui::SliderFloat("##3", &m_PBFParams.vorticityConfinement, 0, 120.0f, "");
        ImGui::Text("Viscosity: %f", m_PBFParams.vorticityC);
        ImGui::SliderFloat("##4", &m_PBFParams.vorticityC, 0, 120.0f, "");
    }
    ImGui::End();

    m_pFluidSystem->TickRender(m_pd3dImmediateContext.Get(),
                               m_ParticleParmas,
                               *m_pCamera,
                               m_pRenderTargetView.Get(),
                               m_pDepthStencilView.Get(),
                               m_DrawFluid);
    if (m_DebugDepth)
    {
        m_pFluidSystem->TickDebugTextrue(m_pd3dImmediateContext.Get(), m_ParticleParmas, AspectRatio(), m_DrawFluid);
    }
    m_pd3dImmediateContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

    if (m_PBFRun || m_Step)
    {
        m_pFluidSystem->TickGpuTimes();
    }

    ImGui::Render();
}

void GameApp::CreateParticle(ID3D11Device     *device,
                             DirectX::XMFLOAT3 lower,
                             DirectX::XMINT3   dim,
                             float             radius,
                             float             jitter)
{
    m_ParticlePos.clear();
    m_ParticleIndex.clear();
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
}

DirectX::XMFLOAT3 GameApp::RandomUnitVector()
{
    float phi      = Randf(DirectX::XM_PI * 2.0f);
    float theta    = Randf(DirectX::XM_PI * 2.0f);

    float cosTheta = cosf(theta);
    float sinTheta = sinf(theta);

    float cosPhi   = cosf(phi);
    float sinPhi   = sinf(phi);

    return DirectX::XMFLOAT3(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi);
}
