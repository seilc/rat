#ifndef ZCONTEXTOBJECT_H
#define ZCONTEXTOBJECT_H

#include "xBase.h"
#include "xMath3.h"
#include "containers.h"

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
            U8 forward;
        } poleSwing;
    };

    virtual U8 Mount(zCommonPlayer* player, MountData& mountData, F32 dt) = 0;
    virtual zContextObjectType GetType() const = 0;
    virtual void Dismount() {}
};

class zContextObjectManager
{
protected:
    zContextObject::MountData mountData;
    zContextObject* currentObject;
    U8 mounted;
    bool attemptContextMove;
    bit_array<3> transition;
    U32 animStateID;
};

#endif
