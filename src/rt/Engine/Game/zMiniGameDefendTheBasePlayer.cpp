#include "xCommon.h"
#include "zMiniGameDefendTheBasePlayer.h"

#include "xCollideSweptSphere.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xModel.h"
#include "xString.h"
#include "zCam.h"
#include "zGlobals.h"
#include "zMiniGameDefendTheBase.h"
#include "zMiniGameManager.h"
#include "zScene.h"

#include <stdio.h>

namespace
{
    xTWEAKABLE F32 SHOT_SPEED = 12.0f;
    xTWEAKABLE F32 PLAYER_TURN_SPEED = 2.0f * PI;
    const U32 SHOT_MODEL_ID = xStrHash("temp_projectile_salt");
}

void zMiniGameDefendTheBasePlayerShot::Init(void* data)
{
    owner = (zMiniGameDefendTheBasePlayer*)data;

    RpAtomic* file = xModelFindAtomic(SHOT_MODEL_ID, NULL);
    xASSERT_FMT_L(31, file, "Can't find model for player shot");
    if (!file) {
        return;
    }

    model = xModelInstanceAlloc(file, NULL, 0, 0, NULL);
    xModelInstanceSetIDSafe_L(36, model, SHOT_MODEL_ID);
    active = false;
}

void zMiniGameDefendTheBasePlayerShot::Update(F32 dt)
{
    if (!active) {
        return;
    }

    xSweptSphere sws;
    xVec3 start = mat.pos;

    xVec3 end = start;
    end.AddScale(mat.at, SHOT_SPEED * dt);
    xSweptSpherePrepare(&sws, &start, &end, GetRadius(), 1);

    zMiniGameDefendTheBaseEnemy* enemy = ((zMiniGameDefendTheBase*)globals.mgManager->GetCurrentGame())->GetActiveEnemyList();
    while (enemy) {
        if (xSweptSphereToSphere(sws, enemy->GetPosition(), enemy->GetRadius())) {
            Kill();
            enemy->Kill();
            owner->RegisterEnemyKill();
            return;
        }

        enemy = (zMiniGameDefendTheBaseEnemy*)enemy->next;
    }

    S32 result = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
    if (result) {
        Kill();
        return;
    }

    mat.pos = end;
}

void zMiniGameDefendTheBasePlayerShot::Render()
{
    if (!active) {
        return;
    }

    xMat4x3Copy((xMat4x3*)model->Mat, &mat);
    xModelRender(model);
    xDrawSetColor(g_RED);
    xDrawSphere(&mat.pos, GetRadius(), 12);
}

void zMiniGameDefendTheBasePlayerShot::Emit()
{
    mat = owner->frame->mat;
    active = true;
}

void zMiniGameDefendTheBasePlayerShot::Exit()
{
    xModelInstanceFree_L(model, true, 96);
    model = NULL;
}

F32 zMiniGameDefendTheBasePlayerShot::GetRadius()
{
    if (model) {
        return xModelGetLocalSBound(model)->r;
    } else {
        return 0.0f;
    }
}

void zMiniGameDefendTheBasePlayer::Init(xEntAsset* asset)
{
    zPlayer::Init(asset);
    coolDown = ((zMiniGameDefendTheBase*)globals.mgManager->GetCurrentGame())->GetPlayerCoolDown();
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        shots[i].Init(this);
    }
}

void zMiniGameDefendTheBasePlayer::BeginUpdate(F32 dt)
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

void zMiniGameDefendTheBasePlayer::UpdateOrientation(F32 dt)
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

void zMiniGameDefendTheBasePlayer::Update(F32 dt)
{
    coolDownTimer -= dt;

    Update_Control();
    UpdateOrientation(dt);
    if (IsActionButtonPressed() && coolDownTimer <= 0.0f) {
        for (S32 j = 0; j < MAX_PLAYER_SHOTS; j++) {
            if (!shots[j].IsActive()) {
                shots[j].Emit();
                coolDownTimer = coolDown;
                break;
            }
        }
    }

    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        if (shots[i].IsActive())
            shots[i].Update(dt);
    }
}

void zMiniGameDefendTheBasePlayer::EndUpdate(F32 dt)
{
    zPlayer::DefaultEndUpdate(dt);
}

void zMiniGameDefendTheBasePlayer::Render()
{
    if (globals.mgManager->GetCurrentGame()->IsGameDone()) {
        return;
    }

    xEntRender(this);
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        if (shots[i].IsActive())
            shots[i].Render();
    }

    xDrawSetColor(g_RED);
    xDrawArrow(&frame->mat.pos, &frame->mat.at, true, 3.0f);
}

void zMiniGameDefendTheBasePlayer::SceneSetup()
{
    zPlayer::SceneSetup();
    Reset();
}

void zMiniGameDefendTheBasePlayer::Reset()
{
    zPlayer::Reset();
    currentBaseHP = maxBaseHP;
}

void zMiniGameDefendTheBasePlayer::Exit()
{
    zPlayer::Exit();
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        shots[i].Exit();
    }
}

#if DEBUG || RELEASE
void zMiniGameDefendTheBasePlayer::SetupTweaks()
{
    char* prefix = ZMINIGAME_TYPE_PREFIX;
    char buf[128];

    sprintf(buf, "%s|Defend the Base|Shot Speed", prefix);
    xTWEAK(buf, &SHOT_SPEED, 0.0f, 100.0f, NULL, NULL, 0x2);
    sprintf(buf, "%s|Defend the Base|Player Turn Speed", prefix);
    xTWEAK(buf, &PLAYER_TURN_SPEED, 0.0f, 2.0f * PI, NULL, NULL, 0x2);
}
#endif
