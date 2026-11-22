#ifndef ZUICUSTOMLEVELMANAGER_H
#define ZUICUSTOMLEVELMANAGER_H

#include "types.h"

struct Scene
{
    const char* levelName;
    const char* levelDescription;
    U16 levelNameLength;
    U16 levelDescriptionLength;
    U32 imageID;
    U32 expBonus;
    S32 goalOffenseMrI;
    S32 goalOffenseFrozone;
    S32 goalDefenseMrI;
    S32 goalDefenseFrozone;
    U32 flags;
    U8 bonusCount;
    char levelCode[5];
};

U32 zUIIncrediblesGetNumScenes();
U32 zUIIncrediblesGetSceneCurrent();
U32 zUIIncrediblesGetCurrentSceneNumber();
Scene* zUIIncrediblesGetScene(U32 level);
bool zUIIncrediblesSceneIsCompleted(U32 level);
bool zUIIncrediblesSceneIsUnlocked(U32 level);

#endif
