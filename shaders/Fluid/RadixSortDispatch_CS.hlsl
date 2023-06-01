#include "NerghborSearchCommon.hlsli"


[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID,uint3 Gid:SV_GroupID)
{
    //get curr digit 
    uint digit = get4Bits(g_cellHash[DTid.x], g_CurrIteration);


    //global dispatch
    uint counterIndex = digit * g_BlocksNums + Gid.x;
    uint globalPos = g_SrcPrefix[DTid.x] + g_DstCounters[counterIndex];
    g_DstKey[globalPos] = g_cellHash[DTid.x];
    g_DstVal[globalPos] = g_cellIndexs[DTid.x];
}