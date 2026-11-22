#ifndef ZCOMPLOGICSTIRRINGREMY_H
#define ZCOMPLOGICSTIRRINGREMY_H

#include "zCompStateMachine.h"

struct xAnimTable;

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

    virtual S32 GetInitialStateEnum();
    virtual zCompStateMachineTable* GetStateTable();
    virtual void SetupStateTranstitionTests();
    virtual void Attached();
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
};

#endif
