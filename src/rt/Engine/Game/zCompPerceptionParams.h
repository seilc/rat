#ifndef ZCOMPPERCEPTIONPARAMS_H
#define ZCOMPPERCEPTIONPARAMS_H

#include "xDynAsset.h"
#include "zCompParameters.h"

// zCompPerceptionParams::flags
#define k_ZCOMPPERCEPTIONPARAMS_SPHERE 0x1
#define k_ZCOMPPERCEPTIONPARAMS_SOUND_SPHERE 0x2
#define k_ZCOMPPERCEPTIONPARAMS_CYLINDER 0x4
#define k_ZCOMPPERCEPTIONPARAMS_CONE 0x8
#define k_ZCOMPPERCEPTIONPARAMS_TRI 0x10

#if RELEASE
class zCompPerceptionParams : public zCompParameters, public xDynAsset
#else
class zCompPerceptionParams : public xDynAsset, public zCompParameters
#endif
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

#if !RELEASE
    static const zCompPerceptionParams* spNPCGenericDefault;
    static const zCompPerceptionParams* spNPCInstaKillDefault;
    static zCompPerceptionParams sNPCGenericDefault;
    static zCompPerceptionParams sNPCInstaKillDefault;
#endif

    zCompPerceptionParams();
#if !RELEASE
    static void InitializeDefaults();
#endif
    static zCompPerceptionParams* GetParamsFromAsset(U32 aid);
};

#endif
