#ifndef ZUIINCREDIBLES_H
#define ZUIINCREDIBLES_H

#include "types.h"

struct xIniFile;
struct xSerial;
typedef struct _xCounter xCounter;

enum ePauseType
{
    PAUSE_NORMAL,
    PAUSE_STATS,
    PAUSE_JOIN,
    PAUSE_UPGRADE,
    PAUSE_MAINMENU
};

enum UnlockFeature
{
    Unlock_Scenes
};

bool zUIIncrediblesIsSceneCompleted(S32 idx);
bool zUIIncrediblesIsSceneUnlocked(S32 idx);
bool zUIIncrediblesAdvanceStage();
bool zUIIncrediblesAllGoalsCompleted(U32 i);
bool zUIIncrediblesAllGoalsCompleted();
S32 zUIIncrediblesGoalsCompletedInPercent();
S32 zUIIncrediblesScenesCompletedInPercent();
bool zUIIncrediblesAllScenesCompleted();
bool zUIIncrediblesAllCompleted();
void zUIIncrediblesUnlockStage(U32 sceneID); // UNUSED
void zUIIncrediblesUnlock(UnlockFeature feature); // UNUSED
void zUIIncrediblesInitINI(xIniFile* ini);
void zUIIncrediblesStartup();
void zUIIncrediblesInit(); // UNUSED
void zUIIncrediblesResetGame();
void zUIIncrediblesSceneSetup();
U32 zUIIncrediblesGetTotalCheckpoints();
U32 zUIIncrediblesGetHitCheckpoints();
void zUIIncrediblesWarp(xCounter* counter);
void zUIIncrediblesWarpDirect(U32 sceneIndex);
void zUIIncrediblesWarp(U32 sceneCode);
void zUIIncrediblesHitCheckPoint();
void zUIIncrediblesPlayedMovie(S32 index);
void zUIIncrediblesOpenBonus(S32 index);
void zUIIncrediblesSave(xSerial* serial);
ePauseType zUIIncrediblesGetPauseType();
void zUIIncrediblesSetPauseType(ePauseType type);
void zUIIncrediblesUnlockUpToLatest(); // UNUSED
void zUIIncrediblesLoad(xSerial* serial);
U32 zUIIncrediblesGetGameProgress();
U32 zUIIncrediblesGetAutoSaveStage();

#endif
