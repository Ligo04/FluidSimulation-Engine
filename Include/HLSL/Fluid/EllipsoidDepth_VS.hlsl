#include "FliudCommon.hlsli"



FluidVertexOut VS(FluidVertexIn FIn)
{
    const float4 q1 = FIn.q1;
    const float4 q2 = FIn.q2;
    const float4 q3 = FIn.q3;
    float3 worldPos = FIn.position.xyz;
    
	// construct quadric matrix
    float4x4 q;
    q._m00_m10_m20_m30 = float4(q1.xyz * q1.w, 0.0);
    q._m01_m11_m21_m31 = float4(q2.xyz * q2.w, 0.0);
    q._m02_m12_m22_m32 = float4(q3.xyz * q3.w, 0.0);
    q._m03_m13_m23_m33 = float4(worldPos, 1.0);
    
    // transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
   
    float4x4 invclip = mul(g_WVP, q);
    
    // solve for the right hand bounds in homogenous clip space
    float a1 = DotInvW(invclip[3], invclip[3]);
    float b1 = -2.0f * DotInvW(invclip[0], invclip[3]);
    float c1 = DotInvW(invclip[0], invclip[0]);
    
    float xmin;
    float xmax;

    solveQuadratic(a1, b1, c1, xmin, xmax);
    
    float a2 = DotInvW(invclip[3], invclip[3]);
    float b2 = -2.0f * DotInvW(invclip[1], invclip[3]);
    float c2 = DotInvW(invclip[1], invclip[1]);
    
    float ymin;
    float ymax;

    solveQuadratic(a2, b2, c2, ymin, ymax);
    
    FluidVertexOut output;
    output.position = float4(worldPos.xyz, 1.0f);
    output.bounds = float4(xmin, xmax,ymin, ymax);
    
    // construct inverse quadric matrix (used for ray-casting in parameter space)
    float4x4 invq;
    invq._m00_m10_m20_m30 = float4(q1.xyz / q1.w, 0.0);
    invq._m01_m11_m21_m31 = float4(q2.xyz / q2.w, 0.0);
    invq._m02_m12_m22_m32 = float4(q3.xyz / q3.w, 0.0);
    invq._m03_m13_m23_m33 = float4(0.0, 0.0, 0.0, 1.0);

    invq = transpose(invq);
    invq._m03_m13_m23_m33 = -(mul(invq, output.position));

	// transform a point from view space to parameter space
    invq = mul(invq, g_WorldViewInv);

	// pass down
    output.invQ0 = invq._m00_m10_m20_m30;
    output.invQ1 = invq._m01_m11_m21_m31;
    output.invQ2 = invq._m02_m12_m22_m32;
    output.invQ3 = invq._m03_m13_m23_m33;
	
	// compute ndc pos for frustrum culling in GS
    float4 projPos = mul(g_WVP, float4(worldPos.xyz, 1.0));
    output.ndcPos = projPos / projPos.w;
    return output;

}