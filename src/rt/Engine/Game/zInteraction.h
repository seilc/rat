#ifndef ZINTERACTION_H
#define ZINTERACTION_H

#include "xBase.h"
#include "zInteractionAsset.h"

struct xEnt;
struct zCommonPlayer;
struct zEntTrigger;

enum zInteractionType
{
    eInteractionTypeSwitchLever,
    eInteractionTypeLaunch,
    eInteractionTypeLookoutPoint,
    eInteractionTypeDistractionPoint,
    eInteractionTypeCount
};

class zInteraction : public xBase
{
protected:
    zCommonPlayer* interactingPlayer;
    zInteractionAsset* asset;
    zInteractionType type;
    xEnt* object;
    bool activated;
    bool available;

public:
    virtual void Start(zCommonPlayer* player) { interactingPlayer = player; }
    virtual void Stop();
    virtual void Ready() {}
};

class zTriggerInteraction : public zInteraction
{
public:
    zTriggerInteraction(zTriggerInteractionAsset* theAsset);
    void Reset();
    bool StartInteracting(zCommonPlayer* player);

protected:
    F32 logoAlpha;
    zEntTrigger* trigger;
};

#endif
