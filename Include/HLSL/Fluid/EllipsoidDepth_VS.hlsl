#include "FliudCommon.hlsli"



FluidVertexOut VS(FluidVertexIn FIn)
{
    const float4 q1 = FIn.q1;
    const float4 q2 = FIn.q2;
    const float4 q3 = FIn.q3;
    float3 worldPos = FIn.PosL.xyz;
    
	// construct quadric matrix
    matrix q;
    q[0] = float4(q1.xyz * q1.w, 0.0);
    q[1] = float4(q2.xyz * q2.w, 0.0);
    q[2] = float4(q3.xyz * q3.w, 0.0);
    q[3] = float4(worldPos, 1.0);
    
    // transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
   
    matrix invclip = transpose(mul(q, g_WVP));
    
    // solve for the right hand bounds in homogenous clip space
    float a1 = DotInvW(invclip[3], invclip[3]);
    float b1 = -2.0f * DotInvW(invclip[0], invclip[3]);
    float c1 = DotInvW(invclip[0], invclip[0]);
    
    float xmin = 0.0f;
    float xmax = 0.0f;
    solveQuadraticVS(a1, b1, c1, xmin, xmax);
    
    // solve for the right hand bounds in homogenous clip space
    float a2 = DotInvW(invclip[3], invclip[3]);
    float b2 = -2.0f * DotInvW(invclip[1], invclip[3]);
    float c2 = DotInvW(invclip[1], invclip[1]);
    
    float ymin = 0.0f;
    float ymax = 0.0f;

    solveQuadraticVS(a2, b2, c2, ymin, ymax);
    
    FluidVertexOut output;
    output.PosL = float4(worldPos.xyz, 1.0f);
    output.bounds = float4(xmin, xmax, ymin, ymax);
    
    // construct inverse quadric matrix (used for ray-casting in parameter space)
    matrix invq;
    invq[0] = float4(q1.xyz / q1.w, 0.0);
    invq[1] = float4(q2.xyz / q2.w, 0.0);
    invq[2] = float4(q3.xyz / q3.w, 0.0);
    invq[3] = float4(0.0, 0.0, 0.0, 1.0);

    //invq = transpose(invq);
    invq[3] = -(mul(output.PosL, invq));

	// transform a point from view space to parameter space
    invq = mul(g_WorldViewInv, invq);

	// pass down
    output.invQ0 = invq[0];
    output.invQ1 = invq[1];
    output.invQ2 = invq[2];
    output.invQ3 = invq[3];
	
	// compute ndc pos for frustrum culling in GS
    float4 projPos = mul(float4(worldPos.xyz, 1.0), g_WVP);
    output.ndcPos = projPos / projPos.w;
    return output;

}