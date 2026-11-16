#ifndef ZCONDITIONAL_H
#define ZCONDITIONAL_H

#include "xBase.h"
#include "zCondAsset.h"

struct zVarEntry;

typedef struct _zConditional : xBase
{
    zCondAsset* asset;
    zVarEntry* varEntry;
} zConditional;

U32 zConditional_Evaluate(zConditional* c);

#endif
