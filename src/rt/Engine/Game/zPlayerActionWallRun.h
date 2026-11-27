#ifndef ZPLAYERACTIONWALLRUN_H
#define ZPLAYERACTIONWALLRUN_H

#include "zCommonPlayerActions.h"

class zPlayerActionWallRun : public zCommonPlayerAction
{
public:
    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags);
    virtual U32 GetID() { return 0x1A; }
    virtual void AddStates(xAnimTable* table);
    virtual void AddInternalTransitions(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable* table);
    virtual void Reset();
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void NotifyCollision(const xVec3& currentDir, const SphereCollisionResults& sceneCollide);
    virtual bool CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos);
    virtual void Begin();
    virtual void End();
#if DEBUG || RELEASE
    virtual void AddTweaks();
#endif

    PLAYER_ACTION_DEST_CHECK(zPlayerActionWallRun, StartLeftCheck)
    PLAYER_ACTION_DEST_CHECK(zPlayerActionWallRun, StartRightCheck)
    PLAYER_ACTION_CHECK_L(zPlayerActionWallRun, FallCheck, 55)

protected:
    F32 targetAngle;
    U8 mountState;
    bool fall;
    bool facingCorrectDir;
    U32 transitionFromID;
};

class zPlayerActionWallRunJump : public zPlayerAction
{
public:
    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags);
    virtual U32 GetID() { return 0x1B; }
    virtual void AddStates(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable* table);
    virtual void Begin();
#if DEBUG || RELEASE
    virtual void AddTweaks();
#endif

    const char* GetTransitionString() { return "WallRunJump*"; }

    PLAYER_ACTION_DEST_CHECK(zPlayerActionWallRunJump, StartLeftCheck)
    PLAYER_ACTION_DEST_CHECK(zPlayerActionWallRunJump, StartRightCheck)
};

#endif
