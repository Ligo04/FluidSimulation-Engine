#include "FliudCommon.hlsli"

struct PixelOut
{
    float4 result : SV_Target0;
    float4 normal : SV_Target1;
    float depthOut : SV_Depth;
};

PixelOut PS(PassThoughVertexOut ptIn)
{
    //flip y-coordinate
    float2 uvCoord = float2(ptIn.TexCoord.x, 1.0f - ptIn.TexCoord.y);
    float eyeZ = g_DepthTexture.Sample(g_TexSampler, uvCoord).x;
    if (eyeZ <= 0.0f)
        discard;
    
    PixelOut pOut;
    
    //reconstruct eye space form depth
    float3 eyePos = viewPortToEyeSpace(uvCoord, eyeZ);
    
    //finite difference approx for normals,
    float3 zl = eyePos - viewPortToEyeSpace(uvCoord - float2(g_InvTexScale.x, 0.0), g_DepthTexture.Sample(g_TexSampler, uvCoord - float2(g_InvTexScale.x, 0.0)).x);
    float3 zr = viewPortToEyeSpace(uvCoord + float2(g_InvTexScale.x, 0.0), g_DepthTexture.Sample(g_TexSampler, uvCoord + float2(g_InvTexScale.x, 0.0)).x) - eyePos;
    
    
    float3 zt = viewPortToEyeSpace(uvCoord + float2(0.0, g_InvTexScale.y), g_DepthTexture.Sample(g_TexSampler, uvCoord + float2(0.0, g_InvTexScale.y)).x) - eyePos;
    float3 zb = eyePos - viewPortToEyeSpace(uvCoord - float2(0.0, g_InvTexScale.y), g_DepthTexture.Sample(g_TexSampler, uvCoord - float2(0.0, g_InvTexScale.y)).x);
    
    float3 dx = -zl;
    float3 dy = -zt;
    
    if (abs(zr.z) < abs(zl.z))
        dx = -zr;
    
    if (abs(zb.z) < abs(zt.z))
        dy = -zb;
 
    float4 worldPos = mul(float4(eyePos, 1.0), g_WorldViewInv);

    float shadow = 0.1f;
	
    float3 l = mul(float4(g_DirLight[0].Direction,0.0f),g_WorldView).xyz;
    float3 v = -normalize(eyePos);

    //float3 normal = normalize(cross(dx, dy));
    
    float3 normal = normalize(cross(dx, dy));

    pOut.normal = float4(-normal * 0.5f + 0.5f, 1.0f); 
    
    float3 n = normal;
    float3 h = normalize(v + l);


    float3 skyColor = float3(0.1, 0.2, 0.3) * 1.2;
    float3 groundColor = float3(0.1, 0.1, 0.3);

    float fresnel = 0.1 + (1.0 - 0.1) * cube(1.0 - max(dot(n, v), 0.0));



    float3 rEye = reflect(-v, n).xyz;
    float3 rWorld = mul(float4(rEye, 0.0), g_WorldViewInv).xyz;

    float2 texScale = float2(0.75, 1.0); // to account for backbuffer aspect ratio (todo: pass in)

    float refractScale = g_Ior * 0.025;
    float reflectScale = g_Ior * 0.1;

	// attenuate refraction near ground (hack)
    refractScale *= smoothstep(0.1, 0.4, worldPos.y);

    float2 refractCoord = uvCoord + n.xy * refractScale * texScale;

	// read thickness from refracted coordinate otherwise we get halos around objectsw
    float thickness = max(g_ThicknessTexture.Sample(g_TexSampler, refractCoord).x,0.8f);

	//vec3 transmission = exp(-(vec3(1.0)-color.xyz)*thickness);
    float3 transmission = (1.0 - (1.0 - g_Color.xyz) * thickness * 0.8) * g_Color.w;
    float3 refract = g_SceneTexture.Sample(g_TexSampler, refractCoord).xyz * transmission;

    float2 sceneReflectCoord = uvCoord - rEye.xy * texScale * reflectScale / eyePos.z;
    float3 sceneReflect = g_SceneTexture.Sample(g_TexSampler, sceneReflectCoord).xyz * shadow;
	//vec3 planarReflect = texture2D(reflectTex, gl_TexCoord[0].xy).xyz;
    float3 planarReflect = float3(0.0, 0.0, 0.0);

	// fade out planar reflections above the ground
	//float3 reflect = lerp(planarReflect, sceneReflect, smoothstep(0.05, 0.3, worldPos.y)) + lerp(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y)*shadow);
    //float3 reflect = sceneReflect + lerp(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y) * shadow);
    float3 reflect = lerp(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y));
	// lighting
    //float3 diffuse = color.xyz * lerp(float3(0.29, 0.379, 0.59), float3(1.0, 1.0, 1.0), (ln * 0.5 + 0.5) * max(shadow, 0.4)) * (1.0 - color.w);
    float3 diffuse = 0.0f;
    float specular = 1.2 * pow(max(dot(h, n), 0.0), 400.0);
	
	// write valid z
    float4 clipPos = mul(float4(0.0, 0.0, eyeZ, 1.0),g_Proj);
    clipPos.z /= clipPos.w;
    pOut.depthOut = clipPos.z;

    //Color

    float4 res= float4(diffuse + (lerp(refract, reflect, fresnel) + specular) * g_Color.w, 1.0f);
    pOut.result = res;
    
    return pOut;
}