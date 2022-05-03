#include "PBFSolverCommon.hlsli"

//StructuredBuffer<float3> g_oldPosition:register(t0);
// StructuredBuffer<float3> g_newPostion:register(t1);
// StructuredBuffer<float3> g_newVelocity:register(t2);
// StructuredBuffer<uint> g_ParticleIndex:register(t3);
// RWStructuredBuffer<float3> g_sortedOldPosition:register(u2);
// RWStructuredBuffer<float3> g_sortedNewPosition:register(u3);
// RWStructuredBuffer<float3> g_sortedVelocity:register(u4);

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