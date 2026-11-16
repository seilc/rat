#ifndef ZCOMPPERCEPTIONPARAMS_H
#define ZCOMPPERCEPTIONPARAMS_H

#include "xDynAsset.h"
#include "zCompParameters.h"

class zCompPerceptionParams : public xDynAsset, public zCompParameters
{
public:
    U32 perceivedEntity;
    U32 setToTemplate;
    F32 detectedUpdateFrequency;
    F32 lostUpdateFrequency;
    F32 perceptionMemoryTime;
    U32 flags;
    F32 sphereR;
    F32 sphereRatio;
    F32 soundSphereR;
    F32 soundSphereRatio;
    F32 soundSphereMinNoise;
    F32 cylinderR;
    F32 cylinderHUp;
    F32 cylinderHDown;
    F32 cylinderRatio;
    F32 coneTheta;
    F32 coneD;
    F32 coneRatio;
    F32 triTheta;
    F32 triD;
    F32 triHUp;
    F32 triHDown;
    F32 triRatio;
    F32 losConeTheta;
    U32 detectionAction;
    U32 lostAction;

    static const zCompPerceptionParams* spNPCGenericDefault;
    static const zCompPerceptionParams* spNPCInstaKillDefault;
    static zCompPerceptionParams sNPCGenericDefault;
    static zCompPerceptionParams sNPCInstaKillDefault;

    zCompPerceptionParams();
    static void InitializeDefaults();
    static zCompPerceptionParams* GetParamsFromAsset(U32 aid);
};

#endif
