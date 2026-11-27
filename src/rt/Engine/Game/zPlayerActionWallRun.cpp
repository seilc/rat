#include "xCommon.h"
#include "zPlayerActionWallRun.h"

#include "xCollideSweptSphere.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xScene.h"
#include "zGlobals.h"
#include "zScene.h"

namespace
{
    xTWEAKABLE F32 WALL_HIT_COS_ANGLE = 0.70710677f;
    xTWEAKABLE F32 WALL_DIST = 1.0f;
} // namespace

#if DEBUG || RELEASE
void zPlayerActionWallRun::AddTweaks()
{
    const char* prefix = "Player|Context Objects|Wall Run|";
    xAUTOTWEAK(prefix, "Wall hit angle", &WALL_HIT_COS_ANGLE, 0.0f, 90.0f, &cb_tweak_convert_cos, NULL, 0, false);
    xAUTOTWEAK(prefix, "WALL_DIST", &WALL_DIST, 0.0f, 1000.0f, NULL, NULL, 0, true);
}
#endif

void zPlayerActionWallRun::AddStates(xAnimTable* table)
{
    NewState(table, "WallRunningLeft01", AnimState_EndTransition, 0x4000);
    NewState(table, "WallRunningRight01", AnimState_EndTransition, 0x8000);
    NewState(table, "WallRunningLeftFall01", AnimState_EndTransition, 0x4);
    NewState(table, "WallRunningRightFall01", AnimState_EndTransition, 0x4);
}

void zPlayerActionWallRun::AddActionTransitions(xAnimTable* table)
{
    manager->AddStandardTransitionsTo(8, table, "WallRunning*Fall#");
    manager->AddStandardTransitionsTo(0x1B, table, "WallRunningLeft01 WallRunningRight01");
    AddActionTransition(table, "WallRunning*", "Land02", anLandCheck, 1001, 0.15f, 0, 0);
    AddActionTransition(table, "WallRunning*", "Fall02", NULL, 1000, 0.15f, AnimTransition_Default, 0);
}

void zPlayerActionWallRun::AddInternalTransitions(xAnimTable* table)
{
    xAnimTableNewTransition(table, "WallRunningLeft01", "WallRunningLeftFall01", anFallCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "WallRunningRight01", "WallRunningRightFall01", anFallCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
}

void zPlayerActionWallRun::AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    AddActionTransition(table, fromStates, "WallRunningLeft01", anStartLeftCheck, priority, blend, flags, userFlags);
    AddActionTransition(table, fromStates, "WallRunningRight01", anStartRightCheck, priority, blend, flags, userFlags);
}

void zPlayerActionWallRun::Move(xScene* s, F32 dt, xEntFrame* frame)
{
    bool nextFacingCorrectDir = true;
    if (!facingCorrectDir) {
        nextFacingCorrectDir = zContextObject::MatchOrientation(frame, targetAngle, dt);
    }

    U32 animFlags = xEntGetAnimFlags(player);
    if (!(animFlags & 0xC000)) {
        GetCommonPlayer()->HandleControl(dt);
    }
    GetCommonPlayer()->HandleMovement(s, dt, frame);
    GetCommonPlayer()->DoCollision(dt);

    if (facingCorrectDir && (animFlags & 0xC000)) {
        xRay3 ray;
        xCollis collisionResults;

        ray.dir = player->frame->mat.right;
        if (xEntGetAnimFlags(player) & 0x8000) {
            ray.dir.negate();
        }

        ray.origin = player->frame->mat.pos;
        ray.origin.y += player->bound.sph.r;
        ray.origin.AddScale(ray.dir, player->bound.sph.r - 0.5f * WALL_DIST);
        xDrawSetColor(g_RED);
        xDrawArrow(&ray.origin, &ray.dir, false, WALL_DIST);

        ray.min_t = 0.0f;
        ray.max_t = WALL_DIST;
        ray.flags = XRAY3_USE_MAX;

        memset(&collisionResults, 0, sizeof(collisionResults));

        xRayHitsSceneFlags(globals.sceneCur, &ray, &collisionResults, k_XENT_COLLTYPE_PC, k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_ENV);

        if (!(collisionResults.flags & k_HIT_IT)) {
            fall = true;
        }
    }

    facingCorrectDir = nextFacingCorrectDir;
}

void zPlayerActionWallRun::Reset()
{
    mountState = 0;
}

void zPlayerActionWallRun::Begin()
{
    mountState = 0;
    fall = false;
    facingCorrectDir = false;
    player->Set_gravityDisabled();
}

void zPlayerActionWallRun::End()
{
    player->Clear_gravityDisabled();
}

U32 zPlayerActionWallRun::StartLeftCheck(xAnimTransition*, xAnimSingle* anim)
{
    transitionFromID = anim->State->ID;
    return mountState == 1;
}

U32 zPlayerActionWallRun::StartRightCheck(xAnimTransition*, xAnimSingle* anim)
{
    transitionFromID = anim->State->ID;
    return mountState == 2;
}

U32 zPlayerActionWallRun::FallCheck(xAnimTransition*, xAnimSingle*)
{
    return fall;
}

bool zPlayerActionWallRun::CollisionResponse(const xVec3& currentDir, const SphereCollisionResults& sceneCollide, xVec3& responseDpos)
{
    if (facingCorrectDir) {
        fall = true;
    }
    return player->DefaultCollisionResponse(currentDir, sceneCollide, responseDpos);
}

void zPlayerActionWallRun::NotifyCollision(const xVec3& currentDir, const SphereCollisionResults& sceneCollide)
{
    if (manager->GetCurrentActionID() == GetID()) {
        return;
    }

    if (!player->GetPlayerInput().on.contextMove) {
        return;
    }

    if (sceneCollide.optr != NULL) {
        return;
    }

    if (transitionFromID != player->model->Anim->Single->State->ID) {
        return;
    }

    if (xfeq0(currentDir.x) && xfeq0(currentDir.z)) {
        return;
    }

    xVec3 dir = currentDir;
    if (!xfeq0(dir.y)) {
        dir.y = 0.0f;
        dir.normalize();
    }

    if (!xfeq0(sceneCollide.normal.y)) {
        return;
    }

    F32 dot = sceneCollide.normal.dot(currentDir);
    if (dot > 0.0f || dot < -WALL_HIT_COS_ANGLE) {
        return;
    }

    xVec3 facingDir;
    xVec3Copy(&facingDir, &dir);
    facingDir.AddScale(sceneCollide.normal, -dot);

    xVec3 wallRight;
    wallRight.cross(sceneCollide.normal, xVec3::m_UnitAxisY);

    targetAngle = xatan2(wallRight.x, wallRight.z);

    if (wallRight.dot(facingDir) > 0.0f) {
        mountState = 2;
    } else {
        mountState = 1;
        targetAngle -= PI;
    }
}

namespace
{
    xTWEAKABLE xVec3 JUMP_VEL = { 10.0f, 10.0f, 2.0f };
} // namespace

#if DEBUG || RELEASE
void zPlayerActionWallRunJump::AddTweaks()
{
    const char* prefix = "Player|Context Objects|Wall Run|";
    xAUTOTWEAK(prefix, "JUMP_VEL", &JUMP_VEL, 0, 0, NULL, NULL, 0, true);
}
#endif

void zPlayerActionWallRunJump::AddStates(xAnimTable* table)
{
    NewState(table, "WallRunJumpLeft01", AnimState_EndTransition, 0x4004);
    NewState(table, "WallRunJumpRight01", AnimState_EndTransition, 0x8004);
}

void zPlayerActionWallRunJump::AddActionTransitions(xAnimTable* table)
{
    zCommonPlayerActionsAirTransitions(manager, table, GetTransitionString());
}

void zPlayerActionWallRunJump::AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags)
{
    AddActionTransition(table, fromStates, "WallRunJumpLeft01", anStartLeftCheck, priority, blend, flags, userFlags);
    AddActionTransition(table, fromStates, "WallRunJumpRight01", anStartRightCheck, priority, blend, flags, userFlags);
}

void zPlayerActionWallRunJump::Begin()
{
    xMat3x3 playerMat = player->frame->mat;
    if (xEntGetAnimFlags(player) & 0x4000) {
        playerMat.right.negate();
    }

    xMat3x3RMulVec(&player->frame->vel, &playerMat, &JUMP_VEL);
}

U32 zPlayerActionWallRunJump::StartLeftCheck(xAnimTransition*, xAnimSingle*)
{
    return player->GetPlayerInput().pressed.jump && (xEntGetAnimFlags(player) & 0x4000);
}

U32 zPlayerActionWallRunJump::StartRightCheck(xAnimTransition*, xAnimSingle*)
{
    return player->GetPlayerInput().pressed.jump && (xEntGetAnimFlags(player) & 0x8000);
}
