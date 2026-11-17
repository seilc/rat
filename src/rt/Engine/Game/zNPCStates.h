#ifndef ZNPCSTATES_H
#define ZNPCSTATES_H

#include "zCompStateMachine.h"

class zTimerState : public zCompState
{
    S32 stateEnumToGo;
    F32 stateTime;
    F32 endTime;

public:
    void Setup(S32 nextStateEnum, F32 time); // UNUSED
#if RELEASE
    virtual void Resume(zNPCBase*); // UNUSED
#else
    virtual void Enter(zNPCBase* owner); // UNUSED
#endif
    virtual void Update(zNPCBase*, F32 dt); // UNUSED
#if RELEASE
    virtual void CheckTranstitions(zNPCBase*); // UNUSED
#endif
};

class zAnimPlayState : public zCompState
{
    U32 animStateID;
    S32 stateEnumToGo;

public:
    void Setup(S32 nextStateEnum, const char* animationName);
#if RELEASE
    virtual void CheckTranstitions(zNPCBase* owner);
    virtual void Activate(zNPCBase* owner);
#else
    virtual void Update(zNPCBase* owner, F32);
    virtual void Resume(zNPCBase* owner);
#endif
};

class zAnimWTimerState : public zCompState
{
    U32 animStateID;
    S32 stateEnumToGo;
    F32 stateTime;
    F32 endTime;

public:
    void Setup(S32 nextStateEnum, const char* animationName, F32 time);
#if RELEASE
    virtual void Activate(zNPCBase* owner);
    virtual void Update(zNPCBase*, F32 dt);
    virtual void Resume(zNPCBase*);
    virtual void CheckTranstitions(zNPCBase*);
#else
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
    virtual void Update(zNPCBase*, F32 dt);
#endif
};

#endif
