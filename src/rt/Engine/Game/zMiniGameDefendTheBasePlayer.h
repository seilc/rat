#ifndef ZMINIGAMEDEFENDTHEBASEPLAYER_H
#define ZMINIGAMEDEFENDTHEBASEPLAYER_H

#include "zMiniGamePlayer.h"
#include "zMiniGameBase.h"

#define MAX_PLAYER_SHOTS 20

class zMiniGameDefendTheBasePlayer;

class zMiniGameDefendTheBasePlayerShot : public zMiniGameObject
{
public:
    virtual void Init(void* data);
    virtual void Update(F32 dt);
    virtual void Render();
    void Emit();
    void Exit();
    virtual F32 GetRadius();

protected:
    zMiniGameDefendTheBasePlayer* owner;
    xMat4x3 mat;
};

class zMiniGameDefendTheBasePlayer : public zMiniGamePlayer
{
public:
    virtual void Init(xEntAsset* asset);
    virtual void BeginUpdate(F32 dt);
    void UpdateOrientation(F32 dt);
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void Render();
    virtual void SceneSetup();
    virtual void Reset();
    virtual void Exit();
#if DEBUG || RELEASE
    virtual void SetupTweaks();
#endif

    virtual void BoundUpdate(xVec3*) {}

    virtual void InitActions() {}

    virtual bool IsJumping() const { return false; }

    void RegisterEnemyKill()
    {
        enemiesKilled++;
    }

protected:
    zMiniGameDefendTheBasePlayerShot shots[MAX_PLAYER_SHOTS];
    S32 maxBaseHP;
    S32 currentBaseHP;
    S32 enemiesKilled;
    S32 wavesWon;
    F32 coolDown;
    F32 coolDownTimer;
};

#endif
