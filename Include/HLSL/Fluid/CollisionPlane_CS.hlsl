#include "PBFSolverCommon.hlsli"

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    float3 currPos = g_sortedNewPosition[DTid.x];
    
    int count = 0;
    int i = 0;
    [unroll]
    for (i = 0; i < g_PlaneNums; ++i)
    {
        float distance = sdfPlane(currPos, g_Plane[i].xyz, g_Plane[i].w);
        if (distance - g_CollisionDistance < g_CollisionThreshold && count<g_MaxCollisionPlanes)
        {
            int index = DTid.x * g_MaxCollisionPlanes + count;
            g_CollisionPlanes[index] = g_Plane[i];
            count++;
        }
    }
    
    g_CollisionCounts[DTid.x] = count;
    
}