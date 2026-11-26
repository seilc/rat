#ifndef ZRATBALLPLAYERACTIONS_H
#define ZRATBALLPLAYERACTIONS_H

#include "zRatBallPlayer.h"

struct zRaftTrack;

class zRatBallPlayerDrive : public zPlayerAction
{
public:
    virtual void AddTransitionsFrom(xAnimTable* table, const char* fromStates, U16 priority, F32 blend, U32 flags, U32 userFlags);
    virtual U32 GetID();
    virtual void AddStates(xAnimTable* table);
    virtual void AddInternalTransitions(xAnimTable* table);
    virtual void AddActionTransitions(xAnimTable* table);
    virtual void Reset();
    virtual void Move(xScene* s, F32 dt, xEntFrame* frame);
    virtual void Setup();
    virtual void Begin();
    virtual void End();
#if DEBUG || RELEASE
    virtual void AddTweaks();
#endif

    void Physics(F32 dt, xEntFrame*);
    bool operator()(xEnt& ent);
    void SphereCollision(const xVec3* pNewPosition, const xVec3* pVelocity, bool* pInGroundContact, xVec3* pGroundNormal);
    void ApplyCollisionRebounds(xVec3* pVelocity);
    void DepenetrateCollisionList(xVec3* pPosition, F32);
    void SlideTrackUpdate();

protected:
    xMat4x3 physicsMat;
    xVec3 velocity;
    xVec3 oldVelocity;
    F32 steeringPosition;
    bool initPending;
    bool started;
    F32 currSpeed;
    xVec3 acc;
    S32 numCollisions;
    xCollis collisions[12];
    U32 collisionUserFlags[12];
    xBound bound[2];
    S32 currBound;
    bool inGroundContact;
    S32 numTracks;
    zRaftTrack* trackList[10];
    xVec3 slideTrackDir;
    bool slideFastTrack;
    xVec3 slideNormal;
    F32 slideMinSpeed;
    F32 slideMaxSpeed;
    F32 slideNormSpeed;
    bool slideNotGroundedSinceSlide;
    U32 slideTrackSliding;

    PLAYER_ACTION_CHECK_L(zRatBallPlayerDrive, MoveStartCheck, 127)
    PLAYER_ACTION_CHECK_L(zRatBallPlayerDrive, MoveStopCheck, 128)
};

#endif
