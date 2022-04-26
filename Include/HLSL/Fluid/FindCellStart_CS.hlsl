#include "NerghborSearchCommon.hlsli"

//load hashid at pos+1
groupshared uint sharedHash[THREAD_NUM_X+1];

[numthreads(THREAD_NUM_X, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID,uint GI:SV_GroupIndex )
{
    uint hashValue = 0;
    if (DTid.x < g_ParticleNums)
    {
        hashValue = g_SrcKey[DTid.x];
        sharedHash[GI + 1] = hashValue;

        if (GI == 0 && DTid.x > 0)
        {
            sharedHash[0] = g_SrcKey[DTid.x - 1];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (DTid.x < g_ParticleNums)
    {
        //If It's equal to the last one
        if (DTid.x == 0 || hashValue != sharedHash[GI])
        {
            g_CellStart[hashValue] = DTid.x;
            if (DTid.x > 0)
            {
                //the last cell end pos
                g_CellEnd[sharedHash[GI]] = DTid.x;
            }
        }
        //thread the last one output
        if (DTid.x == g_ParticleNums - 1)
        {
            g_CellEnd[hashValue] = DTid.x + 1;
        }
    }
}