#ifndef ZNPCINSTAKILL_H
#define ZNPCINSTAKILL_H

#include "zNPCBase.h"
#if !RELEASE
#include "zCompStateMachine.h"
#endif

#if RELEASE
struct xAnimTable;
class zCompSteeringGenericParams;
class zCompPerceptionParams;
class zICompNPCEntityParams;
#endif

class zNPCInstaKill : public zNPCBase
{
public:
    static void CompParametersInit();
#if RELEASE
    static void CreateAnimTable(xAnimTable* table);
    static void InitCombatDamage();
#endif
#if DEBUG || RELEASE
    static void AddTweaks(const char* branchString);
#endif

    virtual bool Activate();
#if !RELEASE
    virtual void Deactivate();
#endif
    virtual void Update(xScene* xscn, F32 dt);
    virtual void Render();

#if RELEASE
    static zICompNPCEntityParams sEntParams;
    static zCompPerceptionParams sPerceptionParams;
    static zCompSteeringGenericParams sSteeringPatrolParams;
    static zCompSteeringGenericParams sSteeringAlertParams;
    static zCompSteeringGenericParams sSteeringChaseParams;
#endif
};

#if !RELEASE
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

#endif
