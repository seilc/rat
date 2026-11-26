#ifndef ZCOMPSTEERINGBASIC_H
#define ZCOMPSTEERINGBASIC_H

#include "xColor.h"
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
    zCompSteeringBasicParams() { SetDefaults(); }

#if RELEASE
    void AddTweaks(const char* parentName, const char* branchName); // UNUSED
#elif DEBUG
    void AddTweaks(const char* name);
#endif

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

    virtual void SetDefaults()
    {
        maxForce = 20.0f;
        maxSpeed = 8.0f;
        turnSpringK = 10.0f;
        mass = 1.0f;
        wallAvoidenceMultiplier = 1.0f;
        wallAvoidenceForceMax = 40.0f;
        wallFeelerLength = 2.0f;
        sphereAvoidenceMultiplier = 10.0f;
        sphereAvoidenceRange = 2.0f;
        sphereAvoidenceBreakRatio = 0.0f;
        wanderMultiplier = 1.0f;
        wanderJitterPerSec = 20.0f;
        wanderRadius = 2.2f;
        wanderDist = 3.0f;
        seekMultiplier = 1.0f;
        seekRadius = 500.0f;
        fleeMultiplier = 1.0f;
        fleeRadius = 500.0f;
        arriveMultiplier = 2.0f;
        arriveRadius = 500.0f;
        lookPosition = NULL;
        lookDirection = NULL;
        frictionK = 1.0f;
        wallAvoidenceOn = true;
        sphereAvoidenceOn = true;
        wanderOn = true;
        seekOn = true;
        fleeOn = true;
        arriveOn = true;
        gravityOn = false;
        frictionOn = false;
        seekCheckObjectUsage = false;
        fleeCheckObjectUsage = false;
        arriveCheckObjectUsage = false;
        lookAtPosition = false;
        lookAtDirection = false;
    }
};

class zCompSteeringBasic : public zICompSteering
{
public:
    zCompSteeringBasic();

    virtual void Attached();
    virtual void Detached();
#if RELEASE
    virtual void Reset();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual void ApplySteering(zICompNPCEntity* npcEnt, F32 dt);

    virtual void SetAttractor(xEnt* ent) { Attractor = ent; AttractorV = NULL; }
    virtual void SetRepeller(xEnt* ent) { Repeller = ent; RepellerV = NULL; }
    virtual void SetArriver(xEnt* ent) { Arriver = ent; ArriverV = NULL; }
    virtual void SetAttractorV(xVec3* vec) { Attractor = NULL; AttractorV = vec; }
    virtual void SetRepellerV(xVec3* vec) { Repeller = NULL; RepellerV = vec; }
    virtual void SetArriverV(xVec3* vec) { Arriver = NULL; ArriverV = vec; }

    xEnt* GetRepellerEnt() { return Repeller; }
    virtual void SetParameters(const zCompParameters* parameters) { steeringParameters = (const zCompSteeringBasicParams*)parameters; }

private:
    virtual void CalculateForce(xVec3& force, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    virtual void CalculateVelConstraints(xVec3& dVel, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    virtual void CalculatePosConstraints(xVec3& dPos, zSteeringBasicDynamics& steeringDynamics, F32 dt);

protected:
#if DEBUG || RELEASE
    void DebugRenderForce(const xVec3* entPos, xVec3* Force, bool render, F32 multiplier, xColor color);
#endif
    bool AccumulateForce(xVec3& CurTotal, xVec3& ForceToAdd, F32 Scale, bool truncateIfTooBig);
    void GetWanderForce(xVec3& wanderForce, zSteeringBasicDynamics& steeringDynamics, F32 dt);
    void GetWallAvoidenceForce(xVec3& avoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSphereAvoidenceForce(xVec3& avoidenceForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSeekForce(xVec3& seekForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetFleeForce(xVec3& fleeForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetArriveForce(xVec3& arriveForce, zSteeringBasicDynamics& steeringDynamics, F32);
    void GetSweptSphereCollDPos(xVec3& collDPos, zSteeringBasicDynamics& steeringDynamics, F32);

private:
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
