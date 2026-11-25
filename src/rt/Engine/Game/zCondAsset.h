#ifndef ZCONDASSET_H
#define ZCONDASSET_H

#include "xBase.h"

// zCondAsset::op
#define k_ZCONDASSET_OP_EQ 0
#define k_ZCONDASSET_OP_GT 1
#define k_ZCONDASSET_OP_LT 2
#define k_ZCONDASSET_OP_GE 3
#define k_ZCONDASSET_OP_LE 4
#define k_ZCONDASSET_OP_NE 5

struct zCondAsset : xBaseAsset
{
    U32 constNum;
    U32 expr1;
    U32 op;
    U32 value_asset;
};

#endif
