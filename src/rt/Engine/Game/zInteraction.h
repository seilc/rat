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
    inline virtual void Start(zCommonPlayer* player);
    virtual void Stop();
    inline virtual void Ready();
};

class zTriggerInteraction : public zInteraction
{
public:
    zTriggerInteraction(zTriggerInteractionAsset* theAsset);

protected:
    F32 logoAlpha;
    zEntTrigger* trigger;
};

#endif
