
StructuredBuffer<float3> g_ParticlePosition : register(t0);
StructuredBuffer<uint> g_acticeIndexs : register(t1);

RWStructuredBuffer<float3> g_ReadBoundsLower : register(u0);
RWStructuredBuffer<float3> g_ReadBoundsUpper : register(u1);
RWStructuredBuffer<float3> g_WriteBoundsLower : register(u2);
RWStructuredBuffer<float3> g_WriteBoundsUpper : register(u3);
RWStructuredBuffer<float3> g_Bounds : register(u4);
//calc hash and index

RWStructuredBuffer<uint> g_cellHash : register(u5);
RWStructuredBuffer<uint> g_cellIndexs : register(u6);

//GPU Radix sort get the local counter(1 phase) and global dispatch(3 phase)
StructuredBuffer<uint> g_SrcKey : register(t2);
RWStructuredBuffer<uint> g_Counters : register(u7);
RWStructuredBuffer<uint> g_Prefix : register(u8);

//GPU Radix sort calc prefix block sum(2 phase)
StructuredBuffer<uint> g_SrcCounters : register(t3);
RWStructuredBuffer<uint> g_DstCounters : register(u9);

//GPU Radix sort calc global position to output des(3 phase)
StructuredBuffer<uint> g_SrcVal : register(t4);
StructuredBuffer<uint> g_SrcPrefixLocal : register(t5);
StructuredBuffer<uint> g_DispatchCounters:register(t6);
RWStructuredBuffer<uint> g_DstKey:register(u10);
RWStructuredBuffer<uint> g_DstVal:register(u11);

//find the cell start 
RWStructuredBuffer<uint> g_CellStart:register(u12);
RWStructuredBuffer<uint> g_CellEnd:register(u13);

#define RADIX_D 4
#define RADIX_R 16
#define THREAD_NUM_X 1024
#define THREAD_BOUNDS_X THREAD_NUM_X/4

cbuffer NerghborSearchConstant : register(b0)
{
    float g_CellSize;
    int g_CurrIteration;   
    uint g_KeyNums;
    uint g_CounterNums;

    uint g_ParticleNums;
    uint g_BlocksNums;
    uint g_BoundsGroups;
    uint g_BoundsFinalizeNums;
}

//calculate hash
uint hashFunc(int3 key)
{
    int p1 = 73856093 * key.x;
    int p2 = 19349663 * key.y;
    int p3 = 83492791 * key.z;
    return p1 ^ p2 ^ p3;
}


uint GetCellHash(int3 cellPos)
{
    //uint3 gridSize = ((g_Bounds[1] - g_Bounds[0]) / g_CellSize) - 1;
    
    //cellPos = cellPos & gridSize;
    //return cellPos.z * gridSize.x * gridSize.y + cellPos.y * gridSize.x + cellPos.x;
    //uint bitmask1 = (1 << 14) - 1;
    //uint bitmask2 = (1 << 7) - 1;
    //uint hash = ((cellPos.z << 14) & (~bitmask1)) + ((cellPos.y << 7) & (~bitmask2)) + cellPos.x;
    //uint hash = (cellPos.z << 14) + (cellPos.y << 7) + cellPos.x;
    uint hash = hashFunc(cellPos);
    return hash % (2 * g_ParticleNums);
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


//uint2 bitmask2 = (((1 << 7) - 1) << uint2(14, 7));
//uint hash = cellPos.x + (cellPos.y << 14) + (cellPos.z << 7);
//return hash % 2097152;
    
//cellPos.yz = (cellPos.zy << uint2(14, 7)) & bitmask2 | (uint2(0, 0) & ~bitmask2);
//cellPos.y += cellPos.z;
//uint bitmask = (((1 << 7) - 1) << 0);
//cellPos.x = ((cellPos.x << 0) & bitmask) | (cellPos.y & ~bitmask);