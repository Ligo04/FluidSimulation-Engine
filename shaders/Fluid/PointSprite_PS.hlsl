#include "FliudCommon.hlsli"



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


    float3 diffuse = float3(0.9f, 0.9f, 0.9f);
    float3 reflectance = pIn.reflectance.xyz;

    matrix modelViewMatrix = mul(g_World, g_View);
    float3 viewLightDir = normalize(mul(modelViewMatrix, float4(g_DirLight[0].Direction, 0.0f)).xyz);
    float3 lo = diffuse * reflectance * max(0.0f, sqr(-dot(viewLightDir, normal) * 0.5 + 0.5)) * 0.2f;

    const float tmp = 1.0f / 2.2f;

    return float4(pow(abs(lo), float3(tmp, tmp, tmp)), 1.0f);

}