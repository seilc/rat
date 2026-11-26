#ifndef ZSPRINGBOARD_H
#define ZSPRINGBOARD_H

#include "xDynAsset.h"
#include "xEnt.h"

struct xAnimSingle;
struct xAnimTable;
struct xAnimTransition;
struct SphereCollisionResults;

struct zSpringboardAsset : xDynAsset
{
    enum { VERSION = 4 };

    enum Type
    {
        BouncePad,
        Trampoline,
        Chargeboard
    };

    xVec3 location;
    F32 yaw;
    F32 pitch;
    F32 roll;
    U32 modelID;
    F32 adjustment;
    U8 top;
    U8 type;
    U8 size;
};

class zSpringboard : public xEnt
{
public:
    enum State
    {
        Idle,
        Compressing,
        Compressed,
        Launching
    };

    static xAnimTable* CreateAnimTable();
    static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    static U32 anChargeboardCheck(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->ChargeboardCheck(tran, anim); }
    static U32 anCompressCheck(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->CompressCheck(tran, anim); }
    static U32 anLaunchCheck(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->LaunchCheck(tran, anim); }
    static U32 anLaunchCB(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->LaunchCB(tran, anim); }
    static U32 anIdleCB(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->IdleCB(tran, anim); }
    static U32 anCompressedCB(xAnimTransition* tran, xAnimSingle* anim, void* object) { return ((zSpringboard*)object)->CompressedCB(tran, anim); }

    void Launch();
    void Compress();
    U32 CompressCheck(xAnimTransition*, xAnimSingle*);
    U32 IdleCB(xAnimTransition*, xAnimSingle*);
    U32 LaunchCheck(xAnimTransition*, xAnimSingle*);
    U32 LaunchCB(xAnimTransition*, xAnimSingle*);
    U32 ChargeboardCheck(xAnimTransition*, xAnimSingle*);
    U32 CompressedCB(xAnimTransition*, xAnimSingle*);
    void EventCB(xBase*, U32 toEvent, const F32*, xBase*, U32);
    void Reset();

    zSpringboard(zSpringboardAsset* theAsset);

    void GetLaunchVelocity(xVec3& vel);
    bool Mount(const SphereCollisionResults& results);

protected:
    zSpringboardAsset* asset;
    State state;
    F32 angle;
    F32 cosAngle;
    F32 sinAngle;
};

#endif
