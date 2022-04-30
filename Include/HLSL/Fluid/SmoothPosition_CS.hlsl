#include "PBFSolverCommon.hlsli"

[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    float3 currPos = g_sortedNewPosition[DTid.x];
    
    uint neightborCounter = g_ContactCounts[DTid.x];
    
    float wTotal = 0.0f;
    float3 smoothNeighPos = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < neightborCounter;++i)
    {
         //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        float3 neighborPartclePos = g_sortedNewPosition[neightborParticleIndex];
        
        float3 deltaPos = currPos - neighborPartclePos;
        float W_ij = Wsmooth(deltaPos, 2 * g_sphSmoothLength);
        wTotal += W_ij;
        smoothNeighPos += W_ij * neighborPartclePos;
    }

    float3 smoothPos = currPos;
    
    
    if (neightborCounter!=0)
    {
        smoothNeighPos = smoothNeighPos / wTotal;
    
        smoothPos = (1 - g_LaplacianSmooth) * currPos + g_LaplacianSmooth * smoothNeighPos;
    }

  
    g_SmoothPosition[DTid.x] = smoothPos;
    g_SmoothPositionOmega[DTid.x] = smoothNeighPos;
}
    