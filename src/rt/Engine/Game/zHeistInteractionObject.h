#ifndef ZHEISTINTERACTIONOBJECT_H
#define ZHEISTINTERACTIONOBJECT_H

#include "zHeistDistractionObjectAsset.h"
#include "zInteraction.h"

struct xMovePoint;
class zHeistDistractionObject;

class zHeistDistractionObjectManager
{
protected:
    zHeistDistractionObject** objectArray;
    S32 objectCount;
};

class zHeistDistractionObject : public zTriggerInteraction
{
public:
    zHeistDistractionObject(zHeistDistractionObjectAsset* theAsset);

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
};

class zHeistInteractionObjectManager
{
public:
    void Init();
    void Exit();
    void Update(F32 dt);
    void RenderOverlays();

protected:
    zHeistDistractionObjectManager* heistDistractionObjectManager;
    zHeistObjectiveManager* heistObjectiveManager;
};

#endif
