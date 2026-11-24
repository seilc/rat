#ifndef ZUIINCREDIBLES_H
#define ZUIINCREDIBLES_H

#include "types.h"

enum ePauseType
{
    PAUSE_NORMAL,
    PAUSE_STATS,
    PAUSE_JOIN,
    PAUSE_UPGRADE,
    PAUSE_MAINMENU
};

bool zUIIncrediblesAdvanceStage();
ePauseType zUIIncrediblesGetPauseType();
void zUIIncrediblesSetPauseType(ePauseType type);
U32 zUIIncrediblesGetGameProgress();

#endif
