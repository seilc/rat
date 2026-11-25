#ifndef ZBUTTONMASHERWIDGET_H
#define ZBUTTONMASHERWIDGET_H

#include "xBase.h"
#include "zButtonMasherWidgetAsset.h"

class zButtonMasherMashChecker;
class zPlayerInput;

class zButtonMasherController
{
public:
    static void Init();
    static void UpdateAll(F32 dt);
    static S32 AddToObjectCount();
    static void InsertInObjectArray(zButtonMasherMashChecker* inObj, S32 idx);

private:
    static S32 objectCount;
    static zButtonMasherMashChecker** objectArray;
};

class zButtonMasherMashChecker : public xBase
{
    struct PrivateFlags
    {
        bool on : 1;
        bool doCalcRate : 1;
        U32 pad : 31;
    };

protected:
    PrivateFlags privateFlags;
    zButtonMasherWidgetAsset* asset;
    zPlayerInput* playerInput;
    bool cycleDone;
    bool pressed;
    U32 button;
    F32 timer;
    F32 curRate;
    F32 curVelocity;
    S32 objectArrayIdx;

public:
    zButtonMasherMashChecker(zButtonMasherWidgetAsset* theAsset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    void EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32);
    static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual bool CycleDone();
    virtual void Update(F32 dt);
    virtual void ResetButton();
};

#endif
