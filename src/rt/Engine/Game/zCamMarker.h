#ifndef ZCAMMARKER_H
#define ZCAMMARKER_H

#include "xBase.h"

struct xCamAsset;
struct xCamTransitionParams;
struct xEnt;

struct zCamBinaryPOI : xBase
{
    bool Activate(const xCamTransitionParams* transitionParams, bool forceRestart);
};

void zCamMarkerActivateCamera(const xCamAsset& a, xEnt* ent);

#endif
