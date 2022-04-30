#include "FliudCommon.hlsli"

PointVertexOut VS(PointVertexIn vIn)
{

    PointVertexOut vOut;
    vOut.PosW = mul(float4(vIn.PosL, 1.0f), g_World).xyz;
    vOut.PosV = mul(float4(vIn.PosL, 1.0f), g_WorldView).xyz; //calculate view-space point size
    vOut.density = vIn.density;
    return vOut;
}