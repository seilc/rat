#ifndef ZEGENASSET_H
#define ZEGENASSET_H

#include "xEntAsset.h"
#include "xVec3.h"

struct zEGenAsset : xEntAsset
{
    xVec3 src_dpos;
    U8 damage_type;
    U8 flags;
    F32 ontime;
    U32 onAnimID;
};

#endif
