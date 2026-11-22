#ifndef ZMINIGAMEPLAYER_H
#define ZMINIGAMEPLAYER_H

#include "zPlayer.h"

class zMiniGamePlayer : public zPlayer
{
public:
    virtual bool IsActionButtonPressed();
    virtual bool IsActionButtonReleased();
    virtual void Init(xEntAsset* asset);
    virtual void SceneSetup();
    virtual void Reset();
    virtual void BeginUpdate(F32 dt);
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void SceneExit(zScene* scene);
#if DEBUG || RELEASE
    virtual void SetupTweaks() {}
#endif
    virtual void ApplyPickup(S32, F32) {}
    virtual S32 GetActivePickup() { return activePickupType; }

    bool IsAI() { return isAI; }

protected:
    S32 activePickupType;
    F32 activePickupTimer;
    bool isAI;
};

#endif
