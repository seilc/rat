#ifndef ZANIMLISTASSET_H
#define ZANIMLISTASSET_H

#include "types.h"

struct zAnimListAsset
{
    U32 ids[10];
    U32 stateHashes[10];
    bool hasPhysics[10];
};

#endif
