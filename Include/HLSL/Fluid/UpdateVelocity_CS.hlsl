#include "PBFSolverCommon.hlsli"

//RWStructuredBuffer<float3> g_sortedOldPosition : register(u2);
//RWStructuredBuffer<float3> g_UpdatedPosition:register(u7);
//RWStructuredBuffer<float3> g_UpdatedVelocity:register(u8);

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

    float3 oldPos = g_sortedOldPosition[DTid.x];
    float3 updatePos = g_sortedNewPosition[DTid.x];

    float3 newVec = g_InverseDeltaTime * (updatePos - oldPos);
    g_UpdatedVelocity[DTid.x] = newVec;
}