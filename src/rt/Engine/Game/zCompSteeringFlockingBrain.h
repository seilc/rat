#ifndef ZCOMPSTEERINGFLOCKINGBRAIN_H
#define ZCOMPSTEERINGFLOCKINGBRAIN_H

#include "zICompSteering.h"

class zCompSteeringFlockingBrain : public zICompSteering
{
private:
    xVec3 flockingCenter;
    xVec3 flockingAlignment;

public:
    void AllAttached();
    void PreUpdate(xScene*, F32);
    void ApplySteering(zICompNPCEntity*, F32) {}
    void SetAttractor(xEnt*) {}
    void SetRepeller(xEnt*) {}
    void SetArriver(xEnt*) {}
    void SetAttractorV(xVec3*) {}
    void SetRepellerV(xVec3*) {}
    void SetArriverV(xVec3*) {}
    void Attached() {}
    void Detached() {}
#ifdef RELEASE
    void Reset() {}
#endif
    void Update(xScene*, F32) {}
};

#endif
