#include "NerghborSearchCommon.hlsli"

[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x < g_ParticleNums)
    {
        float3 currPos = g_ParticlePosition[DTid.x];
        int3 cellPos = floor((currPos - g_Bounds[0]) / g_CellSize);
        g_cellHash[DTid.x] = GetCellHash(cellPos);
        g_cellIndexs[DTid.x] = g_acticeIndexs[DTid.x];
    }
    else
    {
        g_cellHash[DTid.x] = -1;
        g_cellIndexs[DTid.x] = -1;
    }
}