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

class zCompState
{
    S32 stateEnum;
    zCompStateMachine* stateMachine;
    zCompState* prevState;
    zCompStateTest* tests;

    friend class zCompStateMachine;

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

typedef zCompState* (*zCompStateNewFunc)(S32&);

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
    zCompState* GetState(S32 stateEnum) { xASSERT_L(DEBUG ? 243 : 244, stateEnum<numberOfStates); return states[stateEnum]; }
    S32 GetCurrentStateEnum()
    {
        xASSERT_L(DEBUG ? 297 : 301, currentState);
        return currentState->stateEnum;
    }
};

#endif
