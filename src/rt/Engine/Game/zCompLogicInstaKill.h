#ifndef ZCOMPLOGICINSTAKILL_H
#define ZCOMPLOGICINSTAKILL_H

#include "zNPCStates.h"

struct xMovePoint;

class zInstaKillPatrolXZ : public zCompState
{
    xMovePoint* curPatrolPoint;

public:
    zInstaKillPatrolXZ();

    xMovePoint* FindClosestMP(zNPCBase* owner, U32 firstMovePointID);

    virtual void Activate(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void CheckTranstitions(zNPCBase* owner);
    virtual void DebugRender(zNPCBase* owner);
    virtual bool SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32) { return false; }
};

class zInstaKillAlertState : public zAnimWTimerState
{
public:
    virtual void Activate(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32 dt);
};

class zInstaKillChase : public zCompState
{
public:
    virtual void Activate(zNPCBase* owner);
    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void CheckTranstitions(zNPCBase*);
};

class zCompLogicInstaKill : public zCompStateMachine
{
public:
    xVec3 zoneCenter;

    virtual void AllAttached();
    virtual void NPCGetsDamage(const zCombatDamageInfo&) {}
    virtual void NPCGivesDamage(xEnt*, const zCombatDamageInfo&) {}
    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual void SetupStateTranstitionTests();
};

#endif
