#include "NerghborSearchCommon.hlsli"

//一次申请最大内容
groupshared uint localPrefix[THREAD_NUM_X];


//excusive scan
void PresumLocalCounter(uint GI : SV_GroupIndex)
{
    //up sweep
    uint d = 0;
    uint i = 0;
    uint offset = 1;
    //total num must be 2^N
    uint totalNum = g_CounterNums<THREAD_NUM_X?g_CounterNums:THREAD_NUM_X;
    totalNum=pow(2,ceil(log2(totalNum)));
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

void LocalCounterAdd(uint GI : SV_GroupIndex,int num)
{
    localPrefix[GI]+=num;
    GroupMemoryBarrierWithGroupSync();
}


[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID,uint GI : SV_GroupIndex,uint3 Gid:SV_GroupID)
{
    //load date to shared memort
    if (DTid.x < g_CounterNums)
    {
        localPrefix[GI.x] = g_SrcCounters[DTid.x];
    }
    else
    {
        localPrefix[GI.x] = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    //calc counter prefix sum
    PresumLocalCounter(GI);
    
    //ouput 
    if (DTid.x < g_CounterNums)
    {
        g_DstCounters[DTid.x] = localPrefix[GI];
    }

    DeviceMemoryBarrierWithGroupSync();

    if (Gid.x > 0)
    {
        //get pred sum
        uint sum = 0;
        for (uint i = 0; i < Gid.x; ++i)
        {
            sum += g_DstCounters[(i + 1) * THREAD_NUM_X - 1]+g_SrcCounters[(i + 1) * THREAD_NUM_X - 1];
        }
        LocalCounterAdd(GI, sum);

        //output 
        if (DTid.x < g_CounterNums)
        {
            g_DstCounters[DTid.x] = localPrefix[GI];
        }
    }
}