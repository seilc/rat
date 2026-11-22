#ifndef ZINTERACTIONASSET_H
#define ZINTERACTIONASSET_H

#include "xDynAsset.h"

struct zInteractionAsset : xDynAsset
{
    bool initiallyActive;
    bool initiallyAvailable;
};

struct zTriggerInteractionAsset : zInteractionAsset
{
    U32 triggerID;
};

#endif
