#include "xCommon.h"
#include "zNPCChef.h"

#include "containers.h"
#include "xCam.h"
#include "xEntMotion.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "zEvent.h"
#include "zPlayerContainer.h"

namespace {

xTWEAKABLE F32 PLAYER_TOLERANCE_DISTANCE = 5.0f;
xTWEAKABLE F32 PLAYER_TOLERANCE_DISTANCE2 = xsqr(PLAYER_TOLERANCE_DISTANCE);

}

void zNPCChefIdle::create_anim_table(xAnimTable* table)
{
    xAnimTableNewState(table, "CHEF_IDLE01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}

void zNPCChefIdle::enterBehavior(const zNPCCommonBehavior*)
{
    setAnimState("CHEF_IDLE01", 0.3f, NULL);
}

void zNPCChefIdle::updateBehavior(F32)
{
    S32 doingnothing = 0;
}

#if DEBUG || RELEASE
void zNPCChefIdle::debugRender()
{
}
#endif

void zNPCChefRun::create_anim_table(xAnimTable* table)
{
    xAnimTableNewState(table, "CHEF_MOVE01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}

// UNUSED: needed to match (speculative code)
void zNPCChefRun::enterBehavior(const zNPCCommonBehavior*)
{
    setAnimState("CHEF_MOVE01", 0.3f, NULL);
    PickDest();
    timeElapsed = 0.0f;
    arrived = false;
}

// UNUSED: needed to match (speculative code)
void zNPCChefRun::updateBehavior(F32 dt)
{
    bool getNewDest = false;

    (void)dt;

    xVec3 toDest;
    toDest.Sub(targetPos, *xEntGetPos(owner));
    if (toDest.length2() <= xsqr(0.4f)) {
        if (zNPC::CheckForPlayerInRange2(owner, PLAYER_TOLERANCE_DISTANCE2)) {
            getNewDest = true;
        } else {
            arrived = true;
        }
    }

    if (is_stuck()) {
        getNewDest = true;
    }

    if (!arrived) {
        if (getNewDest) {
            PickDest();
            toDest.Sub(targetPos, *xEntGetPos(owner));
        }
        apply_angle_diff(get_angle_difference(&owner->frame->mat.at, &toDest), dt, 2.0f * PI);
    }
}

zNPCChef::zNPCChef()
{
}

void zNPCChef::create_anim_table(xAnimTable* table)
{
    zNPCChefIdle::create_anim_table(table);
    zNPCChefRun::create_anim_table(table);
    zNPCCommon::create_anim_table(table);
}

void zNPCChef::BuildStateMachine()
{
    zNPC_ADD_BEHAVIOR(idleState, zNPCChefIdle);
}

void zNPCChef::init(zNPC::zNPCInstance* init_inst)
{
    zNPCCommon::init(init_inst);
    commonFlags.cast_shadow = false;
    manager.createTransientStack(1);
    BuildStateMachine();
    SetDefaultAndEventStates();
    canJump = false;
    canDodge = false;
    canTakeDamage = false;
}

void zNPCChef::SetDefaultAndEventStates()
{
    manager.setDefaultBehavior(idleState);
}

bool zNPCChef::systemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id)
{
    zNPC_STORE_EVENT(from, to, to_event, to_param, to_param_widget);

    switch (to_event) {
    case eEventNPCEnterIdleSpawn:
        break;
    }

    return zNPCCommon::systemEvent(from, to, to_event, to_param, to_param_widget, to_param_widget_id);
}
