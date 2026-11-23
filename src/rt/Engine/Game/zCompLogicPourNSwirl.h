#ifndef ZCOMPLOGICPOURNSWIRL_H
#define ZCOMPLOGICPOURNSWIRL_H

#include "zICompLogic.h"

struct xAnimTable;

class zCompLogicPourNSwirl : public zICompLogic
{
    U32 whichPot;
    U32 targetPot;
    F32 stirringAccum;
    F32 potTemp[4];
    F32 potPos[4];
    F32 potRate[4];
    bool youLose;
    F32 maxMoveSpeed;

public:
#if DEBUG
    static void AddStaticTweaks();
#endif
    static void CreateAnimTable(xAnimTable*);

    virtual void Attached() {}
    virtual void AllAttached() {}
    virtual void Detached() {}
#if RELEASE
    virtual void Reset() {}
#endif
    virtual void Update(xScene*, F32 dt);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id) { return false; }
};

#endif
