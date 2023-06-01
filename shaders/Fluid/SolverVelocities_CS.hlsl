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
    float3 oldVec = g_sortedVelocity[DTid.x];
    
    
    uint counter = g_ContactCounts[DTid.x];
    
    float3 deltaTotalVec = float3(0.0f, 0.0f, 0.0f);
    float3 etaTotal = float3(0.0f, 0.0f, 0.0f);
    float density;
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
        
        
        //vorsitory confinement
        float neighCurlLength = g_Curl[neightborParticleIndex].w;
        etaTotal += currGrad * neighCurlLength;                             //r4
                
           //XSPH
        float3 deltaVec_j = deltaVelocity * WSpiky(r, g_sphSmoothLength);
        deltaTotalVec += deltaVec_j;
        
            //density
        density += WPoly6(r, g_sphSmoothLength);

    }
    
    float3 impulse = float3(0.0f, 0.0f, 0.0f);
    //vorticity Confinement
    if (length(etaTotal) > 0.0f && g_VorticityConfinement > 0.0f && density>0.0f)
    {
        float epsilon = g_DeltaTime * g_DeltaTime * g_InverseDensity_0 * g_VorticityConfinement;
        
        float3 currCurl = g_Curl[DTid.x].xyz;       //r2
        
        float3 N = normalize(etaTotal);
        float3 force = cross(N, currCurl);
        
        impulse += epsilon * force;

    }
    
    //XSPH
    impulse += g_VorticityC * deltaTotalVec;
    
    // solve plane 
    uint planeCounts = g_CollisionCounts[DTid.x];
    uint resCounts = 0;
    float3 restitutionVec = float3(0.0f, 0.0f, 0.0f);
    for (uint j = 0; j < planeCounts; ++j)
    {
        float4 plane = g_CollisionPlanes[DTid.x * g_MaxCollisionPlanes + j];
        float distance = sdfPlane(currPos, plane.xyz, plane.w) - 1.001f * g_CollisionDistance;
        
        float oldVecD = dot(oldVec, plane.xyz);
        if (distance < 0.0f && oldVecD < 0.0f)
        {
            float currVecD = dot(currVec, plane.xyz);
            float restitutionD = oldVecD * g_Restituion + currVecD;
            restitutionVec += plane.xyz * (-restitutionD);
            resCounts++;
        }
    }
    resCounts = max(resCounts, 1);
    restitutionVec /= resCounts;
    
    
    impulse += restitutionVec;
    g_Impulses[DTid.x] = impulse;
}