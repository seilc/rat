#ifndef ZCONDASSET_H
#define ZCONDASSET_H

#include "xBase.h"

struct zCondAsset : xBaseAsset
{
    U32 constNum;
    U32 expr1;
    U32 op;
    U32 value_asset;
};

#endif
