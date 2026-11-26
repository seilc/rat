#ifndef ZCOMPSTATEMACHINE_H
#define ZCOMPSTATEMACHINE_H

#include "zICompLogic.h"

struct xBase;
class zNPCBase;
class zCompStateMachine;
class zCompState;

enum eStateTranstitionType
{
    eStateTransType_Switch,
    eStateTransType_Push,
    eStateTransType_Pop,
    eStateTransType_SwitchLast,
    eStateTransType_PushOnLast,
    eStateTestTypeForceInt = FORCEENUMSIZEINT
};

typedef bool (*zCompStateTestFunc)(zNPCBase*, zCompState*, zCompState*);

struct zCompStateTest
{
    zCompState* toState;
    zCompStateTestFunc testFunction;
    F32 period;
    eStateTranstitionType transtitionType;
    zCompStateTest* next;
    F32 remainingTime;
};

typedef zCompState* (*zCompStateNewFunc)(S32&);

template <class T>
zCompState* zCompState_New(S32& memAcc)
{
    memAcc += sizeof(T);
    return new (eMemStaticType_0, eMemMgrTag_Components) T;
}

struct zCompStateMachineTable
{
    S32 stateEnum;
    const zCompStateNewFunc newStateFunction;
};

struct zCompStateTransEvent
{
    eStateTranstitionType transtitionType;
    zCompState* fromState;
    zCompState* toState;
};

class zCompStateTransEventFIFO
{
    zCompStateTransEvent* eventsPool;
    S32 poolSize;
    S32 poolUsed;
    S32 front;
    S32 end;
};

class zCompStateMachine : public zICompLogic
{
    zCompStateMachineTable* stateTable;
    S32 numberOfStates;
    zCompState** states;
    zCompState* globalState;
    zCompState* currentState;
    zCompStateTransEventFIFO eventsBuffer;
    zCompState* defaultState;

public:
    virtual S32 Initialize();
    virtual void Attached();
    virtual void Detached();
#if RELEASE
    virtual void Reset();
#endif
    virtual void AllAttached();
    virtual void Update(xScene*, F32 dt);
    virtual void DebugRender();
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual S32 GetInitialStateEnum() = 0;
    virtual zCompStateMachineTable* GetStateTable() = 0;
    virtual void SetupStateTranstitionTests() = 0;
    virtual S32 GetGlobalStateEnum() { return -1; }
    virtual S32 GetEventsBufferSize() { return 4; }

    zCompState* GetState(S32 stateEnum) { xASSERT_L(DEBUG ? 243 : 244, stateEnum<numberOfStates); return states[stateEnum]; }
    S32 GetCurrentStateEnum();
    void AddPushTest(S32 fromStateEnum, S32 toStateEnum, zCompStateTestFunc test, F32 testPeriod);
    void AddPopTest(S32 fromStateEnum, zCompStateTestFunc test, F32 testPeriod);
    void SwitchState(S32 fromStateEnum, S32 toStateEnum);
    void SwitchLastState(S32 toStateEnum);
};

class zCompState
{
    S32 stateEnum;
    zCompStateMachine* stateMachine;
    zCompState* prevState;
    zCompStateTest* tests;

    friend class zCompStateMachine;

public:
    zCompState();

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

    void SwitchState(S32 newStateEnum) { stateMachine->SwitchState(stateEnum, newStateEnum); }
};

inline S32 zCompStateMachine::GetCurrentStateEnum()
{
    xASSERT_L(DEBUG ? 297 : 301, currentState);
    return currentState->stateEnum;
}

#endif
