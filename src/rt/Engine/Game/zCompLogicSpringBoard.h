#ifndef ZCOMPLOGICSPRINGBOARD_H
#define ZCOMPLOGICSPRINGBOARD_H

#include "zCompStateMachine.h"

struct xAnimTable;
struct xMovePoint;

class zSpringBoardPatrol : public zCompState
{
    xMovePoint* curPatrolPoint;

public:
    zSpringBoardPatrol();

#if DEBUG
    static void AddStaticTweaks(const char* name);
#endif

    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
    virtual void DebugRender(zNPCBase* owner);
};

class zSpringBoardTurnAround : public zCompState
{
public:
    zSpringBoardTurnAround();

#if DEBUG
    static void AddStaticTweaks(const char*); // UNUSED
#endif

    virtual void Update(zNPCBase* owner, F32 dt);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zSpringBoardIdle : public zCompState
{
public:
    zSpringBoardIdle();

#if DEBUG
    static void AddStaticTweaks(const char*); // UNUSED
#endif

    virtual void Update(zNPCBase*, F32);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zSpringBoardLaunch : public zCompState
{
public:
    zSpringBoardLaunch();

    virtual void Update(zNPCBase*, F32);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zSpringBoardGoToPatrol : public zCompState
{
public:
    zSpringBoardGoToPatrol();

    virtual void Update(zNPCBase*, F32);
    virtual void Enter(zNPCBase* owner);
    virtual void Resume(zNPCBase* owner);
};

class zCompLogicSpringBoard : public zCompStateMachine
{
    xVec3 acc;
    xVec3 oldAcc;

public:
    static bool IsPlayerInRange(zNPCBase* owner, zCompState*, zCompState*);
    static bool IsPlayerOutRange(zNPCBase* owner, zCompState*, zCompState*);
    static bool IsAnimationStopped(zNPCBase* owner, zCompState*, zCompState*);
#if DEBUG
    static void AddStaticTweaks();
#endif
    static void CreateAnimTable(xAnimTable* table);

    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual void SetupStateTranstitionTests();
    virtual void Attached();
    virtual bool InteractionCheck(const SphereCollisionResults& results);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);

    void GetLaunchVelocity(xVec3& vel);
    void Launch();
    void StopNPC(F32 StopForceMag, F32 dt);
};

#endif
