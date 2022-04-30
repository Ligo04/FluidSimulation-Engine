#include "FliudCommon.hlsli"

float4 PS(FluidGeoOut input) : SV_TARGET
{
    //const float3 invViewport = gParams.invViewport;
    const float4 position = input.Position;

    matrix invQuadric;
    invQuadric._m00_m01_m02_m03 = input.invQ0;
    invQuadric._m10_m11_m12_m13 = input.invQ1;
    invQuadric._m20_m21_m22_m23 = input.invQ2;
    invQuadric._m30_m31_m32_m33 = input.invQ3;

    float4 ndcPos = float4(position.x * g_InvViewport.x * 2.0f - 1.0f, (1.0f - position.y * g_InvViewport.y) * 2.0f - 1.0f, 0.0f, 1.0);
    float4 viewDir = mul(ndcPos, g_ProjInv);

	// ray to parameter space
    float4 dir = mul(float4(viewDir.xyz, 0.0), invQuadric);
    float4 origin = invQuadric._m30_m31_m32_m33;

	// set up quadratric equation
    float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);
    float b = dir.x * origin.x + dir.y * origin.y + dir.z * origin.z - dir.w * origin.w;
    float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

    float minT = 0.0f;
    float maxT = 0.0f;

    if (!solveQuadraticPS(a, 2.0 * b, c, minT, maxT))
    {
        discard;
    }
	{
        float3 eyePos = viewDir.xyz * minT;
        float4 ndcPos = mul(float4(eyePos, 1.0),g_Proj);
        ndcPos.z /= ndcPos.w;

        return float4(-eyePos.z, eyePos.z, eyePos.z, ndcPos.z);
    }
}