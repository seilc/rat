#ifndef ZMINIGAMEAVOIDTHESTUFFPLAYER_H
#define ZMINIGAMEAVOIDTHESTUFFPLAYER_H

#include "zMiniGamePlayer.h"
#include "zPlayerAction.h"

class zAvoidTheStuffPlayerMove : public zPlayerAction
{
public:
#if DEBUG || RELEASE
    virtual void AddTweaks() {}
#endif
    virtual U32 GetID() { return 0; }
    virtual void AddStates(xAnimTable* table);
    virtual void AddInternalTransitions(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable*) {}
    virtual void AddTransitionsFrom(xAnimTable*, const char*, U16, F32, U32, U32) {}
    virtual void Reset();

    PLAYER_ACTION_CHECK_L(zAvoidTheStuffPlayerMove, StartJumpCheck, 31)
    PLAYER_ACTION_CHECK_L(zAvoidTheStuffPlayerMove, StartDuckCheck, 32)
    PLAYER_ACTION_CHECK_L(zAvoidTheStuffPlayerMove, StartBlockLeftCheck, 33)
    PLAYER_ACTION_CHECK_L(zAvoidTheStuffPlayerMove, StartBlockRightCheck, 34)
};

class zMiniGameAvoidTheStuffPlayer : public zMiniGamePlayer
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

    virtual void BoundUpdate(xVec3*) {}

    virtual void InitActions();

    virtual bool IsJumping() const { return false; }
};

#endif
