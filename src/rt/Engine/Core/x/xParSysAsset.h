#ifndef XPARSYSASSET_H
#define XPARSYSASSET_H

#include "xBaseAsset.h"

struct xParSysAsset : xBaseAsset
{
    U32 type;
    U32 parentParSysID;
    U32 textureID;
    U8 pad;
    U8 priority;
    U16 maxPar;
    U8 renderFunc;
    U8 renderSrcBlendMode;
    U8 renderDstBlendMode;
    U8 cmdCount;
    U32 cmdSize;
    U32 parFlags;
};

#endif
