#include "FliudCommon.hlsli"

//only calculate Thickness
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

    normal.z = -sqrt(1.0f - mag);

    //calc thickness
    float temp = -normal.z * 0.005f;
    
    return float4(temp, 0.0f, 0.0f, 1.0f);
}