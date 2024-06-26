#pragma once

#include "Effects.h"
#include <Component/Camera.h>
#include <Graphics/Texture2D.h>
#include <Graphics/Vertex.h>

class FluidRender
{
    public:
        template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

        struct ParticleParams
        {
                UINT              particleNums;
                DirectX::XMFLOAT4 color;
                float             radius;
                float             RestDistance;
                float             scale;
                float             blurRadiusWorld;
                float             blurScale;
                float             blurFalloff;
                float             ior;
                DirectX::XMFLOAT4 invTexScale;
                DirectX::XMFLOAT4 clipToEye;
                DirectX::XMFLOAT4 invViewPort;
                DirectionalLight  dirLight[1];
        };

    public:
        FluidRender()                               = default;
        ~FluidRender()                              = default;
        // 不允许拷贝，允许移动
        FluidRender(const FluidRender &)            = delete;
        FluidRender &operator=(const FluidRender &) = delete;
        FluidRender(FluidRender &&)                 = default;
        FluidRender &operator=(FluidRender &&)      = default;

        HRESULT      Init(ID3D11Device *device, UINT particleNums, int clientWidth, int clientHeight);

        //窗口大小变化
        HRESULT OnResize(ID3D11Device *device, int clientWidth, int clientHeight);
        //粒子数量改变
        HRESULT ParticlesNumsResize(ID3D11Device *device, UINT particleNums);

        void    UpdateVertexInputBuffer(ID3D11DeviceContext *deviceContext,
                                        ID3D11Buffer        *updatePos,
                                        ID3D11Buffer        *updateDensity);

        //绘制流体点精灵的深度图
        void CreateParticleDepth(ID3D11DeviceContext *deviceContext,
                                 FluidEffect         &effect,
                                 ParticleParams       params,
                                 int                  offset);

        void CreateEllipsoidDepth(ID3D11DeviceContext *deviceContext,
                                  FluidEffect         &effect,
                                  ID3D11Buffer        *anisotropy,
                                  ParticleParams       params,
                                  int                  offset);

        void CreateParticleThickness(ID3D11DeviceContext *deviceContext,
                                     FluidEffect         &effect,
                                     ParticleParams       params,
                                     int                  offset);

        void DrawParticle(ID3D11DeviceContext *deviceContext, FluidEffect &effect, ParticleParams params, int offset);

        void DrawFluid(ID3D11DeviceContext    *deviceContext,
                       FluidEffect            &effect,
                       ID3D11RenderTargetView *sceneRTV,
                       ID3D11DepthStencilView *sceneDSV,
                       ID3D11Buffer           *anisotropy,
                       ParticleParams          params,
                       int                     offset);

        void BlurDepth(ID3D11DeviceContext *deviceContext, FluidEffect &effect, ParticleParams params, int offset);

        void Composite(ID3D11DeviceContext    *deviceContext,
                       FluidEffect            &effect,
                       ID3D11RenderTargetView *sceneRTV,
                       ID3D11DepthStencilView *sceneDSV,
                       ParticleParams          params,
                       int                     offset);

        ID3D11ShaderResourceView *GetDepthTexture2DSRV() { return m_pDepthTextrue->GetShaderResource(); };
        ID3D11ShaderResourceView *GetBlurDepthTexture2DSRV() { return m_pBlurDepthTexture->GetShaderResource(); };
        ID3D11ShaderResourceView *GetThicknessTexture2DSRV() { return m_pThicknessTextrue->GetShaderResource(); };
        ID3D11ShaderResourceView *GetNormalTexture2DSRV() { return m_pReCalcNormalTextrue->GetShaderResource(); };

        void                      SetDebugObjectName(const std::string &name);

    private:
        UINT                       m_ParticleNums = 0;

        int                        m_ClientWidth  = 0;
        int                        m_ClientHeigh  = 0;

        std::unique_ptr<Texture2D> m_pDepthTextrue;        //粒子深度图
        std::unique_ptr<Texture2D> m_pBlurDepthTexture;    //模糊后的深度图
        std::unique_ptr<Texture2D> m_pThicknessTextrue;    //粒子厚度图
        std::unique_ptr<Texture2D> m_pCurrSeceneTextrue;   //当前场景的场景信息
        std::unique_ptr<Texture2D> m_pReCalcNormalTextrue; //根据深度重建的法线信息

        std::unique_ptr<Depth2D>   m_pEllipsoidDepthTexture;

        ComPtr<ID3D11Buffer>       m_pParticlePostionVB;   //粒子的顶点数据
        ComPtr<ID3D11Buffer>       m_pParticleDensityVB;   //粒子的顶点数据
        ComPtr<ID3D11Buffer>       m_pParticleAnsotropyVB; //粒子的各向异性
        ComPtr<ID3D11Buffer>       m_pParticleIB;          //粒子的顶点索引数据
        ComPtr<ID3D11Buffer>       m_pQuadVB;
        ComPtr<ID3D11Buffer>       m_pQuadIB;
};
