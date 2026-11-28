#ifndef ZCONTEXTOBJECT_H
#define ZCONTEXTOBJECT_H

#include "xBase.h"
#include "xMath3.h"
#include "containers.h"

struct xEnt;
struct xEntFrame;
struct zCommonPlayer;

enum zContextObjectType
{
    ezContextObjectType_Tightrope,
    ezContextObjectType_PoleSwing,
    ezContextObjectType_ClimbableRope,
    ezContextObjectType_Count
};

class zContextObject : public xBase
{
public:
    union MountData
    {
        struct
        {
            F32 u;
        } tightrope;
        struct
        {
            xVec3 pos;
            bool forward;
        } poleSwing;
    };

#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    static bool MoveTowardsMountPoint(xEntFrame* frame, const xVec3& mountPoint);
    static bool MatchOrientation(xEntFrame* frame, const xVec3& orientation, F32 dt, bool forward);
    static bool MatchOrientation(xEntFrame* frame, F32 targetAngle, F32 dt);
    static F32 GetPriority(F32 distance2) { return distance2; }

    virtual bool Mount(zCommonPlayer* player, MountData& mountData, F32 dt) = 0;
    virtual zContextObjectType GetType() const = 0;
    virtual void Dismount() {}
};

class zContextObjectManager
{
public:
    void Dismount(zCommonPlayer* player);
    bool CheckObjectType(zContextObjectType objectType);
    zContextObject* GetCurrentObject() { return currentObject; }
    static void AddEntFX(xEnt* ent);
    void Reset();

protected:
    zContextObject::MountData mountData;
    zContextObject* currentObject;
    bool mounted;
    bool attemptContextMove;
    bit_array<3> transition;
    U32 animStateID;
};

#endif
