#ifndef ZCOMPLOGICSWARMOWL_H
#define ZCOMPLOGICSWARMOWL_H

#include "zCompStateMachine.h"

class zCompLogicSwarmOwl : public zCompStateMachine
{
public:
#if DEBUG
    static void AddStaticTweaks();
#endif
    static void CreateAnimTable(xAnimTable* table);

    virtual void SetupStateTranstitionTests();
    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual bool SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase* to_param_widget, U32);
    virtual void AllAttached();
};

class zSwarmOwlWander : public zCompState
{
public:
    zSwarmOwlWander() {}

    virtual void Enter(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32);
    virtual void Resume(zNPCBase*) {}
    virtual void DebugRender(zNPCBase*) {}
};

class zSwarmOwlBlock : public zCompState
{
public:
    zSwarmOwlBlock() {}

    virtual void Enter(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32);
    virtual void Resume(zNPCBase*) {}
    virtual void DebugRender(zNPCBase*) {}
};

class zSwarmOwlAttracted : public zCompState
{
public:
    zSwarmOwlAttracted() {}

    virtual void Enter(zNPCBase* owner);
    virtual void Update(zNPCBase*, F32);
    virtual void Resume(zNPCBase*) {}
    virtual void DebugRender(zNPCBase*) {}
};

#endif
