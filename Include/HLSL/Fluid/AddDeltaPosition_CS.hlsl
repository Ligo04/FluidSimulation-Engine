#include "PBFSolverCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    uint totalNeighborNum = g_ContactCounts[DTid.x];
    float factor = max(g_SOR * totalNeighborNum, 1.0f);
    float3 resPos = g_sortedNewPosition[DTid.x] + g_DeltaPosition[DTid.x] * (1 / factor);
    
    g_sortedNewPosition[DTid.x] = resPos;
}