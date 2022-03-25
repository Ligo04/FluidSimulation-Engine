#include "NerghborSearchCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x < g_ParticleNums)
    {
        int3 cellPos = ceil(g_ParticlePosition[DTid.x] * g_CellFactor);
        g_cellHash[DTid.x] = hashFunc(cellPos) % (2 * g_ParticleNums); //hash table size is double particle nums
        g_cellIndexs[DTid.x] = g_acticeIndexs[DTid.x];
    }
    else
    {
        //The maximum value assigned to a redundant thread
        g_cellHash[DTid.x] = -1;
        g_cellIndexs[DTid.x] = -1;
    }
}