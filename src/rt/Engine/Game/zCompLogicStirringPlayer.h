#ifndef ZCOMPLOGICSTIRRINGPLAYER_H
#define ZCOMPLOGICSTIRRINGPLAYER_H

#include "zICompLogic.h"

struct xAnimTable;

class zCompLogicStirringPlayer : public zICompLogic
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
    static void CreateAnimTable(xAnimTable* table);
#if DEBUG || RELEASE
    static void AddStaticTweaks();
#endif

    virtual void Update(xScene*, F32 dt);
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
};

#endif
