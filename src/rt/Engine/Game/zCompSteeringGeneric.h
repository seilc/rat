#ifndef ZCOMPSTEERINGGENERIC_H
#define ZCOMPSTEERINGGENERIC_H

#include "xColor.h"
#include "zCompParameters.h"
#include "zICompSteering.h"
#if RELEASE
#include "zSweptCircle.h"
#endif

class zNPCWalls;

enum eSteeringType
{
    eSteeringType_Straight,
#if RELEASE
    eSteeringType_Turn,
#else
    eSteeringTyoe_Turn,
#endif
    eSteeringType_Stop,
    eSteeringType_Wander,
    eSteeringType_Seek,
    eSteeringType_Flee,
    eSteeringType_Arrive,
    eSteeringType_Pursuit,
    eSteeringType_Evade,
#if RELEASE
    eSteeringType_Jump
#endif
};

class zSteeringGenericDynamics
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

class zCompSteeringGenericParams : public zCompParameters
{
public:
    zCompSteeringGenericParams() { SetDefaults(); }

#if RELEASE
    void AddTweaks(const char* parentName, const char* branchName);
#elif DEBUG
    void AddTweaks(const char* name); // UNUSED
#endif

    F32 maxAcc;
    F32 maxSpeed;
    F32 turnSpringK;
#if RELEASE
    F32 gravity;
    F32 avoidenceTime;
    F32 wallAvoidenceRatio;
    F32 sphereAvoidenceRatio;
    F32 wanderRatio;
    F32 wanderFOV;
    F32 wanderJitterPerSec;
    F32 frictionK;
#else
    F32 wallAvoidenceRatio;
    F32 wallFeelerLength;
    F32 sphereAvoidenceRatio;
    F32 sphereAvoidenceRange;
    F32 sphereAvoidenceBreakRatio;
    F32 wanderRatio;
    F32 wanderFOV;
    F32 wanderJitterPerSec;
#endif
    eSteeringType steeringType;
    bool wallAvoidenceOn;
    bool sphereAvoidenceOn;
    bool wanderOn;
#if RELEASE
    bool frictionOn;
#endif
    bool gravityOn;

#if RELEASE
    virtual void SetDefaults()
    {
        maxAcc = 10.0f;
        maxSpeed = 8.0f;
        turnSpringK = 10.0f;
        gravity = 9.8f;
        avoidenceTime = 1.0f;
        wallAvoidenceRatio = 1.0f;
        sphereAvoidenceRatio = 1.0f;
        wanderRatio = 0.5f;
        wanderFOV = 0.5f;
        wanderJitterPerSec = 10.0f;

        steeringType = eSteeringType_Straight;
        frictionK = 10.0f;

        wallAvoidenceOn = true;
        sphereAvoidenceOn = true;
        wanderOn = true;
        frictionOn = false;
        gravityOn = false;
    }
#else
    virtual void SetDefaults()
    {
        maxAcc = 10.0f;
        maxSpeed = 8.0f;
        turnSpringK = 10.0f;
        wallAvoidenceRatio = 1.0f;
        wallFeelerLength = 2.0f;
        sphereAvoidenceRatio = 1.0f;
        sphereAvoidenceRange = 2.0f;
        sphereAvoidenceBreakRatio = 0.0f;
        wanderRatio = 0.5f;
        wanderFOV = 0.5f;
        wanderJitterPerSec = 10.0f;

        steeringType = eSteeringType_Straight;

        wallAvoidenceOn = true;
        sphereAvoidenceOn = true;
        wanderOn = true;
        gravityOn = false;
    }
#endif
};

class zCompSteeringGeneric : public zICompSteering
{
public:
    zCompSteeringGeneric();

    virtual void Attached();
#if RELEASE
    virtual void SetParameters(const zCompParameters* parameters);
#endif
    virtual void Detached();
#if RELEASE
    virtual void Reset();
#endif
    virtual void AllAttached() {}
    virtual void Update(xScene* xscn, F32 dt);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    virtual void ApplySteering(zICompNPCEntity* npcEnt, F32 dt);
    virtual void SetDestination(xVec3* vec) { destination = vec; }
#if RELEASE
    virtual void SetFacing(xVec3* vec) { facingPos = vec; }
#else
    virtual void SetFacing(xVec3* vec) { facing = vec; }

    virtual void SetParameters(const zCompParameters* parameters) { steeringParameters = (const zCompSteeringGenericParams*)parameters; }
#endif

protected:
#if RELEASE
    virtual void CalculateAcc(xVec3& acc, zSteeringGenericDynamics& steeringDynamics, F32 dt);
    virtual bool CalculateVelConstraints(xVec3& dVel, zSteeringGenericDynamics& steeringDynamics, F32 dt);
    virtual bool CalculatePosConstraints(xVec3& dPos, F32& velCorrectionRatio, zSteeringGenericDynamics& steeringDynamics, F32 dt);
    virtual void CalculateFacing(xVec3& facing, zSteeringGenericDynamics& steeringDynamics, F32);
#else
    virtual void CalculateAcc(xVec3& acc, zSteeringGenericDynamics& steeringDynamics, F32 dt);
    virtual void CalculateVelConstraints(xVec3& dVel, zSteeringGenericDynamics&, F32);
    virtual void CalculatePosConstraints(xVec3& dPos, zSteeringGenericDynamics&, F32);
    virtual void CalculateFacing(xVec3& facing, zSteeringGenericDynamics& steeringDynamics, F32);
#endif

    xVec3* destination;
#if RELEASE
    xVec3* facingPos;
#else
    xVec3* facing;
#endif

private:
    const zCompSteeringGenericParams* Params() { return steeringParameters; }
#if DEBUG || RELEASE
    void DebugRenderForce(const xVec3* entPos, xVec3* Force, bool render, xColor color);
#endif
    bool AccumulateVec(xVec3& curTotalVec, F32& curTotalMag, F32 maxMag, const xVec3& vecToAdd, F32 magToAdd);
    F32 GetWanderAcc(xVec3& wanderAcc, zSteeringGenericDynamics& steeringDynamics, F32 dt);
#if RELEASE
    F32 GetWallAvoidenceAcc(xVec3& avoidenceForce, zSteeringGenericDynamics&, F32);
#else
    void GetWallAvoidenceForce(xVec3& avoidenceForce, zSteeringGenericDynamics& steeringDynamics, F32); // UNUSED
#endif
    void GetSphereAvoidenceForce(xVec3& avoidenceForce, zSteeringGenericDynamics& steeringDynamics, F32); // UNUSED
#if RELEASE
    F32 GetVelCorrectionAcc(xVec3& corAcc, const xVec3& desiredVel, const zSteeringGenericDynamics& steeringDynamics, F32 dt);
#endif
    F32 GetSeekAcc(xVec3& seekAcc, zSteeringGenericDynamics& steeringDynamics, F32 dt);
    void GetFleeForce(xVec3& fleeForce, zSteeringGenericDynamics& steeringDynamics, F32); // UNUSED
    void GetArriveForce(xVec3& arriveForce, zSteeringGenericDynamics& steeringDynamics, F32); // UNUSED
#if RELEASE
    bool GetSweptCircleCollDPos(xVec3& collDPos, F32& velCorrectionRatio, zSteeringGenericDynamics& steeringDynamics, F32 dt);
#else
    void GetSweptSphereCollDPos(xVec3& collDPos, zSteeringGenericDynamics& steeringDynamics, F32); // UNUSED
#endif

    const zCompSteeringGenericParams* steeringParameters;
    xVec3 prevWanderDir;
    xVec3 prevRandomizerDir;
#if RELEASE
    zSweptCircle swCircle;
    S32 wallID;
#endif
    zNPCWalls* npcWalls;
};

#endif
