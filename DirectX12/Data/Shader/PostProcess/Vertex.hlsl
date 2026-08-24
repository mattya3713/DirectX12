#include "Header.hlsli"

// フルスクリーントライアングル(実体はHeader.hlsliのFullscreenVS).
VsOutput VS(uint VertexId : SV_VertexID)
{
    return FullscreenVS(VertexId);
}
