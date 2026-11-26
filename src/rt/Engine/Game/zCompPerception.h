#ifndef ZCOMPPERCEPTION_H
#define ZCOMPPERCEPTION_H

#include "zICompPerception.h"

class zCompPerceptionParams;
class zCompPerceptionDynamics;
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
public:
    zCompPerception();

#if DEBUG
    static void AddStaticTweaks();
#endif

    virtual void Attached();
    virtual void AllAttached();
    virtual void Detached();
#if RELEASE
    virtual void Reset();
    virtual bool IsPerceived() { return perceived; }
    virtual bool IsRemembered() { return remembered; }
#endif
    virtual void Update(xScene*, F32) {}
    virtual void SetParameters(const zCompParameters* parameters) { params = (const zCompPerceptionParams*)parameters; }
    virtual void PreUpdate(xScene*, F32 dt);

private:
    void Perceived(eCompPerceptionType perceptionType, xEnt* perceivedEnt, F32 dt);
    void NotPerceived(F32 dt);
    bool CheckSpherePerception(zCompPerceptionDynamics& percpDyn);
    bool CheckSoundSpherePerception(zCompPerceptionDynamics& percpDyn);
    bool CheckCylinderPerception(zCompPerceptionDynamics& percpDyn);
    bool CheckConePerception(zCompPerceptionDynamics& percpDyn);
    bool CheckTriPerception(zCompPerceptionDynamics& percpDyn);

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
