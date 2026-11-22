#ifndef ZHEISTLOOKOUTPOINT_H
#define ZHEISTLOOKOUTPOINT_H

#include "xCam.h"
#include "xCamAsset.h"
#include "zHeistLookoutPointAsset.h"
#include "zInteraction.h"

class zHeistManager;

class zHeistLookoutPoint : public zTriggerInteraction
{
public:
    zHeistLookoutPoint(zHeistLookoutPointAsset* theAsset);

    static void SetHeistManager(zHeistManager* inManager) { heistManager = inManager; }

protected:
    static zHeistManager* heistManager;

    xEnt* lookAtEnt;
    xCamAsset camAsset;
    xCam* prevCamera;
    xCamTransitionParams prevCameraTransitionParams;
    xBase* eventReciever;
    F32 fov;
    F32 curYAngle;
    F32 curXAngle;
    F32 angleMaxX;
    F32 angleMaxY;
    F32 timeInInteraction;
    U32 xInputState;
    U32 yInputState;
};

#endif
