#include "xCommon.h"
#include "zPlayerActionRopeClimb.h"

#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "zContextObjectClimbableRope.h"

namespace
{
    xTWEAKABLE F32 DEAD_ZONE = 0.8f;
} // namespace

#if DEBUG || RELEASE
void zPlayerActionRopeClimb::AddTweaks()
{
    const char* prefix = "Player|Context Objects|Climbable Rope|";
    zContextObjectClimbableRope::AddStaticTweaks();

    xAUTOTWEAK(prefix, "DEAD_ZONE", &DEAD_ZONE, 0.0f, 1000.0f, NULL, NULL, 0, true);
}
#endif

void zPlayerActionRopeClimb::AddStates(xAnimTable* table)
{
    NewState(table, "RopeClimbReach01", AnimState_EndLoop, 0x20000);
    NewState(table, "RopeClimbGrab01", AnimState_EndTransition, 0x20000);
    NewState(table, "RopeClimbUpIdle01", AnimState_EndLoop, 0x4000);
    NewState(table, "RopeClimbUpMove01", AnimState_EndLoop, 0x14000);
    NewState(table, "RopeClimbDownIdle01", AnimState_EndLoop, 0x8000);
    NewState(table, "RopeClimbDownMove01", AnimState_EndLoop, 0x18000);
    NewState(table, "RopeClimbFlipUp01", AnimState_EndTransition, 0);
    NewState(table, "RopeClimbFlipDown01", AnimState_EndTransition, 0);
    NewState(table, "RopeClimbDismountUp01", AnimState_EndTransition, 0);
    NewState(table, "RopeClimbDismountDown01", AnimState_EndTransition, 0);
}

U32 zPlayerActionRopeClimb::StickUpCheck(xAnimTransition*, xAnimSingle*)
{
    return player->GetPlayerInput().stick1.y > DEAD_ZONE;
}

U32 zPlayerActionRopeClimb::StickDownCheck(xAnimTransition*, xAnimSingle*)
{
    return player->GetPlayerInput().stick1.y < -DEAD_ZONE;
}

U32 zPlayerActionRopeClimb::StickNeutralCheck(xAnimTransition*, xAnimSingle*)
{
    return xabs(player->GetPlayerInput().stick1.y) <= DEAD_ZONE;
}

U32 zPlayerActionRopeClimb::StartCheck(xAnimTransition*, xAnimSingle*)
{
    return GetCommonPlayer()->contextManager.CheckObjectType(ezContextObjectType_ClimbableRope);
}

U32 zPlayerActionRopeClimb::DismountCheck(xAnimTransition*, xAnimSingle*)
{
    return dismount;
}

U32 zPlayerActionRopeClimb::MountedCB(xAnimTransition*, xAnimSingle*)
{
    player->Set_gravityDisabled();
    player->frame->vel.assign(0.0f);
    return 0;
}

void zPlayerActionRopeClimb::AddActionTransitions(xAnimTable* table)
{
    AddActionTransition(table, "RopeClimbDismount*", "Fall01", NULL, 1000, 0.15f, AnimTransition_Default, 0);
    manager->AddStandardTransitionsTo(0x19, table, "RopeClimbUp* RopeClimbDown*");
}

void zPlayerActionRopeClimb::AddInternalTransitions(xAnimTable* table)
{
    xAnimTableNewTransition(table, "RopeClimbReach01", "RopeClimbGrab01", anMountedCheck, anMountedCB, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbGrab01", "RopeClimbUpIdle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);

    xAnimTableNewTransition(table, "RopeClimbUpIdle01", "RopeClimbUpMove01", anStickUpCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbUpMove01", "RopeClimbUpIdle01", anStickNeutralCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbFlipUp01", "RopeClimbUpMove01", anStickUpCheck, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbFlipUp01", "RopeClimbUpIdle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbUp*", "RopeClimbFlipDown01", anStickDownCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbUpMove01", "RopeClimbDismountUp01", anDismountCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);

    xAnimTableNewTransition(table, "RopeClimbDownIdle01", "RopeClimbDownMove01", anStickDownCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbDownMove01", "RopeClimbDownIdle01", anStickNeutralCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbFlipDown01", "RopeClimbDownMove01", anStickDownCheck, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbFlipDown01", "RopeClimbDownIdle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbDown*", "RopeClimbFlipUp01", anStickUpCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RopeClimbDownMove01", "RopeClimbDismountDown01", anDismountCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
}

void zPlayerActionRopeClimb::AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    AddActionTransition(table, fromStates, "RopeClimbReach01", anStartCheck, priority, blend, flags, userFlags);
}

void zPlayerActionRopeClimb::Begin()
{
    dismount = false;
}

void zPlayerActionRopeClimb::Move(xScene* s, F32 dt, xEntFrame* frame)
{
    U32 animFlags = xEntGetAnimFlags(player);
    GetCommonPlayer()->HandleMovement(s, dt, frame);
    zContextObjectClimbableRope* rope = (zContextObjectClimbableRope*)GetContextObjectManager().GetCurrentObject();

    const xVec3& pos = player->frame->mat.pos;
    xVec3& dpos = player->frame->dpos;
    xDrawSetColor(g_BLUE);
    xDrawSphere(&pos, 0.5f, 0xC0006);

    if (!(animFlags & 0x20000)) {
        if ((animFlags & 0x14000) == 0x14000) {
            dismount = rope->Ascend(dt);
        }
        else if ((animFlags & 0x18000) == 0x18000) {
            dismount = rope->Descend(dt);
        } else {
            rope->SwingInput(player->GetPlayerInput().stick1.x);
        }

        rope->UpdateClimberFrame();
    }

    rope->ClearCollisionFlags();
    GetCommonPlayer()->DoCollision(dt);
    rope->RestoreCollisionFlags();
}

void zPlayerActionRopeClimb::End()
{
    GetContextObjectManager().Dismount(GetCommonPlayer());
}

void zPlayerActionRopeJump::AddStates(xAnimTable* table)
{
    NewState(table, "RopeJumpUp01", AnimState_EndLoop, 0x4);
    NewState(table, "RopeJumpDown01", AnimState_EndLoop, 0x4);
}

void zPlayerActionRopeJump::AddActionTransitions(xAnimTable* table)
{
    zCommonPlayerActionsAirTransitions(manager, table, GetTransitionString());
}

void zPlayerActionRopeJump::AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    AddActionTransition(table, fromStates, "RopeJumpUp01", anStartUpCheck, priority, blend, flags, userFlags);
    AddActionTransition(table, fromStates, "RopeJumpDown01", anStartDownCheck, priority, blend, flags, userFlags);
}

U32 zPlayerActionRopeJump::StartUpCheck(xAnimTransition*, xAnimSingle* anim)
{
    return player->GetPlayerInput().pressed.jump && (anim->State->UserFlags & 0x4000);
}

U32 zPlayerActionRopeJump::StartDownCheck(xAnimTransition*, xAnimSingle* anim)
{
    return player->GetPlayerInput().pressed.jump && (anim->State->UserFlags & 0x8000);
}
