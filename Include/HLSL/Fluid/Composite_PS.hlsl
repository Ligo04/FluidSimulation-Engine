#include "FliudCommon.hlsli"


float4 PS(PassThoughVertexOut ptIn,out float depthOut:SV_Depth) : SV_Target
{
    //flip y-coordinate
    float2 uvCoord = float2(ptIn.TexCoord.x, 1.0f - ptIn.TexCoord.y);
    float eyeZ = g_DepthTexture.Sample(g_TexSampler, uvCoord).x;
    if (eyeZ >= 0.0f)
        discard;
 
    //reconstruct eye space form depth
    float3 eyePos = viewPortToEyeSpace(uvCoord, eyeZ);
    
    //finite difference approx for normals,
    float3 zl = eyePos - viewPortToEyeSpace(uvCoord - float2(g_InvTexScale.x, 0.0), g_DepthTexture.Sample(g_TexSampler, uvCoord - float2(g_InvTexScale.x, 0.0)).x);
    float3 zr = viewPortToEyeSpace(uvCoord + float2(g_InvTexScale.x, 0.0), g_DepthTexture.Sample(g_TexSampler, uvCoord + float2(g_InvTexScale.x, 0.0)).x) - eyePos;
    float3 zt = viewPortToEyeSpace(uvCoord + float2(0.0, g_InvTexScale.y), g_DepthTexture.Sample(g_TexSampler, uvCoord + float2(0.0, g_InvTexScale.y)).x) - eyePos;
    float3 zb = eyePos - viewPortToEyeSpace(uvCoord - float2(0.0, g_InvTexScale.y), g_DepthTexture.Sample(g_TexSampler, uvCoord - float2(0.0, g_InvTexScale.y)).x);
    
    float3 dx = zl;
    float3 dy = zt;
    
    if (abs(zr.z) < abs(zl.z))
        dx = zr;
    
    if (abs(zb.z) < abs(zt.z))
        dy = zb;
 
    //float3 dx = ddx(eyePos.xyz);
    //float3 dy = -ddy(eyePos.xyz);
    
    float4 worldPos = mul(float4(eyePos, 1.0f), g_WorldViewInv);
    
    //TODO:SHADOW
    
    float3 l = mul(float4(g_DirLight[0].Direction, 0.0f),g_WorldView).xyz;
    
    float3 v = -normalize(eyePos);
    
    float3 n = -normalize(cross(dx, dy));
    float3 h = normalize(v + l);
    
    float3 skyColor = float3(0.1f, 0.2f, 0.4f) * 1.2;
    float3 groundColor = float3(0.1f, 0.1f, 0.2f);
    
    float fresnel = 0.1 + (1.0 - 0.1) * cube(1.0f - max(dot(n, v), 0.0f));
    //float lVec = g_DirLight[0].Direction;
    
    //float ln = dot(l,n)*a
    float3 rEye = reflect(-v, n).xyz;
    float3 rWorld = mul(float4(rEye, 0.0f), g_WorldViewInv).xyz;
    
    float2 texScale = float2(0.75f, 1.0f); //to account for backbuffer aspect ratio (todo: pass in)
    
    float refracScale = g_Ior * 0.025f;
    float reflectScale = g_Ior * 0.1f;
    
    refracScale *= smoothstep(0.1f, 0.4f, worldPos.y);
    
    float2 refractCoord = uvCoord + n.xy * refracScale * texScale;
    
    // read thickness from refracted coordinate otherwise we get halos around objectsw
    //float thickness = 0.8f;
    
    float thickness = max(g_ThicknessTexture.Sample(g_TexSampler, refractCoord).x, 0.8f);
    //vec3 transmission = exp(-(vec3(1.0)-color.xyz)*thickness);
    float3 transmission = (1.0f - (1.0f - g_Color.xyz) * thickness * 0.8f) * g_Color.w;
    float3 refract = g_SceneTexture.Sample(g_TexSampler, refractCoord).xyz * transmission;
    
    float2 sceneReflectCoord = uvCoord - rEye.xy * texScale * reflectScale / eyePos.z;
    float3 sceneReflect = g_SceneTexture.Sample(g_TexSampler, sceneReflectCoord).xyz;
    //vec3 planarReflect = texture2D(reflectTex, gl_TexCoord[0].xy).xyz;
    float3 planarReflect = float3(0.0f, 0.0f, 0.0f);
    float3 reflect = sceneReflect + lerp(groundColor, skyColor, smoothstep(0.15f, 0.25f, rWorld.y));
    
    //lighting
    float3 diffuse = g_Color.xyz * lerp(float3(0.29f, 0.379, 0.59), float3(1.0f, 1.0f, 1.0f), 0.5f * 0.4f * (1.0f - g_Color.w));
    float specular = 1.2 * pow(max(dot(h, n), 0.0f), 400.0f);
    
    float4 clipPos = mul(float4(0.0f, 0.0f, eyeZ, 1.0f), g_Proj);
    clipPos.z /= clipPos.w;
    depthOut = clipPos.z;
    
    return float4(diffuse + (lerp(refracScale, reflect, fresnel) + specular) * g_Color.w, 1.0f);

}