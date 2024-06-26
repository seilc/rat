#ifndef XMODELBUCKET_H
#define XMODELBUCKET_H

#include "xModelPipe.h"

#include <rpworld.h>

struct xModelInstance;

struct xModelBucket
{
    RpAtomic* Data;
    RpAtomic* OriginalData;
    union
    {
        xModelInstance* List;
        xModelBucket** BackRef;
    };
    S32 ClipFlags;
    xModelPipe Pipe;
};

#endif
