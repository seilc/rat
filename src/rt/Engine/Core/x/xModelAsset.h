#ifndef XMODELASSET_H
#define XMODELASSET_H

#include "types.h"

struct xModelAssetInfo
{
    U32 Magic;
    U32 NumModelInst;
    U32 AnimTableID;
    U32 CombatID;
    U32 BrainID;
};

struct xModelAssetParam
{
    U32 HashID;
    U8 WordLength;
    U8 String[3];
};

#endif
