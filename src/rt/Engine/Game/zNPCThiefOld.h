#ifndef ZNPCTHIEFOLD_H
#define ZNPCTHIEFOLD_H

#include "zNPCCommon.h"
#include "zNPCMove.h"

class zNPCThiefIdle : public zNPCCommonBehavior
{
public:
    static void create_anim_table(xAnimTable* table);

    virtual const char* getName() { return "zNPCThiefIdle"; }
    virtual void enterBehavior(const zNPCCommonBehavior* prev_behavior);
    virtual void updateBehavior(F32 dt);
#if DEBUG || RELEASE
    virtual void debugRender();
#endif
    virtual bool EscapeNoEnterTransitionAllowable() { return false; }
};

class zNPCThiefRun : public zNPCMove
{
public:
    static void create_anim_table(xAnimTable* table);

    // UNUSED: needed to match (speculative code)
    virtual const char* getName() { return "zNPCThiefRun"; }
    virtual void enterBehavior(const zNPCCommonBehavior* prev_behavior); // UNUSED
    virtual void updateBehavior(F32 dt); // UNUSED
#if DEBUG || RELEASE
    virtual void debugRender(); // UNUSED
#endif
    inline virtual bool EscapeNoEnterTransitionAllowable(); // UNUSED

    void PickDest(); // UNUSED

private:
    xVec3 targetPos;
    F32 timeElapsed;
    bool arrived;
};

class zNPCThiefOld : public zNPC::zNPCCommon
{
public:
    zNPCThiefOld();

    static void create_anim_table(xAnimTable* table);

    virtual const char* get_type_name() const { return "zNPCThiefOld"; }
    virtual void BuildStateMachine();
    virtual void init(zNPC::zNPCInstance* init_inst);
    virtual void SetDefaultAndEventStates();
    virtual S32 GetNPCType() { return NPC_TYPE_THIEF; }

private:
    zNPCThiefIdle* idleState;
    zNPCThiefRun* moveState;
};

#endif
