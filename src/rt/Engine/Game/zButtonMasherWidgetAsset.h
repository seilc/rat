#ifndef ZBUTTONMASHERWIDGETASSET_H
#define ZBUTTONMASHERWIDGETASSET_H

#include "xDynAsset.h"

struct zButtonMasherWidgetAsset : xDynAsset
{
    enum { VERSION = 1 };

    U32 playerId;
    U32 buttonId;
    F32 lowRateThreshold;
    F32 medRateThreshold;
    F32 highRateThreshold;
    F32 decayTime;
    F32 decayRate;
};

#endif
