#include "xCommon.h"
#include "zMiniGamePufferoidsPlayer.h"

#include "xCollideSweptSphere.h"
#include "xColor.h"
#include "xDraw.h"
#include "xModel.h"
#include "xString.h"
#include "zCam.h"
#include "zGlobals.h"
#include "zMiniGameManager.h"
#include "zMiniGamePufferoids.h"
#include "zScene.h"
#include "zTextBox.h"
#include "zUIImage.h"

#define MAX_PLAYER_SHOTS 50

namespace
{
    xTWEAKABLE F32 PLAYER_TURN_SPEED = 2.0f * PI;
    xTWEAKABLE F32 PLAYER_MOVE_SPEED = 10.0f;
    const U32 SHOT_MODEL_ID = xStrHash("puff_cream_projectile");
    const U32 SHOT_LONG_MODEL_ID = xStrHash("puff_cream_projectile_six");
}

class zMiniGamePufferoidsShot
{
protected:
    xModelInstance* model;
    xModelInstance* longModel;
    xModelInstance* shortModel;
    xMat4x3 mat;
    S32 power;
    F32 range;
    F32 currRange;
    zMiniGamePufferoidsPlayer* owner;
    bool active;

public:
    void Init(zMiniGamePufferoidsPlayer* owner);
    void Update(F32 dt);
    void Render();
    void Fire(bool longRange, S32 power, F32 yRot);
    void Kill();
    void Exit();

    bool IsActive() { return active; }

    F32 GetRadius()
    {
        return power > 1 ? 0.5f : 0.3f;
    }
};

void zMiniGamePufferoidsShot::Init(zMiniGamePufferoidsPlayer* owner)
{
    this->owner = owner;

    RpAtomic* data = xModelFindAtomic(SHOT_MODEL_ID, NULL);
    xASSERT_FMT_L(62, data, "Can't find model for shot!  Will crash later!");
    if (!data) {
        return;
    }

    shortModel = xModelInstanceAlloc(data, NULL, 0, 0, NULL);
    xModelInstanceSetIDSafe_L(67, shortModel, SHOT_MODEL_ID);

    data = xModelFindAtomic(SHOT_LONG_MODEL_ID, NULL);
    xASSERT_FMT_L(70, data, "Can't find model for shot!  Will crash later!");
    if (!data) {
        return;
    }

    longModel = xModelInstanceAlloc(data, NULL, 0, 0, NULL);
    xModelInstanceSetIDSafe_L(75, longModel, SHOT_LONG_MODEL_ID);

    active = false;
}

void zMiniGamePufferoidsShot::Update(F32 dt)
{
    if (!active) {
        return;
    }

    F32 disp = 14.0f * dt;
    currRange += disp;
    if (currRange >= range) {
        Kill();
        return;
    }

    xSweptSphere sws;
    xVec3 start = mat.pos;

    xVec3 end = start;
    end.AddScale(mat.at, disp);
    xSweptSpherePrepare(&sws, &start, &end, GetRadius(), 1);

    S32 result = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
    if (result) {
        Kill();
        return;
    }

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        if (CURPLAYERS[i] == owner) {
            continue;
        }

        if (xSweptSphereToSphere(&sws, &CURPLAYERS[i]->bound.sph)) {
            Kill();
            return;
        }
    }

    zMiniGamePufferoidsPuff* puffs = ((zMiniGamePufferoids*)globals.mgManager->GetCurrentGame())->GetActivePuffs();
    while (puffs) {
        xSphere sph;
        puffs->GetSphere(sph);
        if (xSweptSphereToSphere(&sws, &sph)) {
            puffs->ProjectileHit(owner->index, power);
            Kill();
            return;
        }

        puffs = puffs->next;
    }

    mat.pos.AddScale(mat.at, disp);
}

void zMiniGamePufferoidsShot::Render()
{
    if (!active) {
        return;
    }

    xMat4x3Copy((xMat4x3*)model->Mat, &mat);
    xMat3x3LMulRotY((xMat3x3*)model->Mat, (xMat3x3*)model->Mat, PI / 2);

    if (power > 1) {
        xDrawSetColor(g_RED);
        xDrawSphere(&mat.pos, 0.8f, 12);
    } else {
        xDrawSetColor(g_BLUE);
        xDrawSphere(&mat.pos, 0.5f, 12);
    }
}

void zMiniGamePufferoidsShot::Fire(bool longRange, S32 power, F32 yRot)
{
    if (active) {
        return;
    }

    if (longRange) {
        model = longModel;
        range = 20.0f;
    } else {
        model = shortModel;
        range = 12.0f;
    }

    currRange = 0.0f;
    this->power = power;

    xMat4x3Copy(&mat, &owner->frame->mat);
    xMat3x3LMulRotY(&mat, &mat, yRot);
    active = true;
}

void zMiniGamePufferoidsShot::Kill()
{
    active = false;
}

void zMiniGamePufferoidsShot::Exit()
{
    if (shortModel) {
        xModelInstanceFree_L(shortModel, true, 191);
        shortModel = NULL;
    }

    if (longModel) {
        xModelInstanceFree_L(longModel, true, 197);
        longModel = NULL;
    }
}

void zMiniGamePufferoidsPlayer::Init(xEntAsset* asset)
{
    zMiniGamePlayer::Init(asset);
    shots = (zMiniGamePufferoidsShot*)xMEMALLOC_L(MAX_PLAYER_SHOTS * sizeof(zMiniGamePufferoidsShot), 0, eMemMgrTag_MiniGame, 0, 208);
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++)
        shots[i].Init(this);
    bound.sph.r = model->Data->boundingSphere.radius;
    bound.sph.center = *(xVec3*)&model->Mat->pos;
}

void zMiniGamePufferoidsPlayer::BeginUpdate(F32 dt)
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

void zMiniGamePufferoidsPlayer::UpdateOrientation(F32 dt)
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

void zMiniGamePufferoidsPlayer::UpdatePosition(F32 dt)
{
    F32 x, y, angle, mag;
    GetPlayerInput().GetAnalog1(x, y, angle, mag);

    xVec3 moveVec;
    F32 moveMag = mag * mag * PLAYER_MOVE_SPEED * dt;
    moveVec.Scale(frame->mat.at, moveMag);

    xSweptSphere sws;
    xVec3 start = frame->mat.pos;

    xVec3 end = start;
    end.Add(moveVec);
    xSweptSpherePrepare(&sws, &start, &end, bound.sph.r, 1);

    S32 result = xSweptSphereToEnv(&sws, xglobals->sceneCur->env);
    if (result) {
        xSweptSphereGetResults(&sws);

        xVec3 slideDir = sws.worldTangent;
        slideDir.safe_normalize(xVec3::m_Null);

        xVec3 leftToMove;
        leftToMove.Sub(end, sws.worldPos);
        F32 slideMag = leftToMove.dot(slideDir);

        frame->mat.pos.AddScale(sws.worldPos, slideDir, slideMag);
    } else
        frame->mat.pos.Add(moveVec);
}

void zMiniGamePufferoidsPlayer::ApplyPickup(S32 applyPickup, F32 activeTime)
{
    if (activePickupType != -1) {
        return;
    }

    switch (applyPickup) {
    case 0:
        xASSERT_L(308, activeTime <= 0.f);
        lives++;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        xASSERT_L(317, activeTime > 0.f);
        activePickupType = applyPickup;
        activePickupTimer = activeTime;
        break;
    default:
        xASSERT_L(323, !"Unknown pickup type");
    }
}

void zMiniGamePufferoidsPlayer::HitByPuff()
{
    if (GetActivePickup() == 4) {
        return;
    }

    if (state != 0) {
        return;
    }

    lives--;
    if (lives > 0) {
        fellTimer = 3.0f;
        state = 1;
        xMat3x3Copy(&restoreMat, &frame->mat);
        trip = true;
    } else
        state = 3;
}

zMiniGamePufferoidsShot* zMiniGamePufferoidsPlayer::GetFreeShot()
{
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        if (!shots[i].IsActive())
            return &shots[i];
    }

    return NULL;
}

void zMiniGamePufferoidsPlayer::Update(F32 dt)
{
    if (globals.mgManager->GetCurrentGame()->IsGameDone()) {
        return;
    }

    zMiniGamePlayer::Update(dt);

    switch (state) {
    case 2:
        invincibleTimer -= dt;
        if (invincibleTimer <= 0.0f) {
            state = 0;
        }
    case 0:
        Update_Control();
        UpdateOrientation(dt);
        UpdatePosition(dt);
        if (IsActionButtonPressed()) {
            S32 power = GetActivePickup() == 3 ? 2 : 1;
            bool longRange = GetActivePickup() == 1;
            S32 numShots = GetActivePickup() == 2 ? 3 : 1;

            for (S32 j = 0; j < numShots; j++) {
                zMiniGamePufferoidsShot* freeShot = GetFreeShot();
                if (freeShot) {
                    freeShot->Fire(longRange, power, (PI / 4) * ((j + 1) / 2) * (j % 2 == 0 ? 1.0f : -1.0f));
                }
            }
        }

        for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
            if (shots[i].IsActive())
                shots[i].Update(dt);
        }
        break;
    case 1:
        if (trip) {
            xMat3x3LMulRotX(&frame->mat, &frame->mat, PI / 2);
            trip = false;
        }

        fellTimer -= dt;
        if (fellTimer <= 0.0f) {
            state = 2;
            invincibleTimer = 2.0f;
            skipRender = false;
            skipCounter = 0;
            xMat3x3Copy(&frame->mat, &restoreMat);
        }
        break;
    case 3:
        return;
    default: break;
    }
}

void zMiniGamePufferoidsPlayer::EndUpdate(F32 dt)
{
    zPlayer::DefaultEndUpdate(dt);
}

void zMiniGamePufferoidsPlayer::Render()
{
    if (globals.mgManager->GetCurrentGame()->IsGameDone()) {
        return;
    }

    switch (state) {
    case 2:
        skipCounter++;
        if (skipCounter >= 3) {
            skipCounter = 0;
            skipRender = !skipRender;
        }

        if (skipRender) {
            break;
        }
    case 0:
    case 1:
        xEntRender(this);
        break;
    case 3:
        return;
    }

    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++) {
        if (shots[i].IsActive())
            shots[i].Render();
    }
}

void zMiniGamePufferoidsPlayer::SceneSetup()
{
    zMiniGamePlayer::SceneSetup();
    Reset();
}

void zMiniGamePufferoidsPlayer::Reset()
{
    zMiniGamePlayer::Reset();
    invincibleTimer = 0.0f;
    score = 0;
    state = 0;
}

void zMiniGamePufferoidsPlayer::Exit()
{
    zPlayer::Exit();
    for (S32 i = 0; i < MAX_PLAYER_SHOTS; i++)
        shots[i].Exit();
}

void zMiniGamePufferoidsPlayer::AddStates(xAnimTable* table)
{
    xAnimTableNewState(table, "Idle01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "Walk01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
}
