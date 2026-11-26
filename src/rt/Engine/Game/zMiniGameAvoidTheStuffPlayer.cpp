#include "xCommon.h"
#include "zMiniGameAvoidTheStuffPlayer.h"

#include "xColor.h"
#include "xDraw.h"
#include "zCam.h"
#include "zGlobals.h"
#include "zMiniGameBase.h"
#include "zScene.h"

namespace
{
    xTWEAKABLE F32 PLAYER_TURN_SPEED = 2.0f * PI;
}

void zAvoidTheStuffPlayerMove::AddStates(xAnimTable* table)
{
    NewState(table, "Idle01", AnimState_EndLoop, 0);
    NewState(table, "Jump01", AnimState_EndTransition, 0);
    NewState(table, "Duck01", AnimState_EndTransition, 0);
    NewState(table, "BlockLeft01", AnimState_EndTransition, 0);
    NewState(table, "BlockRight01", AnimState_EndTransition, 0);

    xAnimTableNewTransition(table, "Jump01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Duck01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "BlockLeft01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "BlockRight01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
}

void zAvoidTheStuffPlayerMove::AddInternalTransitions(xAnimTable* table)
{
    xAnimTableNewTransition(table, "Idle01", "Jump01", anStartJumpCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle01", "Duck01", anStartDuckCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle01", "BlockLeft01", anStartBlockLeftCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle01", "BlockRight01", anStartBlockRightCheck, NULL, 0, 0, 0.0f, 0.0f, 1000, 0, 0.15f, NULL);
}

void zAvoidTheStuffPlayerMove::Reset()
{
    xAnimPlaySetState(player->model->Anim->Single, xAnimTableGetState(player->model->Anim->Table, "Idle01"), 0.0f);
    manager->SetCurrentAction(this);
}

U32 zAvoidTheStuffPlayerMove::StartJumpCheck(xAnimTransition*, xAnimSingle*)
{
    return 0;
}

U32 zAvoidTheStuffPlayerMove::StartDuckCheck(xAnimTransition*, xAnimSingle*)
{
    return 0;
}

U32 zAvoidTheStuffPlayerMove::StartBlockLeftCheck(xAnimTransition*, xAnimSingle*)
{
    return 0;
}

U32 zAvoidTheStuffPlayerMove::StartBlockRightCheck(xAnimTransition*, xAnimSingle*)
{
    return 0;
}

void zMiniGameAvoidTheStuffPlayer::Init(xEntAsset* asset)
{
    zMiniGamePlayer::Init(asset);
}

void zMiniGameAvoidTheStuffPlayer::BeginUpdate(F32 dt)
{
    if (dt < EPSILON) {
        return;
    }

    if (GetControlOff()) {
        playerInput.Clear();
    } else {
        playerInput.Update(dt);
    }

    zEnt* ent = this;
    last_update_dt = update_dt;
    update_dt = dt;
    lastmat = ent->frame->mat;

    xEntBeginUpdate(ent, globals.sceneCur, dt);
}

void zMiniGameAvoidTheStuffPlayer::UpdateOrientation(F32 dt)
{
    xVec3 stickVec, worldStickVec;
    F32 angle, mag;
    GetPlayerInput().GetAnalog1(stickVec.x, stickVec.y, angle, mag);
    if (xabs(stickVec.x) >= 0.1f || xabs(stickVec.y) >= 0.1f) {
        stickVec.z = 0.0f;

        const xMat4x3& camMat = zCamGetMatrix();
        xMat3x3RMulVec(&worldStickVec, &camMat, &stickVec);

        worldStickVec.y = 0.0f;
        worldStickVec.safe_normalize(xVec3::m_Null);
        F32 angleDiff = GetAngleDiff(&frame->mat.at, &worldStickVec);
        ApplyAngleDiff(&frame->mat, angleDiff, dt, PLAYER_TURN_SPEED);
    }
}

void zMiniGameAvoidTheStuffPlayer::Update(F32 dt)
{
    Update_Control();
    UpdateOrientation(dt);
}

void zMiniGameAvoidTheStuffPlayer::EndUpdate(F32 dt)
{
    zPlayer::DefaultEndUpdate(dt);
}

void zMiniGameAvoidTheStuffPlayer::Render()
{
    xEntRender(this);
    xDrawSetColor(g_RED);
    xDrawArrow(&frame->mat.pos, &frame->mat.at, true, 3.0f);
}

void zMiniGameAvoidTheStuffPlayer::SceneSetup()
{
    zMiniGamePlayer::SceneSetup();
    Reset();
}

void zMiniGameAvoidTheStuffPlayer::Reset()
{
    zMiniGamePlayer::Reset();
}

void zMiniGameAvoidTheStuffPlayer::Exit()
{
    zPlayer::Exit();
}

void zMiniGameAvoidTheStuffPlayer::InitActions()
{
    actionManager.Init(1);
    actionManager.Add(this, new (eMemStaticType_0, eMemMgrTag_Player) zAvoidTheStuffPlayerMove);
}
