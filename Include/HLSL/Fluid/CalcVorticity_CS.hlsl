#include "PBFSolverCommon.hlsli"


//Clac curl
[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

   //curr neighbor count
    uint neightborCount = g_ContactCounts[DTid.x];
    
    //curr particle pos
    float3 currPos = g_sortedNewPosition[DTid.x];
    float3 currVec = g_UpdatedVelocity[DTid.x];
    float3 currOmega = float3(0.0f, 0.0f, 0.0f);
    
    uint i = 0;
    for (i = 0; i < neightborCount; ++i)
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
         //calc omega
        float3 omega_j = cross(deltaVelocity, currGrad);
        currOmega += omega_j;
    }
    
  
    float curlLength = length(currOmega);
    g_Curl[DTid.x] = float4(currOmega.xyz, curlLength);
    
}