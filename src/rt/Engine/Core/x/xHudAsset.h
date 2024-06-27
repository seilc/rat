#ifndef XHUDASSET_H
#define XHUDASSET_H

#include "xDynAsset.h"
#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "xLinkAsset.h"
#include "xDebug.h"

namespace xhud {

struct asset : xDynAsset
{
    xVec3 loc;
    xVec3 size;
};

struct model_asset : asset
{
    static const U16 VERSION = 1;

    U32 model;

    static const char* type_name() { return "hud:model"; }
};

#define XHUD_CHECK_ASSET(line, asset, T)                                                          \
do {                                                                                              \
    xASSERTFMT((line), (asset).version == T::VERSION,                                             \
               "%s %s - Wrong version: %d (expected %d).  Get latest and repack.",                \
               T::type_name(),                                                                    \
               xSTAssetName(&(asset)),                                                            \
               (asset).version,                                                                   \
               T::VERSION);                                                                       \
    size_t expected_asset_size = (asset).linkCount * sizeof(xLinkAsset) + sizeof(T);              \
    xASSERTFMT((line), (asset_size) == expected_asset_size,                                       \
               "%s %s - Wrong asset size: %d (expected %d).  Get latest and repack.",             \
               T::type_name(),                                                                    \
               xSTAssetName(&(asset)),                                                            \
               asset_size,                                                                        \
               expected_asset_size);                                                              \
    xASSERT((line), (asset).type == xStrHash(T::type_name()));                                    \
} while (0)

}

#endif
