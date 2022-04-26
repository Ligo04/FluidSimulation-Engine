#include "NerghborSearchCommon.hlsli"

groupshared float3 boundmin[THREAD_BOUNDS_X];
groupshared float3 boundmax[THREAD_BOUNDS_X];

[numthreads(THREAD_BOUNDS_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex, uint3 GId : SV_GroupID)
{
    if (g_BoundsFinalizeNums == 1)
    {
       //only one 
        if (GI==0)
        {
            g_Bounds[0] = g_WriteBoundsLower[0] - float3(g_CellSize, g_CellSize, g_CellSize);
            g_Bounds[1] = g_WriteBoundsUpper[0] + float3(g_CellSize, g_CellSize, g_CellSize);
        }
    }
    else
    {
        float3 currBoundmin = float3(0.0f, 0.0f, 0.0f);
        float3 currBoundmax = float3(0.0f, 0.0f, 0.0f);
        if (DTid.x < g_BoundsFinalizeNums)
        {
            currBoundmin = g_WriteBoundsLower[DTid.x];
            currBoundmax = g_WriteBoundsUpper[DTid.x];
        }
        else
        {
            currBoundmin = g_WriteBoundsLower[g_BoundsFinalizeNums - 1];
            currBoundmax = g_WriteBoundsUpper[g_BoundsFinalizeNums - 1];
        }
        boundmin[GI] = currBoundmin;
        boundmax[GI] = currBoundmax;
        
        uint d = 0;
        uint offset = 1;
        uint totalNum = THREAD_BOUNDS_X;
        //min
        for (d = totalNum >> 1; d > 0; d >>= 1)
        {
            GroupMemoryBarrierWithGroupSync();
            if (GI < d)
            {
                uint ai = offset * (2 * GI + 1) - 1;
                uint bi = offset * (2 * GI + 2) - 1;

                boundmin[bi] = min(boundmin[ai], boundmin[bi]);
                boundmax[bi] = max(boundmax[ai], boundmax[bi]);
            }
            offset *= 2;
        }
    
        GroupMemoryBarrierWithGroupSync();
        if (GI == 0)
        {
            g_Bounds[0] = boundmin[totalNum - 1] - float3(g_CellSize, g_CellSize, g_CellSize);
            g_Bounds[1] = boundmax[totalNum - 1] + float3(g_CellSize, g_CellSize, g_CellSize);
        }
    }
}