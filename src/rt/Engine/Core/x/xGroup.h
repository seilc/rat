#ifndef XGROUP_H
#define XGROUP_H

#include "xBase.h"
#include "xGroupAsset.h"

struct xGroup : xBase
{
    xGroupAsset* asset;
    xBase** item;
    U32 last_index;
    U32 ptr_last_index;
    S32 flg_group;
};

U32 xGroupGetCount(xGroup* g);
xBase* xGroupGetItemPtr(xGroup* g, U32 index);

#endif
