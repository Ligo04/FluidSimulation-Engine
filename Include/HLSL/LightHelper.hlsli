
// 方向光
struct DirectionalLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Direction;
    float Pad;
};

// 点光
struct PointLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float3 Att;
    float Pad;
};

// 聚光灯
struct SpotLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float3 Direction;
    float Spot;

    float3 Att;
    float Pad;
};

// 物体表面材质
struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular; // w = SpecPower
    float4 Reflect;
};



void ComputeDirectionalLight(Material mat, DirectionalLight L,
	float3 normal, float3 toEye,
	out float4 ambient,
	out float4 diffuse,
	out float4 spec)
{
	// 初始化输出
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// 光向量与照射方向相反
    float3 lightVec = -L.Direction;

	// 添加环境光
    ambient = mat.Ambient * L.Ambient;

	// 添加漫反射光和镜面光
    float diffuseFactor = dot(lightVec, normal);

	// 展开，避免动态分支
	[flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;
        spec = specFactor * mat.Specular * L.Specular;
    }
}


void ComputePointLight(Material mat, PointLight L, float3 pos, float3 normal, float3 toEye,
	out float4 ambient, out float4 diffuse, out float4 spec)
{
	// 初始化输出
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// 从表面到光源的向量
    float3 lightVec = L.Position - pos;

	// 表面到光线的距离
    float d = length(lightVec);

	// 灯光范围测试
    if (d > L.Range)
        return;

	// 标准化光向量
    lightVec /= d;

	// 环境光计算
    ambient = mat.Ambient * L.Ambient;

	// 漫反射和镜面计算
    float diffuseFactor = dot(lightVec, normal);

	// 展开以避免动态分支
	[flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;
        spec = specFactor * mat.Specular * L.Specular;
    }

	// 光的衰弱
    float att = 1.0f / dot(L.Att, float3(1.0f, d, d * d));

    diffuse *= att;
    spec *= att;
}


void ComputeSpotLight(Material mat, SpotLight L, float3 pos, float3 normal, float3 toEye,
	out float4 ambient, out float4 diffuse, out float4 spec)
{
	// 初始化输出
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// // 从表面到光源的向量
    float3 lightVec = L.Position - pos;

    // 表面到光源的距离
    float d = length(lightVec);

	// 范围测试
    if (d > L.Range)
        return;

	// 标准化光向量
    lightVec /= d;

	// 计算环境光部分
    ambient = mat.Ambient * L.Ambient;


    // 计算漫反射光和镜面反射光部分
    float diffuseFactor = dot(lightVec, normal);

	// 展开以避免动态分支
	[flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;
        spec = specFactor * mat.Specular * L.Specular;
    }

	// 计算汇聚因子和衰弱系数
    float spot = pow(max(dot(-lightVec, L.Direction), 0.0f), L.Spot);
    float att = spot / dot(L.Att, float3(1.0f, d, d * d));

    ambient *= spot;
    diffuse *= att;
    spec *= att;
}

float3 NormalSampleToWorldSpace(float3 normalMapSample,
    float3 unitNormalW,
    float4 tangentW)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    // 构建位于世界坐标系的切线空间
    float3 N = unitNormalW;
    float3 T = normalize(tangentW.xyz - dot(tangentW.xyz, N) * N); // 施密特正交化
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    // 将凹凸法向量从切线空间变换到世界坐标系
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

static const float SMAP_SIZE = 2048.0f;
static const float SMAP_DX = 1.0f / SMAP_SIZE;

float CalcShadowFactor(SamplerComparisonState samShadow,
                       Texture2D shadowMap,
					   float4 shadowPosH)
{
	// 透视除法
    shadowPosH.xyz /= shadowPosH.w;
	
	// NDC空间的深度值
    float depth = shadowPosH.z;

	// 纹素在纹理坐标下的宽高
    const float dx = SMAP_DX;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    // samShadow为compareValue <= sampleValue时为1.0f(反之为0.0f), 对相邻四个纹素进行采样比较
    // 并根据采样点位置进行双线性插值
    // float result0 = depth <= s0;  // .s0      .s1          
    // float result1 = depth <= s1;
    // float result2 = depth <= s2;  //     .depth
    // float result3 = depth <= s3;  // .s2      .s3
    // float result = BilinearLerp(result0, result1, result2, result3, a, b);  // a b为算出的插值相对位置                           
	[unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(samShadow,
			shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit /= 9.0f;
}


