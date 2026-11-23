#ifndef ZCARRYABLEPROPERTY_H
#define ZCARRYABLEPROPERTY_H

#include "xDynAsset.h"

struct zCarryableProperty : xDynAsset
{
    U32 shrapnelID;
    U32 animCode;
    struct
    {
        U8 destroyDrop;
        U8 pad1;
        U8 pad2;
        U8 pad3;
    } flags;
};

#endif
