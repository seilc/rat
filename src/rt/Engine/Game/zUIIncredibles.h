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

bool zUIIncrediblesIsSceneUnlocked(S32 idx);
bool zUIIncrediblesAdvanceStage();
bool zUIIncrediblesAllGoalsCompleted(U32 i);
ePauseType zUIIncrediblesGetPauseType();
void zUIIncrediblesSetPauseType(ePauseType type);
U32 zUIIncrediblesGetGameProgress();
bool zUIIncrediblesAllCompleted();

#endif
