#ifndef ZSHRAPNEL_H
#define ZSHRAPNEL_H

#include "types.h"

struct xModelInstance;
struct xVec3;
struct zFrag;
struct zFragAsset;

struct zShrapnelAsset
{
    S32 fassetCount;
    U32 shrapnelID;
    void (*initCB)(zShrapnelAsset*, xModelInstance*, xVec3*, xVec3*, void (*)(zFrag*, zFragAsset*));
};

#endif
