#ifndef ZCOMPPERCEPTION_H
#define ZCOMPPERCEPTION_H

#include "zICompPerception.h"

class zCompPerceptionParams;
struct xEnt;

enum eCompPerceptionType
{
    eCompPerceptionType_None,
    eCompPerceptionType_Sphere,
    eCompPerceptionType_SoundSphere,
    eCompPerceptionType_Cylinder,
    eCompPerceptionType_Cone,
    eCompPerceptionType_Tri
};

class zCompPerception : public zICompPerception
{
    const zCompPerceptionParams* params;
    xEnt* perceivedEntity;
    eCompPerceptionType perceptionType;
    xVec3 perceivedLocation;
    F32 updateTimer;
    F32 perceivedTimer;
    F32 lostTimer;
    F32 tanConeHTheta;
    F32 tanTriHTheta;
    F32 tanLosHTheta;
    bool perceived;
    bool remembered;
};

#endif
