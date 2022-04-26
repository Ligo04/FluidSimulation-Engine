#include "PBFSolverCommon.hlsli"


//Find Neighbor Paticle
[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }

    //curr particle pos
    float3 currPos = g_sortedNewPosition[DTid.x];
    float3 sceneMin = g_Bounds[0];
    int3 currCellPos = floor((currPos - sceneMin) / g_CellSize);
    
    //curr particle index
    //uint currParitcleIndex = g_ParticleIndex[DTid.x];
    
    int neighborCount = 0;
    int x = 0, y = 0, z = 0;
    [unroll(3)]
    for (z = -1; z <= 1; ++z)
    {
        [unroll(3)]
        for (y = -1; y <= 1; ++y)
        {
            [unroll(3)]
            for (x = -1; x <= 1; ++x)
            {
                //find 27 cell neighbor particle
                int3 neighCellPos = currCellPos + int3(x, y, z);
                if (neighCellPos.x < 0.0f || neighCellPos.y < 0.0f || neighCellPos.z < 0.0f)
                {
                    continue;
                }
                uint cellHash = GetCellHash(neighCellPos);
                uint neighborCellStart = g_CellStart[cellHash];
                uint neighborCellEnd = g_CellEnd[cellHash];
                if (neighborCellStart >= neighborCellEnd)
                {
                    continue;
                }
                for (uint index = neighborCellStart; index < neighborCellEnd; ++index)
                {
                    //get the cell particle pos
                    float3 neighborPartclePos = g_sortedNewPosition[index];
                    float3 distance = neighborPartclePos - currPos;
                    float distancesq = sqr(distance);
                    if (distancesq < g_ParticleRadiusSq && distancesq > 0.0f)
                    {
                        int contactsIndex = DTid.x * g_MaxNeighborPerParticle + neighborCount;
                        g_Contacts[contactsIndex] = index;
                        neighborCount++;
                    }
                    if (neighborCount == g_MaxNeighborPerParticle)
                    {
                        g_ContactCounts[DTid.x] = neighborCount;
                        return;
                    }
                }

            }
        }
    }
    
    g_ContactCounts[DTid.x] = neighborCount;

}