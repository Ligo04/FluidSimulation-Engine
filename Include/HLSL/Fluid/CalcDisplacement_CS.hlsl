#include "PBFSolverCommon.hlsli"

// StructuredBuffer<float3> g_CurrPosition:register(t4);
// StructuredBuffer<float> g_CellStart:register(t5);
// StructuredBuffer<float> g_CellEnd:register(t6);
// RWStructuredBuffer<float> g_LambdaMultiplier:register(u5);
// RWStructuredBuffer<float3> g_DeltaPosition:register(u6);

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

    //curr particle pos
    float3 currPos = g_sortedNewPosition[DTid.x];
    //curr neighbor count
    uint neightborCount = g_ContactCounts[DTid.x];

    float currLambda = g_LambdaMultiplier[DTid.x];
    float poly6Q = WPoly6(g_DeltaQ, g_sphSmoothLength);

    float3 deltaPos = float3(0.0f, 0.0f, 0.0f);
    uint i = 0;
    for (i = 0; i < neightborCount; ++i)
    {
         //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        float neighborLambda = g_LambdaMultiplier[neightborParticleIndex];
       //get the cell particle pos
        float3 neighborParticlePos = g_sortedNewPosition[neightborParticleIndex];
        //r=p_i-p_j
        float3 r = currPos - neighborParticlePos;
        float poly6 = WPoly6(r, g_sphSmoothLength);
        float diffPoly = poly6 / poly6Q;
        float scorr = -g_ScorrK * pow(diffPoly, g_ScorrN);
        float coff_j = currLambda + neighborLambda + scorr;

        float3 currGrad = WSpikyGrad(r, g_sphSmoothLength);
        deltaPos += coff_j * currGrad;
    }
    
    deltaPos = deltaPos * g_InverseDensity_0;
    g_DeltaPosition[DTid.x] = deltaPos;
}