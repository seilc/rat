#ifndef ZCAMMARKER_H
#define ZCAMMARKER_H

#include "xBase.h"

struct xCamTransitionParams;

struct zCamBinaryPOI : xBase
{
    bool Activate(const xCamTransitionParams* transitionParams, bool forceRestart);
};

#endif
