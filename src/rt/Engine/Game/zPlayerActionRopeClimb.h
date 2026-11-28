#ifndef ZPLAYERACTIONROPECLIMB_H
#define ZPLAYERACTIONROPECLIMB_H

#include "zCommonPlayerContextActions.h"

class zPlayerActionRopeClimb : public zPlayerContextAction
{
public:
    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags);
    virtual U32 GetID() { return 0x18; }
    virtual void AddStates(xAnimTable* table);
    virtual void AddInternalTransitions(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable* table);
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void Begin();
    virtual void End();
#if DEBUG || RELEASE
    virtual void AddTweaks();
#endif

    PLAYER_ACTION_DEST_CHECK(zPlayerActionRopeClimb, StartCheck)
    PLAYER_ACTION_CHECK_L(zPlayerActionRopeClimb, MountedCB, 38)
    PLAYER_ACTION_CHECK_L(zPlayerActionRopeClimb, StickUpCheck, 39)
    PLAYER_ACTION_CHECK_L(zPlayerActionRopeClimb, StickDownCheck, 40)
    PLAYER_ACTION_CHECK_L(zPlayerActionRopeClimb, StickNeutralCheck, 41)
    PLAYER_ACTION_CHECK_L(zPlayerActionRopeClimb, DismountCheck, 42)

protected:
    bool dismount;
};

class zPlayerActionRopeJump : public zPlayerAction
{
public:
    const char* GetTransitionString() { return "RopeJump*"; }

    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags);
    virtual U32 GetID() { return 0x19; }
    virtual void AddStates(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable* table);

    PLAYER_ACTION_DEST_CHECK(zPlayerActionRopeJump, StartUpCheck)
    PLAYER_ACTION_DEST_CHECK(zPlayerActionRopeJump, StartDownCheck)
};

#endif
