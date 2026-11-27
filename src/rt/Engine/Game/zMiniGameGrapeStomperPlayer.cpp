#include "xCommon.h"
#include "zMiniGameGrapeStomperPlayer.h"

#include "xModel.h"
#include "xRand.h"
#include "xString.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zMiniGameGrapeStomper.h"
#include "zMiniGameManager.h"
#include "zScene.h"
#include "zShrapnel.h"
#include "zTextBox.h"
#include "zUIImage.h"

namespace
{
    const U32 LEFT_STOMP_SPLASH_IDS[4][3] = {
        { xStrHash("left_splash_blue"), xStrHash("left_splash_blue_2"), xStrHash("left_splash_blue_3") },
        { xStrHash("left_splash_red"), xStrHash("left_splash_red_2"), xStrHash("left_splash_red_3") }
    };
    const U32 RIGHT_STOMP_SPLASH_IDS[4][3] = {
        { xStrHash("right_splash_blue"), xStrHash("right_splash_blue_2"), xStrHash("right_splash_blue_3") },
        { xStrHash("right_splash_red"), xStrHash("right_splash_red_2"), xStrHash("right_splash_red_3") }
    };
    const U32 modelIDs[3] = { xStrHash("remy_stand_stomp"), xStrHash("remy_left_stomp"), xStrHash("remy_right_stomp") };
}

class InputChecker
{
protected:
    zPlayerInput* playerInput;
    bool cycleDone;

public:
    virtual void Init(zPlayerInput* pi)
    {
        playerInput = pi;
        Reset();
    }

    virtual bool CycleDone() { return cycleDone; }

    virtual void Update(F32 dt) = 0;

    virtual void Reset()
    {
        cycleDone = false;
    }
};

class ActionMashChecker : public InputChecker
{
protected:
    bool pressed;

public:
    virtual void Update(F32)
    {
        if (cycleDone) {
            Reset();
        }

        if (playerInput->pressed.jump)
            pressed = true;
        if (playerInput->released.jump && pressed)
            cycleDone = true;
    }

    virtual void Reset()
    {
        InputChecker::Reset();
        pressed = false;
    }
};

class StickWaggleChecker : public InputChecker
{
protected:
    bool hitLeft;
    bool hitRight;
    bool prevStartLeft;
    bool prevStartRight;
    F32 timer;

public:
    virtual void Update(F32 dt)
    {
        timer += dt;
        if (timer > 1.0f) {
            prevStartLeft = prevStartRight = false;
            timer = 0.0f;
        }

        if (cycleDone) {
            Reset();
        }

        F32 x, y, angle, mag;
        playerInput->GetAnalog1(x, y, angle, mag);

        if (xabs(1.0f - x) < 0.01f) {
            if (!prevStartRight || hitRight) {
                hitLeft = true;
                if (!prevStartRight) {
                    prevStartLeft = true;
                }
            }
            timer = 0.0f;
        }

        if (xabs(-1.0f - x) < 0.01f) {
            if (!prevStartLeft || hitLeft) {
                hitRight = true;
                if (!prevStartLeft) {
                    prevStartRight = true;
                }
            }
            timer = 0.0f;
        }

        if (hitLeft || hitRight) {
            timer = 0.0f;
        }

        if (hitLeft && hitRight)
            cycleDone = true;
    }

    virtual void Reset()
    {
        InputChecker::Reset();
        hitLeft = false;
        hitRight = false;
    }
};

class StickRotateChecker : public InputChecker
{
protected:
    S32 prevDir;
    S32 hitCount;
    F32 timer;

public:
    virtual void Update(F32 dt)
    {
        timer += dt;
        if (timer > 2.0f) {
            Reset();
        }

        if (cycleDone) {
            Reset();
        }

        F32 x, y, angle, mag;
        playerInput->GetAnalog1(x, y, angle, mag);

        S32 curDir = -1;
        if (xabs(1.0f - x) < 0.01f)
            curDir = 3;
        else if (xabs(-1.0f - x) < 0.01f)
            curDir = 1;
        else if (xabs(-1.0f - y) < 0.01f)
            curDir = 2;
        else if (xabs(1.0f - y) < 0.01f)
            curDir = 0;

        if (curDir == -1 || curDir == prevDir) {
            return;
        }

        if ((prevDir == -1 && hitCount == 0) || curDir == (prevDir + 1) % 4)
            hitCount++;
        else
            hitCount = 0;

        prevDir = curDir;

        if (hitCount == 5)
            cycleDone = true;
    }

    virtual void Reset()
    {
        InputChecker::Reset();
        prevDir = -1;
        timer = 0.0f;
        hitCount = 0;
    }
};

void zMiniGameGrapeStomperPlayer::Init(xEntAsset* asset)
{
    zMiniGamePlayer::Init(asset);
    U32 control = ((zMiniGameGrapeStomper*)globals.mgManager->GetCurrentGame())->GetAsset()->controlType;
    switch (control) {
    case 0:
        inputChecker = new (xMEMALLOC_L(sizeof(ActionMashChecker), 0, eMemMgrTag_MiniGame, 0, 265)) ActionMashChecker;
        break;
    case 1:
        inputChecker = new (xMEMALLOC_L(sizeof(StickWaggleChecker), 0, eMemMgrTag_MiniGame, 0, 268)) StickWaggleChecker;
        break;
    case 2:
        inputChecker = new (xMEMALLOC_L(sizeof(StickRotateChecker), 0, eMemMgrTag_MiniGame, 0, 271)) StickRotateChecker;
        break;
    }

    if (inputChecker) {
        inputChecker->Init(&playerInput);
    }

    for (S32 i = 0; i < 3; i++) {
        RpAtomic* file = xModelFindAtomic(modelIDs[i], NULL);
        xASSERT_FMT_L(281, file, "Can't find model for player.");
        if (!file) {
            return;
        }

        models[i] = xModelInstanceAlloc(file, NULL, 0, 0, NULL);
        xModelInstanceSetIDSafe_L(286, models[i], modelIDs[i]);
        models[i]->Scale.assign(2.0f);
    }
}

void zMiniGameGrapeStomperPlayer::BeginUpdate(F32 dt)
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

void zMiniGameGrapeStomperPlayer::Update(F32 dt)
{
    if (!globals.mgManager->GetCurrentGame()->ShouldUpdatePlayer()) {
        return;
    }

    Update_Control();
    inputChecker->Update(dt);
    curStomp = 0;

    if (inputChecker->CycleDone()) {
        score++;
        if (prevStomp == 1)
            curStomp = 2;
        else
            curStomp = 1;
        prevStomp = curStomp;
        frameCount = 4;
        whichModel = curStomp;
    }

    if (curStomp == 1) {
        xAnimPlaySetState(model->Anim->Single, xAnimTableGetState(model->Anim->Table, "StompL01"), 0.0f);
        xVec3 vel = { 0.0f, 0.0f, 0.0f };
        // BUG: 0..3 inclusive, reads past the 3 splashes
        S32 randomIndex = xrand_RandomRange(0, 3);
        if (leftSplash[randomIndex])
            leftSplash[randomIndex]->initCB(leftSplash[randomIndex], model, NULL, &vel, NULL);
    }
    else if (curStomp == 2) {
        xAnimPlaySetState(model->Anim->Single, xAnimTableGetState(model->Anim->Table, "StompR01"), 0.0f);
        xVec3 vel = { 0.0f, 0.0f, 0.0f };
        // BUG: 0..3 inclusive, reads past the 3 splashes
        S32 randomIndex = xrand_RandomRange(0, 3);
        if (rightSplash[randomIndex]) {
            rightSplash[randomIndex]->initCB(rightSplash[randomIndex], model, NULL, &vel, NULL);
        }
    }

    frameCount--;
    if (frameCount <= 0)
        whichModel = curStomp;
}

void zMiniGameGrapeStomperPlayer::EndUpdate(F32 dt)
{
    zPlayer::DefaultEndUpdate(dt);
}

void zMiniGameGrapeStomperPlayer::Render()
{
    if (!globals.mgManager->GetCurrentGame()->ShouldRenderPlayer()) {
        return;
    }

    xEntRender(this);
    xMat4x3Copy((xMat4x3*)models[whichModel]->Mat, &frame->mat);
    models[whichModel]->Mat->pos.y += 1.0f;
}

void zMiniGameGrapeStomperPlayer::SceneSetup()
{
    zPlayer::SceneSetup();
    for (S32 i = 0; i < 3; i++) {
        leftSplash[i] = (zShrapnelAsset*)xSTFindAsset(LEFT_STOMP_SPLASH_IDS[index][i], NULL);
        rightSplash[i] = (zShrapnelAsset*)xSTFindAsset(RIGHT_STOMP_SPLASH_IDS[index][i], NULL);
    }
    Reset();
}

void zMiniGameGrapeStomperPlayer::Reset()
{
    zMiniGamePlayer::Reset();
    score = 0;
    prevStomp = 0;
    frameCount = 0;
    whichModel = 0;
    xAnimPlaySetState(model->Anim->Single, xAnimTableGetState(model->Anim->Table, "Idle01"), 0.0f);
}

void zMiniGameGrapeStomperPlayer::Exit()
{
    zPlayer::Exit();
}

void zMiniGameGrapeStomperPlayer::AddStates(xAnimTable* table)
{
    xAnimTableNewState(table, "Idle01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "Win01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "Lose01", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "StompL01", AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);
    xAnimTableNewState(table, "StompR01", AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, NULL, NULL, NULL);

    xAnimTableNewTransition(table, "StompL01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "StompR01", "Idle01", NULL, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
}
