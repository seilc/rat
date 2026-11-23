#ifndef ZNPCCHEF_H
#define ZNPCCHEF_H

#include "zNPCCommon.h"
#include "zNPCMove.h"

class zNPCChefIdle : public zNPCCommonBehavior
{
public:
    static void create_anim_table(xAnimTable* table);

    virtual const char* getName() { return "zNPCChefIdle"; }
    virtual void enterBehavior(const zNPCCommonBehavior* prev_behavior);
    virtual void updateBehavior(F32 dt);
#if DEBUG || RELEASE
    virtual void debugRender();
#endif
    virtual bool EscapeNoEnterTransitionAllowable() { return false; }
};

class zNPCChefRun : public zNPCMove
{
public:
    static void create_anim_table(xAnimTable* table);

    // UNUSED: needed to match (speculative code)
    virtual const char* getName() { return "zNPCChefRun"; }
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

class zNPCChef : public zNPC::zNPCCommon
{
public:
    zNPCChef();

    static void create_anim_table(xAnimTable* table);

    virtual const char* get_type_name() const { return "zNPCChef"; }
    virtual void BuildStateMachine();
    virtual void init(zNPC::zNPCInstance* init_inst);
    virtual void SetDefaultAndEventStates();
    virtual S32 GetNPCType() { return NPC_TYPE_CHEF; }
    virtual bool systemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);

private:
    zNPCChefIdle* idleState;
    zNPCChefRun* moveState;
};

#endif
