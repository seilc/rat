#include "zWireFrameAsset.h"

void WireframeAssetSetupPointers(zWireframeAsset* wireframe)
{
    wireframe->vertices = (zWireframeVertex*)((U8*)wireframe + sizeof(zWireframeAsset));
    wireframe->lines = (zWireframeLine*)((U8*)wireframe + sizeof(zWireframeAsset) + wireframe->vertexCount * sizeof(zWireframeVertex));
}
