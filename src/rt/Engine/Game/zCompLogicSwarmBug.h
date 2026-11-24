#ifndef ZCOMPLOGICSWARMBUG_H
#define ZCOMPLOGICSWARMBUG_H

#include "zCompStateMachine.h"

class zCompLogicSwarmBug : public zCompStateMachine
{
public:
#if DEBUG
    static void AddStaticTweaks();
#endif

    virtual void SetupStateTranstitionTests();
    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual bool SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase* to_param_widget, U32);
    virtual void AllAttached();
};

class zSwarmBugWander : public zCompState
{
public:
    zSwarmBugWander();

    virtual void Enter(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32);
    virtual void Resume(zNPCBase*) {}
    virtual void DebugRender(zNPCBase*) {}
};

#endif
