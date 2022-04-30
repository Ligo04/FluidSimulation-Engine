#include "PBFSolverCommon.hlsli"

#define M_SQRT3  sqrt(3)
#define FLOAT_MIN 0.00001f
//Cardano¡¯ smethod  calc eigenvalue
float3 dsyevc3(matrix A)
{
    float m, c1, c0;
    
   // Determine coefficients of characteristic poynomial. We write
   //       | a   d   f  0 |
   //  A =  | d*  b   e  0 |
   //       | f*  e*  c  0 |
   //       | 0   0   0  0 |
    float de = A._12 * A._23;
    float dd = dot(A._12, A._12);
    float ee = dot(A._23, A._23);
    float ff = dot(A._13, A._13);
    
    m = A._11 + A._22 + A._33;
    c1 = (A._11 * A._22 + A._11 * A._33 + A._22 * A._33) - (dd + ee + ff);                        // a*b + a*c + b*c - d^2 - e^2 - f^2
    c0 = A._33 * dd + A._11 * ee + A._22 * ff - A._11 * A._22 * A._33 - 2.0f * A._13 * de;        // c*d^2 + a*e^2 + b*f^2 - a*b*c - 2*f*d*e
    
    float p, sqrt_p, q, c, s, phi;
    p = dot(m,m) - 3.0f * c1;
    q = m * (p - (3.0f / 2.0f) * c1) - (27.0f / 2.0f) * c0;
    sqrt_p = sqrt(abs(p));
    
    phi = 27.0f * (0.25f * dot(c1, c1) * (p - c1) + c0 * (q + 27.0f / 4.0f * c0));
    phi = (1.0 / 3.0) * atan2(sqrt(abs(phi)), q);
    
    c = sqrt_p * cos(phi);
    s = (1.0 / M_SQRT3) * sqrt_p * sin(phi);
    
    float3 res;
    res.y = (1.0f / 3.0f) * (m - c);
    res.z = res.y + s;
    res.x = res.y + c;
    res.y -= s;
    
    
    
    return res;

}


matrix GetEigenVectors(matrix C, float3 eigenValues)
{
    matrix cofactor;
    cofactor[3] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    matrix diag;
    diag[0] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diag[1] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diag[2] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diag[3] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 3;++i)
    {
        diag._11_22_33 = float3(eigenValues[i], eigenValues[i], eigenValues[i]);
        matrix temp = C - diag;
        float m11 = temp[1][1] * temp[2][2] - temp[1][2] * temp[2][1];
        float m12 = temp[1][0] * temp[2][2] - temp[1][2] * temp[2][0];
        float m13 = temp[1][0] * temp[2][1] - temp[1][1] * temp[2][0];
        
        cofactor[i] = float4(normalize(float3(m11, m12, m13)), 0.0f);
    }
    cofactor = transpose(cofactor);
    return cofactor;
}

[numthreads(THREAD_NUM_X, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= g_ParticleNums)
    {
        return;
    }
    
    uint prevIndex = g_Particleindex[DTid.x];
    
    float3 currPos = g_sortedNewPosition[DTid.x];
    float3 currSmoothPos = g_SmoothPosition[DTid.x];
    float3 currSmoothOmega = g_SmoothPositionOmega[DTid.x];
    
    
    matrix omegaMaritalTotal;
    float wTotal = 0.0f;
    
    matrix symmetric;
    symmetric._11_21_31_41 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._12_22_32_42 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._13_23_33_43 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._14_24_34_44 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    uint neightborCounter = g_ContactCounts[DTid.x];
    uint i;
    for (i = 0; i < neightborCounter;++i)
    {
        //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        
        float3 neighborPos = g_sortedNewPosition[neightborParticleIndex];
        float3 deltaPos = neighborPos - currSmoothOmega;
        
        symmetric[0] = deltaPos.x * float4(deltaPos, 0.0f);
        symmetric[1] = deltaPos.y * float4(deltaPos, 0.0f);
        symmetric[2] = deltaPos.z * float4(deltaPos, 0.0f);
        
        float W_ij = Wsmooth(deltaPos,2*g_sphSmoothLength);
        wTotal += W_ij;
        
        omegaMaritalTotal += W_ij * symmetric;
    }
    
    matrix C;
    Anisortopy ani;
    
    if (neightborCounter==0)
    {
        
        float3x3 unitMatrix;
        unitMatrix._11_12_13 = float3(1.0f, 0.0f, 0.0f);
        unitMatrix._21_22_23 = float3(0.0f, 1.0f, 0.0f);
        unitMatrix._31_32_33 = float3(0.0f, 0.0f, 1.0f);
        
        ani.q1 = float4(unitMatrix._11_12_13, g_AnisotropyMin);
        ani.q2 = float4(unitMatrix._21_22_23, g_AnisotropyMin);
        ani.q3 = float4(unitMatrix._31_32_33, g_AnisotropyMin);

        g_Anisortopy[prevIndex] = ani;
    }
    else
    {
        C = omegaMaritalTotal / wTotal;
        float3 eigenvalues = dsyevc3(C);
        
        if (abs(eigenvalues.x - eigenvalues.z) < FLOAT_MIN)
        {
            float3x3 unitMatrix;
            unitMatrix._11_12_13 = float3(1.0f, 0.0f, 0.0f);
            unitMatrix._21_22_23 = float3(0.0f, 1.0f, 0.0f);
            unitMatrix._31_32_33 = float3(0.0f, 0.0f, 1.0f);
        
            ani.q1 = float4(unitMatrix._11_12_13, g_AnisotropyMin);
            ani.q2 = float4(unitMatrix._21_22_23, g_AnisotropyMin);
            ani.q3 = float4(unitMatrix._31_32_33, g_AnisotropyMin);

            g_Anisortopy[prevIndex] = ani;
        }
        else 
        {
            matrix eigenVectors = GetEigenVectors(C, eigenvalues);
            
            
            eigenvalues.y = max(eigenvalues.y, eigenvalues.x / 4);
            eigenvalues.z = max(eigenvalues.z, eigenvalues.x / 4);
            
            matrix diag;
            diag[0] = float4(1.0f / eigenvalues.x, 0.0f, 0.0f, 0.0f);
            diag[1] = float4(0.0f, 1.0f / eigenvalues.x, 0.0f, 0.0f);
            diag[2] = float4(0.0f, 0.0f, 1.0f / eigenvalues.x, 0.0f);
            diag[3] = float4(0.0f, 0.0f, 0.0f, 0.0f);
           
            
          
            matrix G = (1.0f / (2 * g_sphSmoothLength)) * eigenVectors * diag * transpose(eigenVectors);
            
            G = G * g_AnisotropyScale;
            
            ani.q1.w = clamp(G[0][0], g_AnisotropyMin, g_AnisotropyMax);
            ani.q2.w = clamp(G[1][1], g_AnisotropyMin, g_AnisotropyMax);
            ani.q3.w = clamp(G[2][2], g_AnisotropyMin, g_AnisotropyMax);
        
            g_Anisortopy[prevIndex] = ani;
            
            if (abs(eigenvalues.x - eigenvalues.y) > FLOAT_MIN && abs(eigenvalues.y - eigenvalues.z) < FLOAT_MIN)
            {
                float3 v1 = eigenVectors._11_21_31;
            }
            else
            {
                float3 v1 = eigenVectors._11_21_31;
                float3 v3 = eigenVectors._13_23_33;
                v3 = v3 - v1 * dot(v1, v3);
                float3 v2 = cross(v1, v3);
                ani.q1.xyz = normalize(v1);
                ani.q2.xyz = normalize(v2);
                ani.q3.xyz = normalize(v3);
            }
            
            g_Anisortopy[prevIndex] = ani;
        }
    }
}