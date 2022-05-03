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
    float de = A[0][1] * A[1][2];
    float dd = dot(A[0][1], A[0][1]);
    float ee = dot(A[1][2], A[1][2]);
    float ff = dot(A[0][2], A[0][2]);
    
    m = A[0][0] + A[1][1] + A[2][2];
    c1 = (A[0][0] * A[1][1] + A[0][0] * A[2][2] + A[1][1] * A[2][2]) - (dd + ee + ff); // a*b + a*c + b*c - d^2 - e^2 - f^2
    c0 = A[2][2] * dd + A[0][0] * ee + A[1][1] * ff - A[0][0] * A[1][1] * A[2][2] - 2.0f * A[0][2] * de; // c*d^2 + a*e^2 + b*f^2 - a*b*c - 2*f*d*e
    
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
    diag[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    [unroll]
    for (int i = 0; i < 3;++i)
    {
        diag._11_22_33 = float3(eigenValues[i], eigenValues[i], eigenValues[i]);
        matrix temp = diag - C;
        float C11 = temp[1][1] * temp[2][2] - temp[1][2] * temp[2][1];
        float C12 = temp[1][2] * temp[2][0] - temp[1][0] * temp[2][2];
        float C13 = temp[1][0] * temp[2][1] - temp[1][1] * temp[2][0];
        
        cofactor[i] = float4(float3(C11, C12, C13), 0.0f);
    }
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
    float3 currSmoothPos = g_SmoothPosition[DTid.x];
    float3 omegaTotal = float3(0.0f, 0.0f, 0.0f);
    float wTotal = 0.0f;
    
    
    uint neightborCounter = g_ContactCounts[DTid.x];
    uint i;
    for (i = 0; i < neightborCounter; ++i)
    {
        //get the cell particle pos
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        
        float3 neighborSmoothPos = g_SmoothPosition[neightborParticleIndex];
        float3 deltaPos = currSmoothPos - neighborSmoothPos;
        
        float W_ij = Wsmooth(deltaPos, g_sphSmoothLength);
        wTotal += W_ij;
        omegaTotal += W_ij * neighborSmoothPos;
    }
    
    if (wTotal == 0 || neightborCounter == 0)
    {
        
        float3x3 unitMatrix;
        unitMatrix._11_12_13 = float3(1.0f, 0.0f, 0.0f);
        unitMatrix._21_22_23 = float3(0.0f, 1.0f, 0.0f);
        unitMatrix._31_32_33 = float3(0.0f, 0.0f, 1.0f);
        
        Anisotropy ani;
        ani.q1 = float4(unitMatrix._11_12_13, g_AnisotropyMin);
        ani.q2 = float4(unitMatrix._21_22_23, g_AnisotropyMin);
        ani.q3 = float4(unitMatrix._31_32_33, g_AnisotropyMin);

        g_Anisortopy[prevIndex] = ani;
        
        return;
    }
    
    matrix symmetric;
    symmetric._11_21_31_41 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._12_22_32_42 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._13_23_33_43 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    symmetric._14_24_34_44 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        
    matrix covariance;
    covariance[3] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (i = 0; i < neightborCounter; ++i)
    {
        uint neightborParticleIndex = g_Contacts[DTid.x * g_MaxNeighborPerParticle + i];
        
        float3 neighborSmoothPos = g_SmoothPosition[neightborParticleIndex];
        
        float3 deltaRadius = currSmoothPos - neighborSmoothPos;
        
        float w_ij = Wsmooth(deltaRadius, g_sphSmoothLength);
        
        float3 deltaPos = neighborSmoothPos - omegaTotal / wTotal;
        
        symmetric[0] = float4(deltaPos.x * deltaPos, 0.0f);
        symmetric[1] = float4(deltaPos.y * deltaPos, 0.0f);
        symmetric[2] = float4(deltaPos.z * deltaPos, 0.0f);
            
        covariance += w_ij * symmetric;
    }
        
    matrix C = symmetric / wTotal;
    C[3][3] = 1.0f;
    float detC = determinant(C);
    float3 eigenvalues = dsyevc3(C);
    matrix ratotion;
    ratotion._14_24_34_44 = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    [flatten]
    if (abs(eigenvalues.x - eigenvalues.z) < FLOAT_MIN)
    {
        //rho_1 = rho_3
        ratotion._11_21_31_41 = float4(1.0f, 0.0f, 0.0f,0.0f);
        ratotion._12_22_32_42 = float4(0.0f, 1.0f, 0.0f,0.0f);
        ratotion._13_23_33_43 = float4(0.0f, 0.0f, 1.0f,0.0f);
    }
    else 
    {
        matrix eiegnVectors = GetEigenVectors(C, eigenvalues);
        
        if (abs(eigenvalues.x - eigenvalues.y) > FLOAT_MIN && abs(eigenvalues.y - eigenvalues.z) < FLOAT_MIN)
        {
            float3 v1 = eiegnVectors[0].xyz;
            float3 v2 = float3(v1.y, -v1.x, 0.0f);
            float3 v3 = float3(v1.xy, -(v1.x * v1.x + v1.y * v1.y) / v1.z);
            ratotion._11_21_31_41 = float4(normalize(v1), 0.0f);
            ratotion._12_22_32_42 = float4(normalize(v2), 0.0f);
            ratotion._13_23_33_43 = float4(normalize(v3), 0.0f);
        }
        else
        {
            float3 v1 = eiegnVectors[0].xyz;
            float3 v3 = eiegnVectors[2].xyz;
            v3 = v3 - v1 * dot(v1, v3);
            float3 v2 = cross(v1, v3);
            ratotion._11_21_31_41 = float4(normalize(v1), 0.0f);
            ratotion._12_22_32_42 = float4(normalize(v2), 0.0f);
            ratotion._13_23_33_43 = float4(normalize(v3), 0.0f);
        }
    }
    ratotion[3][3] = 1.0f;
    
    eigenvalues.y = max(eigenvalues.y, eigenvalues.x / 4);
    eigenvalues.z = max(eigenvalues.z, eigenvalues.x / 4);
    
    matrix diag;
    diag[0] = float4(1 / eigenvalues.x, 0.0f, 0.0f, 0.0f);
    diag[1] = float4(0.0f, 1 / eigenvalues.y, 0.0f, 0.0f);
    diag[2] = float4(0.0f, 0.0f, 1 / eigenvalues.z, 0.0f);
    diag[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
            
    matrix anisotroMatrix;      //G matrix
    anisotroMatrix = (1 / g_sphSmoothLength) * ratotion * (1 / 1400.0f) * diag * transpose(ratotion);
    
    Anisotropy ani;
    
    ani.q1 = float4(normalize(anisotroMatrix._11_21_31), clamp(sqrt(anisotroMatrix[0][0]) * g_AnisotropyScale, g_AnisotropyMin, g_AnisotropyMax));
    ani.q2 = float4(normalize(anisotroMatrix._12_22_32), clamp(sqrt(anisotroMatrix[1][1]) * g_AnisotropyScale, g_AnisotropyMin, g_AnisotropyMax));
    ani.q3 = float4(normalize(anisotroMatrix._13_23_33), clamp(sqrt(anisotroMatrix[2][2]) * g_AnisotropyScale, g_AnisotropyMin, g_AnisotropyMax));

    g_Anisortopy[prevIndex] = ani;
}