#ifndef ZCOMPLOGICCONTEXTSENSITIVE_H
#define ZCOMPLOGICCONTEXTSENSITIVE_H

#include "zCompStateMachine.h"

struct xAnimTable;
struct xMovePoint;

class zConSenPatrol : public zCompState
{
    xMovePoint* curPatrolPoint;

public:
    zConSenPatrol();

#if DEBUG
    static void AddStaticTweaks(const char* name);
#endif

    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
    virtual void DebugRender(zNPCBase* owner);
};

class zConSenChase : public zCompState
{
public:
    zConSenChase();

    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void Resume(zNPCBase* owner);
};

class zConSenAttack1 : public zCompState
{
public:
    virtual void Exit(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zCompLogicContextSensitive : public zCompStateMachine
{
    xVec3 zoneCenter;

public:
    static bool GoToChase(zNPCBase* owner, zCompState*, zCompState*);
    static bool BackToPatrol(zNPCBase* owner, zCompState*, zCompState*);
    static bool IsInAttackDistance(zNPCBase* owner, zCompState*, zCompState*);
    static bool ButtonPressed(zNPCBase*, zCompState*, zCompState*);
    static bool ButtonPressLast(zNPCBase* owner, zCompState*, zCompState*);
    static bool GroundAttackMissed(zNPCBase* owner, zCompState*, zCompState*);
    static bool GroundAttackHit(zNPCBase* owner, zCompState*, zCompState*);
#if DEBUG
    static void AddStaticTweaks();
#endif
    static void CreateAnimTable(xAnimTable* table);

    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual void SetupStateTranstitionTests();
    virtual void Attached();
    virtual void AllAttached();
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
};

#endif
