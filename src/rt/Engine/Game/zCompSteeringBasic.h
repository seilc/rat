#ifndef ZCOMPSTEERINGBASIC_H
#define ZCOMPSTEERINGBASIC_H

#include "zCompParameters.h"
#include "zICompSteering.h"

class zNPCWalls;

class zSteeringBasicDynamics
{
public:
    void Initialize(zICompNPCEntity* npcEnt);

    const xVec3* oldPos;
    const xVec3* pos;
    const xVec3* oldVel;
    const xVec3* vel;
    const xVec3* heading;
    F32 boundRadius;
};

class zCompSteeringBasicParams : public zCompParameters
{
public:
    F32 maxForce;
    F32 maxSpeed;
    F32 turnSpringK;
    F32 mass;
    F32 wallAvoidenceMultiplier;
    F32 wallAvoidenceForceMax;
    F32 wallFeelerLength;
    F32 sphereAvoidenceMultiplier;
    F32 sphereAvoidenceRange;
    F32 sphereAvoidenceBreakRatio;
    F32 wanderMultiplier;
    F32 wanderJitterPerSec;
    F32 wanderRadius;
    F32 wanderDist;
    F32 seekMultiplier;
    F32 seekRadius;
    F32 fleeMultiplier;
    F32 fleeRadius;
    F32 arriveMultiplier;
    F32 arriveRadius;
    xVec3* lookPosition;
    xVec3* lookDirection;
    F32 frictionK;
    bool wallAvoidenceOn;
    bool sphereAvoidenceOn;
    bool wanderOn;
    bool seekOn;
    bool fleeOn;
    bool arriveOn;
    bool gravityOn;
    bool frictionOn;
    bool seekCheckObjectUsage;
    bool fleeCheckObjectUsage;
    bool arriveCheckObjectUsage;
    bool lookAtPosition;
    bool lookAtDirection;

    virtual void SetDefaults();
};

class zCompSteeringBasic : public zICompSteering
{
public:
    zCompSteeringBasic();

    virtual void Attached();
    virtual void Detached();
    virtual void Update(xScene* xscn, F32 dt);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual void ApplySteering(zICompNPCEntity* npcEnt, F32 dt);

private:
    void CalculateForce(xVec3& force, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    void CalculateVelConstraints(xVec3& dVel, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    void CalculatePosConstraints(xVec3& dPos, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    bool AccumulateForce(xVec3& CurTotal, xVec3& ForceToAdd, F32 Scale, bool truncateIfTooBig);
    void GetWanderForce(xVec3& wanderForce, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    void GetWallAvoidenceForce(xVec3& avoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSphereAvoidenceForce(xVec3& avoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSeekForce(xVec3& seekForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetFleeForce(xVec3& fleeForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetArriveForce(xVec3& arriveForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSweptSphereCollDPos(xVec3& collDPos, zSteeringBasicDynamics& steeringDynamics, F32);

    const zCompSteeringBasicParams* steeringParameters;
    xVec3 m_vWanderTarget;
    xEnt* Attractor;
    xVec3* AttractorV;
    xEnt* Repeller;
    xVec3* RepellerV;
    xEnt* Arriver;
    xVec3* ArriverV;
    zNPCWalls* npcWalls;
};

#endif
