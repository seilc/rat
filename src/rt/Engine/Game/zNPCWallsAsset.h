#ifndef ZNPCWALLSASSET_H
#define ZNPCWALLSASSET_H

#include "xDynAsset.h"

struct zNPCWallsAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 npcWallsGroup;
    U32 flags;
};

#endif
