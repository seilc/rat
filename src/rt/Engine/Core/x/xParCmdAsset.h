#ifndef XPARCMDASSET_H
#define XPARCMDASSET_H

#include "types.h"

struct xParCmdAsset
{
    U32 type;
    bool enabled;
    U8 mode;
    U8 pad[2];
};

#endif
