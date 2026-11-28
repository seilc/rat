#ifndef ZMINIGAMEPUFFEROIDSPLAYER_H
#define ZMINIGAMEPUFFEROIDSPLAYER_H

#include "zMiniGamePlayer.h"

class zMiniGamePufferoidsShot;

class zMiniGamePufferoidsPlayer : public zMiniGamePlayer
{
public:
    virtual void Init(xEntAsset* asset);
    virtual void BeginUpdate(F32 dt);
    void UpdateOrientation(F32 dt);
    void UpdatePosition(F32 dt);
    virtual void ApplyPickup(S32 applyPickup, F32 activeTime);
    void HitByPuff();
    zMiniGamePufferoidsShot* GetFreeShot();
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void Render();
    virtual void SceneSetup();
    virtual void Reset();
    virtual void Exit();

    virtual void BoundUpdate(xVec3*) {}

    virtual void InitActions() {}

    virtual void AddStates(xAnimTable* table);

    virtual bool IsJumping() const { return false; }

protected:
    S32 state;
    xMat3x3 restoreMat;
    S32 lives;
    S32 score;
    F32 invincibleTimer;
    F32 fellTimer;
    bool trip;
    bool skipRender;
    S32 skipCounter;
    zMiniGamePufferoidsShot* shots;
};

#endif
