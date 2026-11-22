#ifndef ZCOMPLOGICALARMER_H
#define ZCOMPLOGICALARMER_H

#include "zICompLogic.h"

class zCompLogicAlarmer : public zICompLogic
{
public:
    enum eAlarmerStates
    {
        eAlarmerStates_None = -1,
        eAlarmerStates_SearchPlayer,
        eAlarmerStates_ActivatingAlarm,
        eAlarmerStates_Alarm,
        eAlarmerStates_DeactivatingAlarm
    };

    virtual void Attached();
    virtual void AllAttached() {}
    virtual void Detached() {}
#if RELEASE
    virtual void Reset() {}
#endif
    virtual void Update(xScene*, F32);
    virtual bool SystemEvent(xBase*, xBase*, U32, const F32*, xBase*, U32) { return false; }
    virtual F32 GetMaxMoveSpeed() { return 5.0f; }

    void ChangeState(eAlarmerStates nextState);

private:
    eAlarmerStates curState;
};

#endif
