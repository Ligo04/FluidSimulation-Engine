#include "Basic.hlsli"

// 顶点着色器
TessVertexOut VS(VertexPosNormalTangentTex vIn)
{
    TessVertexOut vOut;

    vOut.PosW = mul(float4(vIn.PosL, 1.0f), g_World).xyz;
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.TangentW = mul(vIn.TangentL, g_World);
    vOut.Tex = vIn.Tex;
    
    float d = distance(vOut.PosW, g_EyePosW);
    
    // 标准化曲面细分因子
    // TessFactor = 
    //   0, d >= g_MinTessDistance
    //   (g_MinTessDistance - d) / (g_MinTessDistance - g_MaxTessDistance), g_MinTessDistance <= d <= g_MaxTessDistance
    //   1, d <= g_MaxTessDistance
    float tess = saturate((g_MinTessDistance - d) / (g_MinTessDistance - g_MaxTessDistance));
    
    // [0, 1] --> [g_MinTessFactor, g_MaxTessFactor]
    vOut.TessFactor = g_MinTessFactor + tess * (g_MaxTessFactor - g_MinTessFactor);
    
    return vOut;
}
