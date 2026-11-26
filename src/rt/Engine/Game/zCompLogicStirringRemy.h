#ifndef ZCOMPLOGICSTIRRINGREMY_H
#define ZCOMPLOGICSTIRRINGREMY_H

#include "zCompStateMachine.h"

struct xMovePoint;

class zStirringRemyIdle : public zCompState
{
public:
    zStirringRemyIdle();

    virtual void Update(zNPCBase* owner, F32);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zStirringRemyRun : public zCompState
{
    xMovePoint* curPatrolPoint;

public:
    zStirringRemyRun();

    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zStirringRemyPush : public zCompState
{
public:
    zStirringRemyPush();

    virtual void Update(zNPCBase*, F32);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zCompLogicStirringRemy : public zCompStateMachine
{
    xVec3 acc;
    xVec3 oldAcc;

public:
    static bool AreWeThereYet(zNPCBase*, zCompState*, zCompState*);
    static bool GotRunEvent(zNPCBase*, zCompState*, zCompState*);
    static bool GotPushEvent(zNPCBase*, zCompState*, zCompState*);
    static bool IsAnimationStopped(zNPCBase* owner, zCompState*, zCompState*);
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif
    static void CreateAnimTable(xAnimTable* table);

    virtual void SetupStateTranstitionTests();
    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual void Attached();
    virtual void AllAttached() {}
    virtual void Detached() {}
    virtual bool SystemEvent(xBase*, xBase*, U32 to_event, const F32* to_param, xBase*, U32 to_param_widget_id);
};

#endif
