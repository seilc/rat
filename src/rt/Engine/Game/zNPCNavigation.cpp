#include "xCommon.h"
#include "zNPCNavigation.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xEntMotion.h"
#include "xHud.h"
#include "xMovePoint.h"
#include "xNavigationMeshAsset.h"
#include "xPtankPool.h"
#include "xSpacePartitionTree.h"
#include "zBase.h"
#include "zCam.h"
#include "zEntTrigger.h"
#include "zMeshCollision.h"
#include "zMeshNavigation.h"
#include "zMeshPosition.h"
#include "zNPCCommon.h"
#include "zPlayerContainer.h"

namespace
{
    xTWEAKABLE F32 STEERING_MIN_DIST2 = 1.0f;
    xTWEAKABLE bool USE_STEERING = true;
#if DEBUG || RELEASE
    bool need_add_tweak = true;
#endif
} // namespace

// UNUSED: needed to match (code from RotU GC)
void zNPCJump::Init(const xVec3& inSource, const xVec3& inTarget, F32 inGravity, F32 inHeight)
{
    source = inSource;
    target = inTarget;
    gravity = inGravity;
    height = inHeight;
    active = true;

    xVec3 source_to_target;
    source_to_target.Sub(target, source);
    F32 distance = xsqrt(xsqr(source_to_target.x) + xsqr(source_to_target.z));

    F32 up_y = height, down_y = height + (source.y - target.y);

    timeUp = xsqrt(2.0f * up_y / gravity);
    velocity = timeUp * gravity;

    F32 time_down = xsqrt(2.0f * down_y / gravity);
    speed = distance / (timeUp + time_down);
    totalTime = timeUp + time_down;
    time = 0.0f;
}

// UNUSED: needed to match (code from RotU GC)
void zNPCJump::GetDirection(xVec3& direction) const
{
    xVec3Sub(direction, target, source);
    direction.y = 0.0f;
    direction.safe_normalize(xVec3::m_Null);
}

zNPCNavigate::zNPCNavigate(zNPC::zNPCCommon* npc) : npc(npc)
{
    meshPosition = new (xMEMALLOC_L(sizeof(zMeshPosition), 0, eMemMgrTag_Enemy, 0, 117)) zMeshPosition();

    reset();

#if DEBUG || RELEASE
    if (need_add_tweak) {
        need_add_tweak = false;
        xTWEAK_BOOL("NPC OLD|Steering|Enable", &USE_STEERING, NULL, NULL, 0);
        zMeshNavigationAddTweaks();
    }
#endif
}

void zNPCNavigate::reset()
{
    dpos_apply = true;

    if (my_circle.GetMesh() != NULL) {
        my_circle.GetMesh()->RemoveCircle(&my_circle);
    }

    my_circle.ownerEntity = npc;

    F32 npcRadius;
    npc->GetParameter("NavMeshRadius", &npcRadius, npc->GetMeshCircleRadius());

    my_circle.SetRadius(npcRadius);

    my_circle.SetPosition(npc->frame->mat.pos);

    currentThreatCircle = NULL;

    meshPosition->FindAnyValidAutoY(npc->frame->mat.pos, 0x400, false);

    if (meshPosition->GetMesh() != NULL) {
        meshPosition->GetMesh()->AddCircle(&my_circle);
    }

    reset_stuck();
    move_away = false;
    last_destination = &g_O3;
}

// UNUSED: needed to match (code from RotU GC)
U8 zNPCNavigate::PickRandomPos(const xVec3& centerPos, F32 innerRadius, F32 outerRadius, xVec3& pointOnMesh) const
{
    pointOnMesh.assign(2000.0f);

    if (!meshPosition->Valid()) {
        return 0;
    }

    return zMeshNavigationFindRandomPosition(meshPosition->GetMesh(), centerPos, innerRadius, outerRadius, pointOnMesh, 0);
}

bool zNPCNavigate::findChangedNavMesh()
{
    if (meshPosition->IsMeshEnabled()) {
        return false;
    }

    bool navMeshFound = meshPosition->FindAnyValidAutoY(npc->frame->mat.pos, 0x400, false);

    if (navMeshFound) {
        if (my_circle.GetMesh() != NULL) {
            my_circle.GetMesh()->RemoveCircle(&my_circle);
        }

        if (meshPosition->GetMesh() != NULL) {
            meshPosition->GetMesh()->AddCircle(&my_circle);
        }
    }

    return navMeshFound;
}

void zNPCNavigate::collide(F32 dt)
{
    stuck = false;
    xVec3 initialDpos = npc->frame->dpos;

    if (!meshPosition->Valid() || my_circle.GetMesh() == NULL) {
        return;
    }

    if (!meshPosition->IsMeshEnabled()) {
        if (!findChangedNavMesh()) {
            return;
        }
    }

    zMeshCircle* nearbyCircles = meshPosition->GetMesh()->GetNearbyCircles(npc->frame->mat.pos, my_circle.radius, 3.0f, npc, npc, 4);

    zMeshPosition updatedPosition = *meshPosition;

    bool result = updatedPosition.UpdatePositionAutoY(npc->frame->mat.pos, 0x400);

    if (!result) {
        npc->frame->dpos.assign(0.0f);
        return;
    }

    xVec2 dpos = { npc->frame->getDpos().x, npc->frame->getDpos().z };

    if (dpos.length2() > EPSILON) {
        if (!zMeshCollisionSweptCircleEnemies(updatedPosition, my_circle.radius, dpos, nearbyCircles)) {
            npc->frame->dpos.assign(0.0f);
            return;
        }

        F32 newDposLength = updatedPosition.GetPoint().Distance(npc->frame->mat.pos);

        if (newDposLength / dt < 1.0f) {
            stuck = true;
        }
    }

    if (!zMeshCollisionDepenetrateEnemies(updatedPosition, my_circle.radius, nearbyCircles, eBaseTypePlayer)) {
        npc->frame->dpos.assign(0.0f);
        stuck = true;
        return;
    }

    xFAIL_RETURN_IF_L(337, updatedPosition.Valid() == false);

    F32 dposOldY = npc->frame->getDpos().y;

    npc->frame->setDpos().Sub(updatedPosition.GetPoint(), npc->frame->mat.pos);

    if (npc->npcDrive.driver != NULL || npc->npcDrive.oldDriver != NULL || npc->inSlowEnemyCollisionVolume) {
        npc->frame->setDpos().y = dposOldY;
    }

    xVec3 endingDpos = npc->frame->dpos;
    F32 endingDposLength2 = endingDpos.length2();
    if (endingDposLength2 < 0.0001f && initialDpos.length2() > endingDposLength2) {
        stuck = true;
    }
}

bool zNPCNavigate::IsStuck() const
{
    return reported_stuck || !HasNavmesh();
}

bool zNPCNavigate::HasNavmesh() const
{
    return meshPosition->Valid();
}

void zNPCNavigate::apply_dpos(F32 dt)
{
    if (!HasNavmesh()) {
        return;
    }

    if (!meshPosition->IsMeshEnabled()) {
        if (!findChangedNavMesh()) {
            return;
        }
    }

    if (dpos_apply) {
        xVec3 newPos;
        newPos.Add(npc->frame->mat.pos, npc->frame->getDpos());

        zEntTrigger* isectrigger = zSpecialTriggersTree::IntersectingSphere(newPos, npc->bound.sph.r, 0x4, 0x400);

        if (isectrigger == NULL) {
            bool result = meshPosition->UpdatePositionAutoY(newPos, 0x400);

            if (result == true) {
                npc->frame->mat.pos = newPos;

                my_circle.SetPosition(newPos);
            }
        } else {
            if (!npc->manager.getCurrentBehavior()->TreatNoEnterAsCollision()) {
                bool result = meshPosition->UpdatePositionAutoY(newPos, 0x400);

                if (result == true) {
                    npc->frame->mat.pos = newPos;

                    my_circle.SetPosition(newPos);
                }
            }

            npc->manager.getCurrentBehavior()->NoEnterCollision(isectrigger);
        }
    }

    if (npc->pflags & k_XENT_HAS_GRAVITY) {
        npc->frame->vel.y = npc->frame->getDpos().y / dt;
    }

    npc->frame->setDpos(g_O3);

    dpos_apply = true;
}

void zNPCNavigate::remove()
{
    if (my_circle.mesh != NULL) {
        my_circle.mesh->RemoveCircle(&my_circle);
    }
}

void zNPCNavigate::GetDirection(const xVec3& destination, F32, xVec3& direction)
{
    xFAIL_RETURN_IF_L(741, meshPosition->Valid() == false);

    direction = npc->frame->mat.at;

    zMeshPosition destinationMeshPos = *meshPosition;

    bool result = destinationMeshPos.UpdatePositionAutoY(destination, 0x400);

    if (!result) {
        return;
    }

    zMeshNavigationGetDirection(*meshPosition, destinationMeshPos, my_circle.GetRadius(), direction);

    if (npc->perception.GetPriorityTargetDistance() >= STEERING_MIN_DIST2 && USE_STEERING) {
        xVec2 dir2D;
        dir2D.assign(direction.x, direction.z);
        currentThreatCircle = zMeshNavigationCommon::AvoidCircles(npc, meshPosition, my_circle, dir2D, dir2D);
        direction.assign(dir2D.x, 0.0f, dir2D.y);
    }
}

void zNPCNavigate::update_stuck(F32 dt)
{
    if (stuck) {
        stuck_timer += dt;
        if (stuck_timer >= 1.0f)
            reported_stuck = true;
    } else {
        reported_stuck = false;
        stuck_timer = 0.0f;
    }
}

void zNPCNavigate::InvalidateMeshPosition()
{
    meshPosition->Invalidate();
}
