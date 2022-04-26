#include "PBFSolverCommon.hlsli"



[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    float3 currPos = g_sortedNewPosition[DTid.x];
    
    
    float3 solverContactPos = float3(0.0f, 0.0f, 0.0f);
    int collisionCount = g_CollisionCounts[DTid.x];
    int i = 0;
    for (i = 0; i < collisionCount; ++i)
    {
        int index = DTid.x * g_MaxCollisionPlanes + i;
        float4 currPlane = g_CollisionPlanes[index];
        float distance = sdfPlane(currPos, currPlane.xyz, currPlane.w);
        float3 sdfPos = (-distance) * currPlane.xyz;
        solverContactPos += sdfPos;

    }
    currPos += solverContactPos;
    
    g_UpdatedPosition[DTid.x] = currPos;
}