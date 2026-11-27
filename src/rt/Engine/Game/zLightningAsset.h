#ifndef ZLIGHTNINGASSET_H
#define ZLIGHTNINGASSET_H

#include "xColor.h"
#include "xDynAsset.h"
#include "xVec3.h"

struct zLightningAsset : xDynAsset
{
    xVec3 start;
    xVec3 end;
    xColor color;
    F32 thickness;
    F32 branchSpeed;
    U32 mainTexture;
    U32 branchTexture;
    S32 damage;
    F32 knockBackSpeed;
    U32 sound;
    U32 soundHit1;
    U32 soundHit2;
    U32 followStart;
    U32 followEnd;
    U32 collisionEnabled;
};

#endif
