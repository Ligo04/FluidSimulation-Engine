#include "FliudCommon.hlsli"

PassThoughVertexOut VS(MeshVertexIn MvIn)
{
    PassThoughVertexOut ptVout;

    ptVout.PosH = float4(MvIn.PosW, 1.0f);
    ptVout.TexCoord = MvIn.TexCoord;
    return ptVout;
}