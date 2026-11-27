#ifndef ZHEISTLOOKOUTPOINTASSET_H
#define ZHEISTLOOKOUTPOINTASSET_H

#include "zInteractionAsset.h"

struct zHeistLookoutPointAsset : zTriggerInteractionAsset
{
    enum { VERSION = 3 };

    U32 lookAtId;
    F32 transitionTime;
    F32 fov;
    F32 angleMaxX;
    F32 angleMaxY;
    U32 eventRecieverId;
};

#endif
