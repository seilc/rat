#include "xCommon.h"
#include "zNPCThiefOld.h"

#include "containers.h"
#include "xCam.h"
#include "xEntMotion.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "zPlayerContainer.h"

namespace {

xTWEAKABLE F32 PLAYER_TOLERANCE_DISTANCE = 5.0f;
xTWEAKABLE F32 PLAYER_TOLERANCE_DISTANCE2 = xsqr(PLAYER_TOLERANCE_DISTANCE);
xTWEAKABLE F32 RUN_GET_NEW_DEST_TIME = 8.0f;

}

void zNPCThiefIdle::create_anim_table(xAnimTable* table)
{
    xAnimTableNewState(table, "THIEF_L_IDLE01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}

void zNPCThiefIdle::enterBehavior(const zNPCCommonBehavior*)
{
    setAnimState("THIEF_L_IDLE01", 0.3f, NULL);
}

void zNPCThiefIdle::updateBehavior(F32)
{
}

#if DEBUG || RELEASE
void zNPCThiefIdle::debugRender()
{
}
#endif

void zNPCThiefRun::create_anim_table(xAnimTable* table)
{
    xAnimTableNewState(table, "THIEF_L_MOVE01", AnimState_Loop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}

// UNUSED: needed to match (speculative code)
void zNPCThiefRun::enterBehavior(const zNPCCommonBehavior*)
{
    setAnimState("THIEF_L_MOVE01", 0.3f, NULL);
    PickDest();
    timeElapsed = 0.0f;
    arrived = false;
}

// UNUSED: needed to match (speculative code)
void zNPCThiefRun::updateBehavior(F32 dt)
{
    bool getNewDest = false;

    timeElapsed += dt;

    xVec3 toDest;
    toDest.Sub(targetPos, *xEntGetPos(owner));
    if (toDest.length2() <= xsqr(0.4f)) {
        if (zNPC::CheckForPlayerInRange2(owner, PLAYER_TOLERANCE_DISTANCE2)) {
            getNewDest = true;
        } else {
            arrived = true;
        }
    }

    if (is_stuck() || timeElapsed >= RUN_GET_NEW_DEST_TIME) {
        getNewDest = true;
    }

    if (!arrived) {
        if (getNewDest) {
            PickDest();
            timeElapsed = 0.0f;
            toDest.Sub(targetPos, *xEntGetPos(owner));
        }
        apply_angle_diff(get_angle_difference(&owner->frame->mat.at, &toDest), dt, 2.0f * PI);
    }
}

zNPCThiefOld::zNPCThiefOld()
{
}

void zNPCThiefOld::create_anim_table(xAnimTable* table)
{
    zNPCThiefIdle::create_anim_table(table);
    zNPCThiefRun::create_anim_table(table);
    zNPCCommon::create_anim_table(table);
}

void zNPCThiefOld::BuildStateMachine()
{
    zNPC_ADD_BEHAVIOR(idleState, zNPCThiefIdle);
}

void zNPCThiefOld::init(zNPC::zNPCInstance* init_inst)
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

void zNPCThiefOld::SetDefaultAndEventStates()
{
    manager.setDefaultBehavior(idleState);
}
