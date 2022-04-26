#include "PBFSolverCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    float3 currPos = g_sortedNewPosition[DTid.x];
    float3 currVec = g_UpdatedVelocity[DTid.x];
    float3 oldVec = g_sortedOldPosition[DTid.x];
    
    
    uint counter = g_ContactCounts[DTid.x];
    
    float3 deltaTotalVec = float3(0.0f, 0.0f, 0.0f);
    float3 etaTotal = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < counter; ++i)
    {
        //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        //get the cell particle pos
        float3 neighborParticlePos = g_sortedNewPosition[neightborParticleIndex];
        //r=p_i-p_j
        float3 r = currPos - neighborParticlePos;
        //v_j-v_i
        float3 deltaVelocity = g_UpdatedVelocity[neightborParticleIndex] - currVec;
        
        float3 currGrad = WSpikyGrad(r, g_sphSmoothLength);
        
        //vorticity
        float curlLength = g_Curl[neightborParticleIndex].w;
        float density_j = g_Density[neightborParticleIndex];
        if (density_j != 0)
        {
            float3 eta_j = (curlLength * currGrad) * (1.0f / density_j);
            etaTotal += eta_j;
        }
                
        //XSPH
        float3 deltaVec_j = deltaVelocity * currGrad;
        deltaTotalVec += deltaVec_j;
    }
    
    float3 impulse = float3(0.0f, 0.0f, 0.0f);
    //vorticity
    if (counter != 0)
    {
        etaTotal = normalize(etaTotal);
        float3 currCurl = g_Curl[DTid.x].xyz;
        float3 force = g_VorticityConfinement * cross(etaTotal, currCurl);
        
        impulse += g_DeltaTime * force;
        
        impulse += g_VorticityC * deltaTotalVec;
        
        
        //float3 impulse = currVec * g_InverseDeltaTime;
        //g_UpdatedVelocity[DTid.x] += g_VorticityC * deltaTotalVec;
    }
    
    // solve plane 
    uint planeCounts = g_CollisionCounts[DTid.x];
    float3 restitutionVec = float3(0.0f, 0.0f, 0.0f);
    for (uint j = 0; j < planeCounts; ++j)
    {
        float4 plane = g_CollisionPlanes[DTid.x * g_MaxCollisionPlanes + j];
        float distance = sdfPlane(currPos, plane.xyz, plane.w);
        float oldVecD = dot(oldVec, plane.xyz);
        if (distance < g_CollisionDistance && oldVecD < 0.0f)
        {
            float currVecD = dot(currVec, plane.xyz);
            float restitutionD = oldVecD * g_Restituion + currVecD;
            restitutionVec += plane.xyz * (-restitutionD);
        }
    }
    
    
    impulse += restitutionVec;
    g_Impulses[DTid.x] = impulse;
}