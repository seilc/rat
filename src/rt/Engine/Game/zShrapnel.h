#ifndef ZSHRAPNEL_H
#define ZSHRAPNEL_H

#include "xSpacePartitionTree.h"

struct xModelInstance;
struct xVec3;
struct zFrag;
struct zFragAsset;
struct zShrapnelAsset;

typedef void (*zShrapnelFragCallback)(zFrag*, zFragAsset*);
typedef void (*zShrapnelInitCallback)(zShrapnelAsset*, xModelInstance*, xVec3*, xVec3*, zShrapnelFragCallback);

struct zShrapnelAsset
{
    S32 fassetCount;
    U32 shrapnelID;
    zShrapnelInitCallback initCB;
};

#endif
