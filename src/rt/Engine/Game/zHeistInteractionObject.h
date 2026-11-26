#ifndef ZHEISTINTERACTIONOBJECT_H
#define ZHEISTINTERACTIONOBJECT_H

#include "zHeistDistractionObjectAsset.h"
#include "zInteraction.h"

struct xMovePoint;
class zHeistDistractionObject;

class zHeistDistractionObjectManager
{
public:
    void Init();
    void Exit();
    void Reset(); // UNUSED
    void RenderOverlays();
    void Update(F32);
    void RegisterObject(zHeistDistractionObject* regObj);
    S32 AddToObjectCount();
    void InsertObject(zHeistDistractionObject* insObj, S32 idx);

protected:
    zHeistDistractionObject** objectArray;
    S32 objectCount;
};

class zHeistDistractionObject : public zTriggerInteraction
{
public:
    zHeistDistractionObject(zHeistDistractionObjectAsset* theAsset);

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);
    static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    void EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32);
    void SceneEnter();
    void Update(F32 dt);
    virtual void Start(zCommonPlayer* player);
    virtual void Stop();

    static void SetDistractionObjectManager(zHeistDistractionObjectManager* inMgr) { heistDistractionObjectManager = inMgr; }
    S32 GetObjectArrayIdx() { return objectArrayIdx; }

protected:
    static zHeistDistractionObjectManager* heistDistractionObjectManager;

    xEnt* distractEnt;
    xMovePoint* distractPoint;
    F32 distractTime;
    F32 distractDelay;
    F32 timer;
    S32 objectArrayIdx;
};

class zHeistObjectiveManager
{
public:
    void Init();
    void Exit();
    void Reset(); // UNUSED
    void RenderOverlays();
    void Update(F32);
};

class zHeistInteractionObjectManager
{
public:
    void Init();
    void Exit();
    void Reset(); // UNUSED
    void RenderOverlays();
    void Update(F32 dt);

protected:
    zHeistDistractionObjectManager* heistDistractionObjectManager;
    zHeistObjectiveManager* heistObjectiveManager;
};

#endif
