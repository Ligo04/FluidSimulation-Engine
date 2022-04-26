#include "NerghborSearchCommon.hlsli"

//0:min 1:max
groupshared float3 boundmin[THREAD_BOUNDS_X];
groupshared float3 boundmax[THREAD_BOUNDS_X];

[numthreads(THREAD_BOUNDS_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID,uint GI:SV_GroupIndex,uint3 GId:SV_GroupID)
{
    
    float3 currPos = float3(0.0f, 0.0f, 0.0f);
    if (DTid.x < g_ParticleNums)
    {
        currPos = g_ParticlePosition[DTid.x];
    }
    else
    {
        currPos = g_ParticlePosition[g_ParticleNums - 1];

    }
    boundmin[GI] = currPos;
    boundmax[GI] = currPos;
    
    
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
        float3 min = boundmin[totalNum - 1];
        float3 max = boundmax[totalNum - 1];
        g_ReadBoundsLower[GId.x] = min;
        g_ReadBoundsUpper[GId.x] = max;
    }
}