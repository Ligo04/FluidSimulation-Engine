#include "..\LightHelper.hlsli"

Texture2D<float4> g_DepthTexture : register(t0);
Texture2D<float4> g_ThicknessTexture : register(t1);
Texture2D<float4> g_SceneTexture : register(t2);

SamplerState g_TexSampler : register(s0);

static const float2 g_TexCoord[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

cbuffer CBChangeEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    
};

cbuffer CBChangeEveryFrame : register(b1)
{
    matrix g_View;
    matrix g_Proj;
    matrix g_ProjInv;
    matrix g_WorldView;
    matrix g_WorldViewInv;
    matrix g_WVP;
    float4 g_clipPosToEye;
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

cbuffer CBChangeFluid : register(b3)
{
    float g_BlurRadiusWorld;
    float g_BlurScale;
    float g_BlurFalloff;
    float g_Ior;
    
    
    float4 g_InvTexScale;
    float4 g_InvViewport;
}

cbuffer CBChangesRarely : register(b4)
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

struct MeshVertexIn
{
    float3 PosW : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

struct PassThoughVertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};


struct FluidVertexIn
{
    float3 PosL : POSITION;
    float4 q1 : U;
    float4 q2 : V;
    float4 q3 : W;
};

struct FluidVertexOut
{
    float4 PosL : POSITION;
    float4 bounds : TEXCOORD0; // xmin, xmax, ymin, ymax
    float4 invQ0 : TEXCOORD1;
    float4 invQ1 : TEXCOORD2;
    float4 invQ2 : TEXCOORD3;
    float4 invQ3 : TEXCOORD4;
    float4 ndcPos : TEXCOORD5; /// Position in normalized device coordinates (ie /w)
};

struct FluidGeoOut
{
    float4 Position : SV_POSITION;
    float4 invQ0 : TEXCOORD0;
    float4 invQ1 : TEXCOORD1;
    float4 invQ2 : TEXCOORD2;
    float4 invQ3 : TEXCOORD3;
};

float sqr(float x)
{
    return x * x;
}

float cube(float x)
{
    return x * x * x;
}

float DotInvW(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z - a.w * b.w;
}

float Sign(float x)
{
    return x < 0.0 ? -1.0 : 1.0;
}


float3 viewPortToEyeSpace(float2 coord,float eyeZ)
{
    float2 clipPosToeye = g_clipPosToEye.xy;
    
    float2 uv = float2(coord.x * 2.0f - 1.0f, (1.0f - coord.y) * 2.0f - 1.0f) * clipPosToeye;
    
    return float3(-uv * eyeZ, eyeZ);
}

bool solveQuadraticVS(float a, float b, float c, out float minT, out float maxT)
{
    if (a == 0.0 && b == 0.0)
    {
        minT = maxT = 0.0;
        return false;
    }

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
    {
        return false;
    }

    float t = -0.5 * (b + Sign(b) * sqrt(discriminant));
    minT = t / a;
    maxT = c / t;

    if (minT > maxT)
    {
        float tmp = minT;
        minT = maxT;
        maxT = tmp;
    }

    return true;
}

bool solveQuadraticPS(float a, float b, float c, out float minT, out float maxT)
{
    if (a == 0.0 && b == 0.0)
    {
        minT = maxT = 0.0;
        return true;
    }

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
    {
        return false;
    }

    float t = -0.5 * (b + Sign(b) * sqrt(discriminant));
    minT = t / a;
    maxT = c / t;

    if (minT > maxT)
    {
        float tmp = minT;
        minT = maxT;
        maxT = tmp;
    }

    return true;
}