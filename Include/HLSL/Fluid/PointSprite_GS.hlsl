#include "FliudCommon.hlsli"


[maxvertexcount(4)]
void GS(point PointVertexOut input[1],inout TriangleStream<PointGeoOut> output)
{
    const float4 viewPosition = float4(input[0].PosV, 1.0f);            //view-sytem
    const float spriteSize = g_PointRadius * 2.0f;

    PointGeoOut gOut;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4 eyePos = viewPosition;
        eyePos.xy += spriteSize * (g_TexCoord[i] - float2(0.5f, 0.5f));

        gOut.PosH = mul(eyePos, g_Proj);
        gOut.texCoord = float2(g_TexCoord[i].x, 1.0f - g_TexCoord[i].y);
        gOut.PosV = eyePos.xyz;
        gOut.PosW = input[0].PosW;
        gOut.reflectance = float4(lerp(g_Color.xyz * 2.0, float3(1.0f, 1.0f, 1.0f), 0.1f), 0.0f);
        output.Append(gOut);
    }

}