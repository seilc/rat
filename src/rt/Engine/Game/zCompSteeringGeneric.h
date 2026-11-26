#ifndef ZCOMPSTEERINGGENERIC_H
#define ZCOMPSTEERINGGENERIC_H

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

class zCompSteeringGenericParams : public zCompParameters
{
public:
    zCompSteeringGenericParams() { SetDefaults(); }

#if RELEASE
    void AddTweaks(const char* parentName, const char* branchName);
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
    virtual void SetDefaults();
#endif
};

class zCompSteeringGeneric : public zICompSteering
{
protected:
    xVec3* destination;
#if RELEASE
    xVec3* facingPos;
#else
    xVec3* facing;
#endif

private:
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
