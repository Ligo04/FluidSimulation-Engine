#include "PBFSolverCommon.hlsli"

// StructuredBuffer<float3> g_oldPosition:register(t0);
// RWStructuredBuffer<float3> g_PredPosition:register(u0);
//RWStructuredBuffer<float3> g_newVelocity : register(u1);

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x > g_ParticleNums)
    {
        return;
    }

    //now only gravity  
    //vi<=vi+ deltaT * g
    float3 vec = g_oldVelocity[DTid.x] + g_DeltaTime * g_Gravity;
    //xi*<=xi+ deltaT * v
    //g_PredPosition[DTid.x]=g_oldPosition[DTid.x]+g_DeltaTime*vec;
    float3 pos = g_oldPosition[DTid.x] + g_DeltaTime * vec;
    
    g_PredPosition[DTid.x] = pos;
    //update v
    g_newVelocity[DTid.x] = vec;
}