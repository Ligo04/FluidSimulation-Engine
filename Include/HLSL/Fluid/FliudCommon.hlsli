#include "..\LightHelper.hlsli"

cbuffer CBChangeEveryInstanceDrawing : register(b0)
{
    matrix g_World;
};

cbuffer CBChangeEveryFrame : register(b1)
{
    matrix g_View;
    matrix g_Proj;
};

cbuffer CBChangeParticleRarely : register(b2)
{
    //球体颜色
    float4 g_Color;
    //点精灵
    float g_PointRadius;
    float g_PointScale;
    float2 g_Pad0;
};

cbuffer CBChangesRarely : register(b3)
{
    //方向光
    DirectionalLight g_DirLight[1];
};


struct PointVertexIn
{
    float3 PosL : POSITION;
    float density : DENSITY;
};

struct PointVertexOut
{
    float3 PosV : POSITION;
    float density : DENSITY;
    float3 PosW : TEXCOORD0;
};

struct PointGeoOut
{
    float4 PosH : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 PosW : TEXCOORD1;
    float3 PosV : TEXCOORD2;
    float4 reflectance : TEXCOORD3;
};

float sqr(float x)
{
    return x * x;
}
