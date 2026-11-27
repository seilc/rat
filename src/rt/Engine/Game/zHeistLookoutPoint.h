#ifndef ZHEISTLOOKOUTPOINT_H
#define ZHEISTLOOKOUTPOINT_H

#include "xCam.h"
#include "xCamAsset.h"
#include "zHeistLookoutPointAsset.h"
#include "zInteraction.h"

class zHeistManager;
class zPad;

class zHeistLookoutPoint : public zTriggerInteraction
{
public:
    zHeistLookoutPoint(zHeistLookoutPointAsset* theAsset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    static void UpdateHeistManager(F32 dt);
    static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    void EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32);
    void Update(F32 dt);
    static void RenderOverlays();
    void HandleInput(zPad* currPad, F32 dt);
    virtual void Start(zCommonPlayer* player);
    virtual void Stop();
    void GetLookAtVec(xVec3& inVec);
    void GetLookFromPos(xVec3& inVec);

    static void SetHeistManager(zHeistManager* inManager) { heistManager = inManager; }
    zHeistLookoutPointAsset* GetAsset() { return (zHeistLookoutPointAsset*)asset; }

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
