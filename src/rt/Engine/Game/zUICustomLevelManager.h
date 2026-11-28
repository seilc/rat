#ifndef ZUICUSTOMLEVELMANAGER_H
#define ZUICUSTOMLEVELMANAGER_H

#include "types.h"

typedef struct _xCounter xCounter;

struct Scene
{
    enum
    {
        MAX_SCENES = 200
    };

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

// Scene::flags
static const U32 SCENEFLAGS_COMPLETED = 0x1;
static const U32 SCENEFLAGS_UNLOCKED = 0x2;
static const U32 SCENEFLAGS_WAS_COMPLETED = 0x80;

struct SceneUnlock
{
    U32 sceneID;
    U32 sceneID_Unlock[4];
};

extern S32 numScenes;
extern S32 numStoryScenes;
extern S32 numArenaScenes;
extern S32 latestScene;
extern char* allsoaklevels[];

U32 zUIIncrediblesGetNumScenes();
void zUIIncrediblesSetSceneCurrent(U32 value);
U32 zUIIncrediblesGetSceneCurrent();
void zUIIncrediblesSetLatestSceneFromSave(); // UNUSED
U32 zUIIncrediblesGetCurrentSceneNumber();
U32 zUIGetSceneNumberByID(U32 sceneID);
Scene* zUIGetSceneByID(U32 sceneID);
S32 zUIIncrediblesGetNumStoryScenes();
S32 zUIIncrediblesGetNumArenaScenes();
Scene* zUIIncrediblesGetScene(U32 level);
SceneUnlock* zUIGetSceneUnlock(U32 idx);
Scene* zUIIncrediblesFindCurrentScene();
bool zUIIncrediblesSceneIsCompleted(U32 level);
bool zUIIncrediblesSceneIsUnlocked(U32 level);
bool zUIIncrediblesSceneIsCompleted(const char* level); // UNUSED
bool zUIIncrediblesSceneIsCompleted_FindByID(U32 sceneID); // UNUSED
U32 zUISceneNumberFromCounter(xCounter* counter);
bool zUISceneExists(xCounter* mission);
void zUIAddScene(U32 sceneID);
void zUIAllScenesAdded();
void zUILevelsUpdate();

#endif
