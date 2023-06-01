#include "NerghborSearchCommon.hlsli"

//0:min 1:max
groupshared float3 boundmin[THREAD_BOUNDS_X];
groupshared float3 boundmax[THREAD_BOUNDS_X];

[numthreads(THREAD_BOUNDS_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex, uint3 GId : SV_GroupID)
{
    float3 currBoundmin = float3(0.0f, 0.0f, 0.0f);
    float3 currBoundmax = float3(0.0f, 0.0f, 0.0f);
    if (DTid.x < g_BoundsGroups)
    {
        currBoundmin = g_ReadBoundsLower[DTid.x];
        currBoundmax = g_ReadBoundsUpper[DTid.x];
    }
    else
    {
        currBoundmin = g_ReadBoundsLower[g_BoundsGroups-1];
        currBoundmax = g_ReadBoundsUpper[g_BoundsGroups-1];
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
        g_WriteBoundsLower[GId.x] = boundmin[totalNum - 1];
        g_WriteBoundsUpper[GId.x] = boundmax[totalNum - 1];
    }
}