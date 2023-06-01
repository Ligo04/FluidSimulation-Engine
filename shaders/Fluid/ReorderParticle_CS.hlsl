#include "PBFSolverCommon.hlsli"

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

    int pos=g_ParticleIndex[DTid.x];
    g_sortedOldPosition[DTid.x] = g_oldPosition[pos];
    g_sortedNewPosition[DTid.x] = g_PredPosition[pos];
    g_sortedVelocity[DTid.x] = g_newVelocity[pos];
}