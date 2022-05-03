#include "FliudCommon.hlsli"

//only calculate Depth
float4 PS(PointGeoOut pIn) : SV_Target
{
    //calculate normal from texture coordinates
    float3 normal;
    normal.xy = pIn.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0f)
    {
        discard; //kill pixel outside circle
    }

    normal.z = sqrt(1.0f - mag);

    //calculate depth
    float4 pixelPos = float4(pIn.PosV + normal * g_PointRadius, 1.0f);
    float4 clipSpacePos = mul(pixelPos, g_Proj);
    float fragDepth =  clipSpacePos.z / clipSpacePos.w;

    return float4(fragDepth, fragDepth, fragDepth, 1.0f);
}