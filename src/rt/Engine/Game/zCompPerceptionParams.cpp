#include "xCommon.h"
#include "zCompPerceptionParams.h"

zCompPerceptionParams zCompPerceptionParams::sNPCGenericDefault;
zCompPerceptionParams zCompPerceptionParams::sNPCInstaKillDefault;
const zCompPerceptionParams* zCompPerceptionParams::spNPCGenericDefault = &sNPCGenericDefault;

void zCompPerceptionParams::InitializeDefaults()
{
    sNPCInstaKillDefault.detectedUpdateFrequency = 0.0f;
    sNPCInstaKillDefault.lostUpdateFrequency = 0.0f;
}

zCompPerceptionParams::zCompPerceptionParams()
{
    perceivedEntity = 0;
    setToTemplate = 0;
    detectedUpdateFrequency = 0.5f;
    lostUpdateFrequency = 0.5f;
    perceptionMemoryTime = 5.0f;
    flags = 0;
    sphereR = 1.0f;
    sphereRatio = 1.25f;
    soundSphereR = 1.0f;
    soundSphereRatio = 1.25f;
    soundSphereMinNoise = 0.5f;
    cylinderR = 1.0f;
    cylinderHUp = 1.0f;
    cylinderHDown = 1.0f;
    cylinderRatio = 1.25f;
    coneTheta = 30.0f;
    coneD = 1.0f;
    coneRatio = 1.25f;
    triTheta = 30.0f;
    triD = 1.0f;
    triHUp = 1.0f;
    triHDown = 1.0f;
    triRatio = 1.25f;
    losConeTheta = 30.0f;
    detectionAction = 0;
    lostAction = 0;
}

zCompPerceptionParams* zCompPerceptionParams::GetParamsFromAsset(U32 aid)
{
    U32 assetSize;
    zCompPerceptionParams* params = (zCompPerceptionParams*)xSTFindAsset(aid, &assetSize);
    xASSERT_L(90, assetSize == sizeof(zCompPerceptionParams));
    return params;
}
