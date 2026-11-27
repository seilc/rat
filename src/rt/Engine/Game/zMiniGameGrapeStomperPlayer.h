#ifndef ZMINIGAMEGRAPESTOMPERPLAYER_H
#define ZMINIGAMEGRAPESTOMPERPLAYER_H

#include "zMiniGamePlayer.h"

struct zShrapnelAsset;
class InputChecker;

class zMiniGameGrapeStomperPlayer : public zMiniGamePlayer
{
public:
    virtual void Init(xEntAsset* asset);
    virtual void BeginUpdate(F32 dt);
    virtual void Update(F32 dt);
    virtual void EndUpdate(F32 dt);
    virtual void Render();
    virtual void SceneSetup();
    virtual void Reset();
    virtual void Exit();

    virtual void AddStates(xAnimTable* table);

    virtual void BoundUpdate(xVec3*) {}

    virtual void InitActions() {}

    virtual bool IsJumping() const { return false; }

protected:
    U32 curStomp;
    U32 prevStomp;
    S32 score;
    xModelInstance* models[3];
    S32 frameCount;
    S32 whichModel;
    InputChecker* inputChecker;
    zShrapnelAsset* leftSplash[3];
    zShrapnelAsset* rightSplash[3];
};

#endif
