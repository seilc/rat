#ifndef ZCOMPLOGICWAITER_H
#define ZCOMPLOGICWAITER_H

#include "zICompLogic.h"
#include "xBound.h"
#include "xCollide.h"

struct xMovePoint;

class zCompLogicWaiter : public zICompLogic
{
    static const U32 MAX_JSP_COLLISIONS;
    static const U32 MAX_OBJECT_COLLISIONS;
    static const U32 MAX_COLLISIONS = 12;
    static const S32 NUM_BOUNDS = 1;

    // collisionUserFlags
    static const U32 zCOLLISION_NEEDS_REFLECTION = 0x1;
    static const U32 zCOLLISION_NEEDS_DEPEN_OUTER_BOUND = 0x2;
    static const U32 zCOLLISION_NEEDS_DEPEN_INNER_BOUND;
    static const U32 zCOLLISION_NPC = 0x8;

    xVec3 vel;
    xVec3 oldPos;
    xVec3 preKnockbackVel;
    F32 knockbackTimer;
    bool activated;
    bool playingSpecialAnim;
    F32 scalarVel;
    bool hitReactPending;
    S32 numCollisions;
    xCollis collisions[MAX_COLLISIONS];
    U32 collisionUserFlags[MAX_COLLISIONS];
    xBound bound[NUM_BOUNDS];
    S32 currBound;
    xMovePoint* curPatrolPoint;
    bool arrived;
    F32 timer;
    bool init;
    F32 animWalk[3];
};

#endif
