struct Anisortopy
{
    float4 q1;
    float4 q2;
    float4 q3;
};

//Predict pos
StructuredBuffer<float3> g_oldPosition : register(t0);
StructuredBuffer<float3> g_oldVelocity : register(t1);
RWStructuredBuffer<float3> g_PredPosition : register(u0);
RWStructuredBuffer<float3> g_newVelocity : register(u1);
//reorder particle
StructuredBuffer<float3> g_newPostion : register(t2);
StructuredBuffer<uint> g_ParticleIndex : register(t3);
StructuredBuffer<float3> g_Bounds : register(t4);
RWStructuredBuffer<float3> g_sortedOldPosition : register(u2);
RWStructuredBuffer<float3> g_sortedNewPosition : register(u3);
RWStructuredBuffer<float3> g_sortedVelocity : register(u4);

//contact and collision
StructuredBuffer<uint> g_CellStart : register(t6);
StructuredBuffer<uint> g_CellEnd : register(t7);
RWStructuredBuffer<uint> g_Contacts : register(u5);
RWStructuredBuffer<uint> g_ContactCounts : register(u6);
RWStructuredBuffer<uint> g_CollisionCounts : register(u7);
RWStructuredBuffer<float4> g_CollisionPlanes : register(u8);

//calclagrangeMUltiplier
RWStructuredBuffer<float> g_Density : register(u9);
RWStructuredBuffer<float> g_LambdaMultiplier:register(u10);
//calcDisplacement
RWStructuredBuffer<float3> g_DeltaPosition:register(u11);
//ADDDeltaPosition
RWStructuredBuffer<float3> g_UpdatedPosition : register(u12);
//UpdateVeclosity
RWStructuredBuffer<float3> g_UpdatedVelocity:register(u13);
RWStructuredBuffer<float4> g_Curl : register(u14);
RWStructuredBuffer<float3> g_Impulses : register(u15);
//UpdatePosition
StructuredBuffer<uint>  g_Particleindex:register(t8); 
RWStructuredBuffer<float3> g_SolveredPosition:register(u16);
RWStructuredBuffer<float3> g_SolveredVelocity:register(u17);

//anisotropy
RWStructuredBuffer<float3> g_SmoothPosition : register(u18);
RWStructuredBuffer<float3> g_SmoothPositionOmega : register(u19);
RWStructuredBuffer<Anisortopy> g_Anisortopy : register(u20);


#define THREAD_NUM_X 256


cbuffer PBFConstant : register(b0)
{
    uint g_ParticleNums;
    float g_CellSize;
    float g_ScorrK;      //seem surface tension k=0.1
    float g_ScorrN;      //n = 4 

    float g_Poly6Coff;
    float g_SpikyCoff;
    float g_SpikyGradCoff;
    float g_InverseDensity_0; // 1/p_0


    float g_VorticityConfinement;  //epsilon    The strength of the turbine control force   (user set)
    float g_VorticityC;            //c=0.01
    float g_sphSmoothLength;
    float g_DeltaQ;           //delta q=0.1h,...,0.3h

    float3 g_Gravity;
    int g_MaxNeighborPerParticle;
    
    float g_CollisionDistance;
    float g_CollisionThreshold;
    float g_ParticleRadiusSq;
    float g_SOR;               //successive over-relaxation (SOR)
    
    float g_MaxSpeed;
    float g_MaxVeclocityDelta;
    float g_Restituion;
    float g_LaplacianSmooth;
    
    float g_AnisotropyScale;             //AnisotropyScale
    float g_AnisotropyMin;
    float g_AnisotropyMax;
    float g_Pad0;
    
    float g_StaticFriction;
    float g_DynamicFriction;
    float g_Pad11[2];
}

cbuffer PBFChanges : register(b1)
{
    float g_DeltaTime;
    float g_InverseDeltaTime;
    float g_LambdaEps;   //Relaxation parameter(soft constraint use)
    float g_pad1;
}

cbuffer PBFBoundary : register(b2)
{
    float4 g_Plane[6];
    int g_PlaneNums;
    int g_MaxCollisionPlanes;
    float g_pad2[2];
}


//W_poly6(r,h)=315/(64*PI*h^9) * (h^2-r^2)^3
float WPoly6(float3 r, float h)
{
    float radius = length(r);
    float res = 0.0f;
    if (radius <= h)
    {
        float item = dot(h, h) - dot(radius, radius);
        res = g_Poly6Coff * pow(item, 3);
    }
    return res;
}

//W_Spiky(r,h)=15/(PI*h^6) * (h-r)^3
float WSpiky(float3 r, float h)
{
    float radius = length(r);
    float res = 0.0f;
    if (radius <= h)
    {
        float item = h - radius;
        res = g_SpikyCoff * pow(item, 3);
    }
    return res;
}

//W_Spiky_Grad(r,h)= -45/(PI*h^6) * (h-r)^2*(r/|r|);
float3 WSpikyGrad(float3 r, float h)
{
    float radius = length(r);
    float3 res = float3(0.0f, 0.0f, 0.0f);
    if (radius <= h && radius > 0)
    {
        float item = h - radius;
        res = g_SpikyGradCoff * pow(item, 2) * normalize(r);
    }
    return res;
}


//calculate hash
uint hashFunc(uint3 key)
{
    uint p1 = 73856093 * key.x;
    uint p2 = 19349663 * key.y;
    uint p3 = 83492791 * key.z;
    return p1 ^ p2 ^ p3;
}



uint GetCellHash(int3 cellPos)
{
    //uint hash = ((cellPos.z << 14) & (~bitmask1)) + ((cellPos.y << 7) & (~bitmask2)) + cellPos.x;
    //uint hash = (cellPos.z << 14) + (cellPos.y << 7) + cellPos.x;
    uint hash = hashFunc(cellPos);
    return hash % (2 * g_ParticleNums);
}

//sdf plane function
float sdfPlane(float3 p, float3 n, float h)
{
    return dot(p, n) + h;
}


float sqr(float3 ele)
{
    return ele.x * ele.x + ele.y * ele.y + ele.z * ele.z;
}

float Wsmooth(float3 r,float h)
{
    float radius = length(r);
    float res = 0.0f;
    if (radius < h)
    {
        float item = radius / h;
        res = 1 - pow(item, 3);

    }
    return res;
}