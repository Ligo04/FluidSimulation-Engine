#include "PBFSolverCommon.hlsli"



[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    float3 currPos = g_sortedNewPosition[DTid.x];                  
    float3 oldPos = g_sortedOldPosition[DTid.x];
    
    int collisionCount = g_CollisionCounts[DTid.x];
    int i = 0;
    for (i = 0; i < collisionCount; ++i)
    {
        int index = DTid.x * g_MaxCollisionPlanes + i;
        float4 currPlane = g_CollisionPlanes[index];
        float distance = sdfPlane(currPos, currPlane.xyz, currPlane.w) - g_CollisionDistance; //d
        if (distance < 0.0f)
        {
            float3 sdfPos = (-distance) * currPlane.xyz; 
            
            //friction model
            float3 deltaPos = currPos - oldPos;
            float deltaX = dot(deltaPos, currPlane.xyz);
            float3 deltaDistane = (-deltaX) * currPlane.xyz + deltaPos; //DeltaX 
            float deltaLength = dot(deltaDistane, deltaDistane);
            [flatten]
            if (deltaLength < (g_StaticFriction * distance))        //|deltaX|< u_s*disctance
            {
                sdfPos -= deltaDistane;
            }
            else
            {
                float dynamicFriction = min((-distance) * 0.01f * rsqrt(deltaLength), 1.0f); //
                sdfPos -= dynamicFriction * (deltaDistane);
            }
            currPos += sdfPos;
        }
    }
    
    g_UpdatedPosition[DTid.x] = currPos;
}