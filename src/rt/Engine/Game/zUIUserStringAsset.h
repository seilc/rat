#ifndef ZUIUSERSTRINGASSET_H
#define ZUIUSERSTRINGASSET_H

#include "zUITextAsset.h"

struct zUIUserStringAsset : zUITextAsset
{
    U8 hardMaxChars;
    U8 softMaxChars;
};

#endif
