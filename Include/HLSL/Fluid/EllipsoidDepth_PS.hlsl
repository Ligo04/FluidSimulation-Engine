#include "FliudCommon.hlsli"

float4 PS(FluidGeoOut input, out float depthOut : SV_DEPTH) : SV_TARGET
{
    //const float3 invViewport = gParams.invViewport;
    const float4 position = input.position;

	// transform from view space to parameter space
	//column_major
    float4x4 invQuadric;
    invQuadric._m00_m10_m20_m30 = input.invQ0;
    invQuadric._m01_m11_m21_m31 = input.invQ1;
    invQuadric._m02_m12_m22_m32 = input.invQ2;
    invQuadric._m03_m13_m23_m33 = input.invQ3;

    float4 ndcPos = float4(position.x  * 2.0f - 1.0f, (1.0f) * 2.0 - 1.0, 0.0f, 1.0);
    float4 viewDir = mul(g_ProjInv, ndcPos);

	// ray to parameter space
    float4 dir = mul(invQuadric, float4(viewDir.xyz, 0.0));
    float4 origin = invQuadric._m03_m13_m23_m33;

	// set up quadratric equation
    float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);
    float b = dir.x * origin.x + dir.y * origin.y + dir.z * origin.z - dir.w * origin.w;
    float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

    float minT;
    float maxT;

    if (!solveQuadratic(a, 2.0 * b, c, minT, maxT))
    {
        discard;
    }
	{
        float3 eyePos = viewDir.xyz * minT;
        float4 ndcPos = mul(g_Proj, float4(eyePos, 1.0));
        ndcPos.z /= ndcPos.w;

        depthOut = ndcPos.z;
        return float4(eyePos.z, eyePos.z, eyePos.z,1.0f);
    }
}