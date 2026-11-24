#ifndef ZCOMPSTEERINGFLOCKING_H
#define ZCOMPSTEERINGFLOCKING_H

#include "zCompSteeringBasic.h"

class zCompSteeringFlockingBrain;

class zCompSteeringFlockingParams : public zCompSteeringBasicParams
{
public:
    F32 seperationMultiplier;
    F32 seperationNeighbourRadius;
    F32 playerAvoidenceMultiplier;
    F32 playerAvoidenceRadius;
    F32 playerAvoidenceForceLimit;
    F32 alignmentMultiplier;
    F32 cohesionMultiplier;
};

class zCompSteeringFlocking : public zCompSteeringBasic
{
public:
    zCompSteeringFlocking();

    virtual void Attached();
    virtual void AllAttached();
    virtual void Detached();
#if RELEASE
    virtual void Reset();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    inline xVec3* GetFlockingCenter();
    inline xVec3* GetFlockingAlignment();

private:
    void CalculateForce(xVec3& force, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    void GetSeperationForce(xVec3& SeperationForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetPlayerAvoidenceForce(xVec3& PlayerAvoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetAlignmentForce(xVec3& AlignmentForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetCohesionForce(xVec3& CohesionForce, zSteeringBasicDynamics& steeringDynamics, F32);

    const zCompSteeringFlockingParams* flockingParameters;
    zCompSteeringFlockingBrain* flockingBrain;
};

#endif
