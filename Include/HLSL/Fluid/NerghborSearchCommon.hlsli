
//calc hash and index
StructuredBuffer<float3> g_ParticlePosition : register(t0);
StructuredBuffer<uint> g_acticeIndexs : register(t1);

RWBuffer<uint> g_cellHash : register(u0);
RWBuffer<uint> g_cellIndexs : register(u1);

//GPU Radix sort get the local counter(1 phase) and global dispatch(3 phase)
Buffer<uint> g_SrcKey : register(t2);
RWBuffer<uint> g_Counters : register(u2);
RWBuffer<uint> g_Prefix : register(u3);

//GPU Radix sort calc prefix block sum(2 phase)
Buffer<uint> g_SrcCounters : register(t3);
RWBuffer<uint> g_DstCounters : register(u4);

//GPU Radix sort calc global position to output des(3 phase)
Buffer<uint> g_SrcVal : register(t4);
Buffer<uint> g_SrcPrefixLocal : register(t5);
Buffer<uint> g_DispatchCounters:register(t6);
RWBuffer<uint> g_DstKey:register(u5);
RWBuffer<uint> g_DstVal:register(u6);

#define RADIX_D 4
#define RADIX_R 16
#define THREAD_NUM_X 1024

cbuffer Constant : register(b0)
{
    float g_CellFactor;
    int g_CurrIteration;   //µ±Ç°µü´ú
    uint g_KeyNums;
    uint g_CounterNums;

    uint g_ParticleNums;
    uint g_BlocksNums;
    float2 g_Pad;
}

//calculate hash
uint hashFunc(int3 key)
{
    const uint p1 = 73856093 * key.x;
    const uint p2 = 19349663 * key.y;
    const uint p3 = 83492791 * key.z;
    return p1 ^ p2 ^ p3;
}

//get LSD 4-bit 
uint4 get4Bits(uint4 num,int i)
{
    return ((num >> i*4) & 0xf);
}

//get LSD 4-bit 
uint get4Bits(uint num,int i)
{
    return ((num >> i*4) & 0xf);
}