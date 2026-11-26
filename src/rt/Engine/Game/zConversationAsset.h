#ifndef ZCONVERSATIONASSET_H
#define ZCONVERSATIONASSET_H

#include "xDynAsset.h"

struct zConversationAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 soundGroupID;
};

#endif
