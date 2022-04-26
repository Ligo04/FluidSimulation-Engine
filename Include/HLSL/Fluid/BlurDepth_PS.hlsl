#include "FliudCommon.hlsli"


float4 PS(PassThoughVertexOut ptIn) : SV_Target
{
    float4 position = ptIn.PosH;
    float2 texCoord[1] = { ptIn.TexCoord };
    
    //eye-space depth of center sample
    float depth = g_DepthTexture.Load(int3(position.xy, 0)).x;
    
    float blurDepthFalloff = 5.5f;
    float maxBlurRadius = 5.0f;
    
    //discontinuities between different tap counts are visible. to avoid this we 
	//use fractional contributions between #taps = ceil(radius) and floor(radius) 
    float radius = min(maxBlurRadius, g_BlurScale * (g_BlurRadiusWorld / -depth));
    float radiusInv = 1.0f / radius;
    float taps = ceil(radius);
    float frac = taps - radius;
    
    float sum = 0.0f;
    float wsum = 0.0f;
    float count = 0.0f;
    
    for (float y = -taps; y <= taps; y += 1.0f)
    {
        for (float x = -taps; x <= taps; x += 1.0f)
        {
            float2 offset = float2(x, y);
            float sample = g_DepthTexture.Load(int3(position.xy + offset, 0)).x;

            //spatial domain
            float r1 = length(float2(x, y)) * radiusInv;
            float w = exp(-(r1 * r1));

            //range domain (based on depth difference)
            float r2 = (sample - depth) * blurDepthFalloff;
            float g = exp(-(r2 * r2));
            
            //fractional radius contribution
            float wBoundaty = step(radius, max(abs(x), abs(y)));
            float wFrac = 1.0 - wBoundaty * frac;
            
            sum += sample * w * g * wFrac;
            wsum += w * g * wFrac;
            count += g * wFrac;

        }
    }

    if (wsum > 0.0f)
    {
        sum /= wsum;
    }
    
    float blend = count / sqr(2.0f * radius + 1.0f);
    float res = lerp(depth, sum, blend);
    return float4(res, 0.0f, 0.0f, 1.0f);
}
