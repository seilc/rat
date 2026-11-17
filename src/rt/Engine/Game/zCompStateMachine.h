#ifndef ZCOMPSTATEMACHINE_H
#define ZCOMPSTATEMACHINE_H

#include "types.h"

struct xBase;
class zNPCBase;
class zCompStateMachine;
struct zCompStateTest;

class zCompState
{
    S32 stateEnum;
    zCompStateMachine* stateMachine;
    zCompState* prevState;
    zCompStateTest* tests;

public:
    void SwitchState(S32 newStateEnum);

    virtual void Update(zNPCBase*, F32) {}
    virtual void Enter(zNPCBase* owner) { Resume(owner); }
    virtual void Exit(zNPCBase*) {}
    virtual void Pause(zNPCBase*) {}
    virtual void Resume(zNPCBase*) {}
    virtual void Activate(zNPCBase*) {}
    virtual void Deactivate(zNPCBase*) {}
#if RELEASE
    virtual void CheckTranstitions(zNPCBase*) {}
#endif
    virtual void DebugRender(zNPCBase*) {}
    virtual bool SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32) { return false; }
};

#endif
