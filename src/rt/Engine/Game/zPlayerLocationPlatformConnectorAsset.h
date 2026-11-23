#ifndef ZPLAYERLOCATIONPLATFORMCONNECTORASSET_H
#define ZPLAYERLOCATIONPLATFORMCONNECTORASSET_H

#include "xDynAsset.h"

struct zPlayerLocationPlatformConnectorAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 platformId;
    U32 playerId;
};

#endif
