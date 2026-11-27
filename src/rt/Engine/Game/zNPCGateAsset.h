#ifndef ZNPCGATEASSET_H
#define ZNPCGATEASSET_H

#include "xDynAsset.h"

struct zNPCGateAsset : xDynAsset
{
    enum { VERSION = 3 };

    U32 gateTrigger;
    U32 gateFacing;
    U32 npcsGroup;
    U32 flags;
};

#endif
