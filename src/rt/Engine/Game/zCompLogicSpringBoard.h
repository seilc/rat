#ifndef ZCOMPLOGICSPRINGBOARD_H
#define ZCOMPLOGICSPRINGBOARD_H

#include "zCompStateMachine.h"

struct xAnimTable;

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
