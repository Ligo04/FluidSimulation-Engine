#include "NerghborSearchCommon.hlsli"


groupshared uint localPrefix[THREAD_NUM_X];

//excusive scan
void PresumLocal(uint GI : SV_GroupIndex)
{
    //up sweep
    uint d = 0;
    uint i = 0;
    uint offset = 1;
    uint totalNum = THREAD_NUM_X;
    [unroll]
    for (d = totalNum>>1; d > 0; d >>= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        if (GI < d)
        {
            uint ai = offset * (2 * GI + 1) - 1;
            uint bi = offset * (2 * GI + 2) - 1;

            localPrefix[bi] += localPrefix[ai];
        }
        offset *= 2;
    }

    //clear the last element
    if (GI == 0)
    {
        localPrefix[totalNum-1] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    //Down-sweep
    [unroll]
    for (d = 1; d < totalNum; d *= 2)
    {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();

        if (GI < d)
        {
            uint ai = offset * (2 * GI + 1) - 1;
            uint bi = offset * (2 * GI + 2) - 1;

            uint tmp = localPrefix[ai];
            localPrefix[ai] = localPrefix[bi];
            localPrefix[bi] += tmp;
        }
    }
    GroupMemoryBarrierWithGroupSync();
}

//one thread process one element
[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID
,uint3 Gid:SV_GroupID )
{
    //get curr digit correspend to 4-bit LSD
    uint digit = get4Bits(g_SrcKey[DTid.x], g_CurrIteration);

    //counter
    [unroll(RADIX_R)]
    for (uint r = 0; r < RADIX_R; ++r)
    {
        //load to share memory 
        localPrefix[GI] = (digit == r ? 1 : 0);
        GroupMemoryBarrierWithGroupSync();

        //prefix sum according to r in this blocks
        PresumLocal(GI);

        //ouput the total sum to counter
        if (GI == THREAD_NUM_X - 1)
        {
            uint counterIndex = r * g_BlocksNums + Gid.x;
            uint counter=localPrefix[GI];
            //output prefix sum according to r
            if (digit == r)
            {
                counter++;
            }
            g_Counters[counterIndex] = counter;
        }

        //output prefix sum according to r
        if(digit==r)
        {
            g_Prefix[DTid.x] = localPrefix[GI];
        }
        
        GroupMemoryBarrierWithGroupSync();
    }
}


