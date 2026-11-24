#include "xCommon.h"
#include "zNPCBehavior.h"

#include "xCam.h"
#include "xEntMotion.h"
#include "xFX.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xMovePoint.h"
#include "xPtankPool.h"
#include "xSpacePartitionTree.h"
#include "xSpline.h"
#include "zCombatSystem.h"
#include "zCommonPlayer.h"
#include "zPlayerContainer.h"

#include <stdio.h>

namespace zNPC {

F32 move::get_angle_difference(const xVec3* direction) const
{
    xASSERT_L(75, direction);

    F32 angle_caller = xatan2(direction->x, direction->z);

    F32 angle_true = xatan2(xEntGetFacingDirection(owner)->x, xEntGetFacingDirection(owner)->z);

    return xDangleClamp(angle_caller - angle_true);
}

// UNUSED: needed to match (code from RotU GC)
F32 move::get_angle_difference(const xVec3* startDir, const xVec3* endDir) const
{
    xASSERT(startDir);
    xASSERT(endDir);

    F32 angle_caller = xatan2(endDir->x, endDir->z), angle_true = xatan2(startDir->x, startDir->z);

    return xDangleClamp(angle_caller - angle_true);
}

F32 move::turn_to_face(const xVec3* direction, F32 dt, F32 turning_speed)
{
    if (!direction) {
        return 0.0f;
    }

    if (turning_speed == -1.0f) {
        turning_speed = turn_speed;
    }

    return apply_angle_diff(get_angle_difference(direction), dt, turning_speed);
}

F32 move::apply_angle_diff(F32 angle_difference, F32 dt, F32 turning_speed)
{
    F32 delta_angle = CLAMP(angle_difference, -turning_speed * dt, turning_speed * dt);

    xMat3x3 drot;
    xMat3x3Rot(&drot, &xVec3::m_UnitAxisY, delta_angle);
    xMat3x3Mul(&owner->frame->mat, &owner->frame->oldmat, &drot);
    return delta_angle;
}

void move::accelerate(F32 dt, const xVec3& direction, bool change_anim_speed, F32 move_speed)
{
    move_speed *= owner->move_speed_multiply;
    if (change_anim_speed) {
        owner->model->Anim->Single->State->Speed = owner->move_speed_multiply;
        owner->model->Anim->Single->CurrentSpeed = owner->move_speed_multiply;
    }

    if (owner->model->Anim->Single->SingleFlags & 0x4) {
        speed = owner->frame->dpos.length() / dt;
        return;
    }

    if (move_speed < 0.0f) {
        move_speed = this->move_speed * owner->move_speed_multiply;
    }

    if (speed + dt * acceleration > move_speed) {
        speed = move_speed;
    }
    else if (speed + dt * acceleration < min_speed) {
        speed = min_speed;
    } else {
        speed += dt * acceleration;
    }

    owner->frame->mode |= k_XENT_SET_DPOS;
    owner->frame->setDpos().AddScale(direction, speed * dt);

    if (xEntGetAnimFlags(owner) & 0x1) {
        owner->model->Anim->Single->CurrentSpeed = speed / move_speed;
        owner->model->Anim->Single->State->Speed = owner->model->Anim->Single->CurrentSpeed;
    }
}

void move::stop()
{
    xVec3Init(owner->frame->setDpos(), 0.0f, 0.0f, 0.0f);
    speed = 0.0f;
}

void move::setup()
{
    F32 turn_speed_degrees;
    owner->GetParameter("TurnSpeed", &turn_speed_degrees, 180.0f);
    turn_speed = (PI / 180.0f) * turn_speed_degrees;
    owner->GetParameter("MoveSpeed", &move_speed, 4.0f);
    owner->GetParameter("MinSpeed", &min_speed, 4.0f);
    owner->GetParameter("Acceleration", &acceleration, 10.0f);
}

void move::reset()
{
    speed = 0.0f;
}

#if DEBUG || RELEASE
void move::add_tweaks(const char* prefix)
{
    char buffer[1024];
    sprintf(buffer, "%sMaxSpeed", prefix);
    xTWEAK(buffer, &move_speed, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%sMinSpeed", prefix);
    xTWEAK(buffer, &min_speed, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%sTurnSpeed", prefix);
    xTWEAK(buffer, &turn_speed, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%sAcceleration", prefix);
    xTWEAK(buffer, &acceleration, 0.0f, 1000.0f, NULL, NULL, 0x2);
}
#endif

// UNUSED: needed to match (speculative code)
void take_damage::create_anim_table(xAnimTable* table)
{
    xAnimTableNewState(table, "TD_T_Hit01", AnimState_CopyOnWrite | AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, zCombatHitChooser, NULL, NULL);
    xAnimTableNewState(table, "TD_T_HitMelee01", AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}

// UNUSED: needed to match (speculative code)
void take_damage::enter_state(const zNPCCommonBehavior*)
{
    setAnimState("TD_T_Hit01", 0.3f, NULL);

    xVec3 direction;
    direction.Sub(owner->frame->mat.pos, xglobals->players[0]->frame->mat.pos);
    direction.y = 0.0f;
}

// UNUSED: needed to match (code from Incredibles GC)
bool take_damage::damage(zCombatDamageInfo& damageInfo)
{
    hasBeenDamaged = true;
    contact = damageInfo.hitLocation;
    direction = damageInfo.direction;

    if (owner->get_current_behavior() == this && damageInfo.source != zHS_THROW) {
        zCombatDamage(owner, zCombatDamageInfo(damageInfo.from, 0, zHS_THROW, zHT_GENERAL, damageInfo.knockback, xVec3::m_Null, xVec3::m_Null, 0));
    }

    return false;
}

// UNUSED: needed to match (code from RotU GC)
void idle::enter_state(const zNPCCommonBehavior*)
{
    setAnimState("ID_L_Idle01", 0.3f, NULL);
}

}
