#include "NerghborSearchCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID,uint3 Gid:SV_GroupID)
{
    //get curr digit 
    uint digit = get4Bits(g_SrcKey[DTid.x], g_CurrIteration);


    //global dispatch
    uint counterIndex = digit * g_BlocksNums + Gid.x;
    uint globalPos = g_SrcPrefixLocal[DTid.x] + g_DispatchCounters[counterIndex];
    g_DstKey[globalPos] = g_SrcKey[DTid.x];
    g_DstVal[globalPos] = g_SrcVal[DTid.x];
}