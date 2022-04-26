#include "PBFSolverCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

    uint prevIndex = g_Particleindex[DTid.x];
    
    g_SolveredPosition[prevIndex] = g_sortedNewPosition[DTid.x];
    
    float3 currVec = g_UpdatedVelocity[DTid.x];
    float3 impulse = g_Impulses[DTid.x];
    float3 oldVec = g_oldVelocity[prevIndex];
    float3 deltaVec = currVec + impulse - oldVec;
    float deltaVecLengthsq = dot(deltaVec, deltaVec);
    if (deltaVecLengthsq > g_MaxVeclocityDelta)
    {
        deltaVec *= g_MaxVeclocityDelta;
    }
    
    float3 finVec = oldVec + deltaVec;
    g_SolveredVelocity[prevIndex] = finVec;
}