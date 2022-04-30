#include "FliudCommon.hlsli"

[maxvertexcount(4)]
void GS(point FluidVertexOut input[1], inout TriangleStream<FluidGeoOut> triStream)
{
    const float4 ndcPos = input[0].ndcPos;
	// frustrum culling
    const float ndcBound = 1.0;
    if (any(abs(ndcPos.xy) > ndcBound))
    {
        return;
    }

    float4 bounds = input[0].bounds;

    const float4 invQuad0 = input[0].invQ0;
    const float4 invQuad1 = input[0].invQ1;
    const float4 invQuad2 = input[0].invQ2;
    const float4 invQuad3 = input[0].invQ3;

    float xmin = bounds.x;
    float xmax = bounds.y;
    float ymin = bounds.z;
    float ymax = bounds.w;

    FluidGeoOut output;

    output.Position = float4(xmin, ymax, 0.5, 1.0);
    output.invQ0 = invQuad0;
    output.invQ1 = invQuad1;
    output.invQ2 = invQuad2;
    output.invQ3 = invQuad3;
    triStream.Append(output);

    output.Position = float4(xmin, ymin, 0.5, 1.0);
    output.invQ0 = invQuad0;
    output.invQ1 = invQuad1;
    output.invQ2 = invQuad2;
    output.invQ3 = invQuad3;
    triStream.Append(output);

    output.Position = float4(xmax, ymax, 0.5, 1.0);
    output.invQ0 = invQuad0;
    output.invQ1 = invQuad1;
    output.invQ2 = invQuad2;
    output.invQ3 = invQuad3;
    triStream.Append(output);

    output.Position = float4(xmax, ymin, 0.5, 1.0);
    output.invQ0 = invQuad0;
    output.invQ1 = invQuad1;
    output.invQ2 = invQuad2;
    output.invQ3 = invQuad3;
    triStream.Append(output);
}