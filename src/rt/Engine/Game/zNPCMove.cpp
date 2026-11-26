#include "xCommon.h"
#include "zNPCMove.h"

#include "xDraw.h"
#include "xEntMotion.h"
#include "xEvent.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xNavigationMeshAsset.h"
#include "xutil.h"
#include "zCombat.h"
#include "zCommonPlayer.h"
#include "zEvent.h"
#include "zMeshCollision.h"
#include "zMeshPosition.h"
#include "zNPCCommon.h"
#include "zNPCNavigation.h"
#include "zNPCNavigationMesh.h"
#include "zScene.h"

void zNPCMove::setup()
{
    if (!owner->navigation_behavior) {
        owner->navigation_behavior = new (xMEMALLOC_L(sizeof(zNPCNavigate), 0, eMemMgrTag_Enemy, IDTAG('N', 'B', 'A', 'S'), 21)) zNPCNavigate(owner);
    }

#if DEBUG || RELEASE
    static bool _static_setup = zNPCNavigationMesh::static_setup();
#endif

    F32 moveAngle;
    owner->GetParameter("MinMoveAngle", &moveAngle, 60.0f);
    cos_min_move_angle = icos((PI / 180.0f) * moveAngle);
    zNPC::move::setup();
}

bool zNPCMove::systemEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventJumpTo:
        xASSERT_FMT_L(40, toParamWidget, "Need a widget to jump to");
        return true;
    }

    return behavior::systemEvent(from, to, toEvent, toParam, toParamWidget, toParamWidgetID);
}

bool zNPCMove::has_mesh()
{
    return owner->navigation_behavior && owner->navigation_behavior->HasNavmesh();
}

// UNUSED: needed to match (speculative code)
void zNPCMove::check_movement(F32 dt)
{
    if (!has_mesh()) {
        return;
    }

    xVec3 position = owner->frame->mat.pos, dpos = owner->frame->getDpos();
    move_to(owner->navigation_behavior->get_destination(), dt, owner->navigation_behavior->get_move_away(), false, false, 0.4f * 0.4f);
    owner->navigation_behavior->dont_apply_dpos();
}

void zNPCMove::jump_to(xVec3&, F32, bool)
{
}

#if DEBUG || RELEASE
bool draw_move_to;
#endif

void zNPCMove::move_to(const xVec3& destination, F32 dt, bool move_away, bool face_dir, bool change_anim_speed, F32 arrived_threshold2)
{
    zMeshPosition* curMeshPos = &owner->navigation_behavior->GetMeshPosition();

    xASSERT_FMT_L(81, curMeshPos->Valid(), "NPC %s has an invalid navmesh", xSceneID2Name(xglobals->sceneCur, owner->id));

    bool goodMeshPos = true;
    if (!curMeshPos->Valid()) {
        goodMeshPos = false;
        curMeshPos->FindAnyValidAutoY(owner->frame->mat.pos, 0x400, true);
        if (curMeshPos->Valid()) {
            zMeshCircle* myCircle = owner->navigation_behavior->get_circle();
            zMeshCircle* nearbyCircles = curMeshPos->GetMesh()->GetNearbyCircles(curMeshPos->GetPoint(), myCircle->radius, 2.0f, owner, NULL, 0);

            zMeshCollisionDepenetrateEnemies(*curMeshPos, myCircle->radius, nearbyCircles, 0);

            F32 distanceToPoint2 = owner->frame->mat.pos.Distance2(curMeshPos->GetPoint());

            if (curMeshPos->Valid() && distanceToPoint2 < myCircle->radius2) {
                owner->frame->mat.pos = curMeshPos->GetPoint();
                goodMeshPos = true;
            }
        }
    }

    xASSERT_FMT_L(108, goodMeshPos, "Holy crap! We couldn't fix our invalid navmesh, now we're gonna blow up for no reason.");

    if (!goodMeshPos) {
        zNPC::zNPCCommon* npc = owner;
        xVec3 npcPos = npc->frame->mat.pos;
        zCombat* npcCombat = npc->get_combat();

        zCombatDamageInfo dmInfo;
        dmInfo.from = NULL;
        dmInfo.damage = npcCombat->currentHitPoints;
        dmInfo.source = zHS_GENERAL;
        dmInfo.target = zHT_GENERAL;
        dmInfo.knockback = xVec3::m_Null;
        dmInfo.hitLocation = xVec3::m_Null;
        dmInfo.direction = xVec3::m_Null;

        npc->manager.damage(dmInfo);
        npc->deliver_damage(dmInfo);
        return;
    }

    xASSERT_FMT_L(131, has_mesh(), "How on earth did we get here with a bad mesh?");

    if (!has_mesh()) {
        return;
    }

#if DEBUG || RELEASE
    if (draw_move_to) {
        xDrawSetColor(0, 0x80, 0, 0xFF);
        xDrawLine(&destination, xEntGetPos(owner));
    }
#endif

    xVec3 to_destination;
    to_destination.Sub(destination, owner->frame->mat.pos);

    if (xabs(to_destination.y) < 3.0f)
        to_destination.y = 0.0f;
    F32 distance2 = to_destination.length2();

    xVec3 desired_facing;
    if (distance2 <= arrived_threshold2) {
        owner->navigation_behavior->set_arrived(true);
        xVec3Copy(&desired_facing, &owner->frame->mat.at);
        zEntEvent(owner, eEventNPCHasArrived);
    } else {
        owner->navigation_behavior->set_arrived(false);
        owner->navigation_behavior->GetDirection(destination, speed, desired_facing);
    }

    if (face_dir) {
        turn_to_face(&desired_facing, dt, -1.0f);
    }

    owner->navigation_behavior->set_destination(destination);
    owner->navigation_behavior->set_move_away(move_away);

    if (face_dir) {
        if (owner->frame->mat.at.dot(desired_facing) < cos_min_move_angle || (!move_away && have_arrived())) {
            stop();
        }
        else if (move_away) {
            xVec3 backward;
            backward.Negate(owner->frame->mat.at);
            accelerate(dt, backward, change_anim_speed, -1.0f);
        } else {
            accelerate(dt, owner->frame->mat.at, change_anim_speed, -1.0f);
        }
    } else {
        if (have_arrived()) {
            stop();
        } else {
            accelerate(dt, desired_facing, change_anim_speed, -1.0f);
        }
    }

    if (is_stuck())
        zEntEvent(owner, eEventNPCIsStuck);
}

bool zNPCMove::physics_move(F32 dt)
{
    xVec3 target_dpos = owner->frame->dpos;
    F32 target_length = target_dpos.length();
    owner->navigation_behavior->collide(dt);

    bool succeeded = true;
    const xVec3& actual_dpos = owner->frame->dpos;
    F32 actual_length = actual_dpos.length();
    if (actual_length < 0.95f * target_length) {
        succeeded = false;
    }
    else if (!xfeq0(actual_length) && !xfeq0(target_length)) {
        xVec2 target = { target_dpos.x, target_dpos.z }, actual = { actual_dpos.x, actual_dpos.z };
        target.normalize();
        actual.normalize();
        succeeded = target.dot(actual) > 0.95f;
    }

    owner->navigation_behavior->apply_dpos(dt);
    owner->navigation_behavior->update_stuck(dt);
    return succeeded;
}

void zNPCMove::move_away_from(const xVec3& point, F32 dt)
{
    move_to(point, dt, true, true, true, 0.4f * 0.4f);
}

bool zNPCMove::is_stuck() const
{
    if (owner->navigation_behavior == NULL) {
        return false;
    }

    return owner->navigation_behavior->IsStuck();
}

bool zNPCMove::have_arrived() const
{
    if (owner->navigation_behavior == NULL) {
        return false;
    }

    return owner->navigation_behavior->have_arrived();
}

void zNPCMove::step_move(const xVec3& direction, F32 distance, F32 dt)
{
    F32 move_dist = MIN(dt * MIN(move_speed, acceleration * dt + speed), distance);

    xVec3 pos;
    pos.AddScale(owner->frame->mat.pos, direction, 0.404f + move_dist);
    move_to(pos, dt, false, false, true, 0.4f * 0.4f);
}
