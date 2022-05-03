#include "PBFSolverCommon.hlsli"


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
    
    //clac density
    float density = 0;
    //clac Lagrange multiplier
    float3 gradSum_i = float3(0.0f, 0.0f, 0.0f);
    float gradSum_j = 0;
    
    uint i = 0;
    for (i = 0; i < neightborCount; ++i)
    {
         //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        float3 neighborPartclePos = g_sortedNewPosition[neightborParticleIndex];
        //r=p_i-p_j
        float3 r = currPos - neighborPartclePos;
        density += WPoly6(r, g_sphSmoothLength);

        float3 currGrad = WSpikyGrad(r, g_sphSmoothLength);
        currGrad *= g_InverseDensity_0;
        gradSum_i += currGrad;

        if (neightborParticleIndex != DTid.x)
        {
            gradSum_j += dot(currGrad, currGrad);
        }
        
    }
    

    g_Density[DTid.x] = density;
    float gradSumTotal = gradSum_j + dot(gradSum_i, gradSum_i);
    // evaluate density constraint
    float constraint = max(density * g_InverseDensity_0 - 1.0f, 0.0f);
    float lambda = -constraint / (gradSumTotal + g_LambdaEps);
    
    g_LambdaMultiplier[DTid.x] = lambda;
}