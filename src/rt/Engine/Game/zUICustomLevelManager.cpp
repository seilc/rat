#include "xCommon.h"
#include "zUICustomLevelManager.h"

#include "xCounter.h"
#include "xString.h"
#include "xutil.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

#include "decomp.h"

#include <string.h>

namespace {

Scene scenes[Scene::MAX_SCENES];
SceneUnlock scenesUnlock[Scene::MAX_SCENES];
U32 sceneCurrent;

} // namespace

S32 numScenes;
S32 numStoryScenes;
S32 numArenaScenes;
S32 latestScene;
char* allsoaklevels[Scene::MAX_SCENES];

U32 zUIIncrediblesGetNumScenes()
{
    return numScenes;
}

void zUIIncrediblesSetSceneCurrent(U32 value)
{
    xASSERT_L(34, value < Scene::MAX_SCENES);
    sceneCurrent = value;
}

U32 zUIIncrediblesGetSceneCurrent()
{
    return sceneCurrent;
}

U32 zUIIncrediblesGetCurrentSceneNumber()
{
    if (globals.sceneCur->sceneID == IDTAG('M','N','U','S'))
        return 1;

    for (S32 i = 1; i < numScenes; i++) {
        U32 sceneID = IDTAG(
            scenes[i].levelCode[0], scenes[i].levelCode[1], scenes[i].levelCode[2], scenes[i].levelCode[3]);
        if (sceneID == globals.sceneCur->sceneID)
            return i;
    }

    xFAIL_MSG_L(64, "Error looking up scene");
    return 0;
}

U32 zUIGetSceneNumberByID(U32 sceneID)
{
    for (U32 i = 1; i < numScenes; i++) {
        U32 id = IDTAG(
            scenes[i].levelCode[0], scenes[i].levelCode[1], scenes[i].levelCode[2], scenes[i].levelCode[3]);
        if (id == sceneID)
            return i;
    }
    return 0;
}

Scene* zUIGetSceneByID(U32 sceneID)
{
    U32 sceneNumber = zUIGetSceneNumberByID(sceneID);
    if (sceneNumber)
        return zUIIncrediblesGetScene(sceneNumber);
    xFAIL_FMT_L(85, "Scene with ID - 0x%08X doesn't exist", sceneID);
    return NULL;
}

S32 zUIIncrediblesGetNumStoryScenes()
{
    return numStoryScenes;
}

S32 zUIIncrediblesGetNumArenaScenes()
{
    return numArenaScenes;
}

Scene* zUIIncrediblesGetScene(U32 level)
{
    xASSERT_L(101, level < Scene::MAX_SCENES);
    return &scenes[level == 0 ? 1 : level];
}

SceneUnlock* zUIGetSceneUnlock(U32 idx)
{
    xASSERT_L(107, idx < Scene::MAX_SCENES);
    return &scenesUnlock[idx];
}

Scene* zUIIncrediblesFindCurrentScene()
{
    return &scenes[zUIIncrediblesGetCurrentSceneNumber()];
}

bool zUIIncrediblesSceneIsCompleted(U32 level)
{
    xASSERT_L(120, level < Scene::MAX_SCENES);
    return scenes[level].flags & 0x1 ? true : false;
}

bool zUIIncrediblesSceneIsUnlocked(U32 level)
{
    xASSERT_L(127, level < Scene::MAX_SCENES);
    return scenes[level].flags & 0x2 ? true : false;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("0");
#endif

struct Group
{
    char code[2];
    char pad_unused[2];
};

class zUISceneGroups
{
public:
    enum
    {
        MAX_SCENE_GROUPS = 20
    };

    zUISceneGroups() { numSceneGroups = 0; }

    void AddSceneGroup(const char* sceneID)
    {
        for (U32 i = 0; i < numSceneGroups; i++)
            if (groups[i].code[0] == sceneID[0] && groups[i].code[1] == sceneID[1])
                return;
        xASSERT_L(175, numSceneGroups < MAX_SCENE_GROUPS);

        groups[numSceneGroups].code[0] = sceneID[0];
        groups[numSceneGroups].code[1] = sceneID[1];
        numSceneGroups++;
    }

    void AddSceneGroup(U32 sceneID)
    {
        char code[2];
        code[0] = sceneID >> 24;
        code[1] = sceneID >> 16;
        AddSceneGroup(code);
    }

    Group& operator[](U32 index)
    {
        xASSERT_L(191, index < numSceneGroups);
        return groups[index];
    }

    U32 NumGroups() { return numSceneGroups; }

private:
    U32 numSceneGroups;
    Group groups[MAX_SCENE_GROUPS];
};

zUISceneGroups sSceneGroups;

U32 zUISceneNumberFromCounter(xCounter* counter)
{
    U32 groupIndex = counter->count / 100;
    if (groupIndex >= sSceneGroups.NumGroups())
        return 0;
    const Group& group = sSceneGroups[groupIndex];
    U32 tens = (counter->count % 100) / 10;
    U32 ones = counter->count % 10;
    U32 sceneID = IDTAG(group.code[0], group.code[1], '0' + tens, '0' + ones);
    return zUIGetSceneNumberByID(sceneID);
}

bool zUISceneExists(xCounter* mission)
{
    return zUISceneNumberFromCounter(mission) != 0;
}

void zUIAddScene(U32 sceneID)
{
    char sceneIDStr[5];
    sceneIDStr[0] = sceneID >> 24;
    sceneIDStr[1] = sceneID >> 16;
    sceneIDStr[2] = sceneID >> 8;
    sceneIDStr[3] = sceneID;
    sceneIDStr[4] = '\0';

    U32 number = zUIGetSceneNumberByID(sceneID);
    if (number) {
        iprintf("WARNING: Scene %s added twice!", sceneIDStr);
        return;
    }
    if (numScenes >= Scene::MAX_SCENES) {
        iprintf("WARNING: Too many scenes. Max is %d!", Scene::MAX_SCENES);
        return;
    }

    sSceneGroups.AddSceneGroup(sceneID);
    Scene& scene = scenes[numScenes];
    strcpy(scene.levelCode, sceneIDStr);
    allsoaklevels[numScenes - 1] = scene.levelCode;

    scene.levelName = NULL;
    scene.levelDescription = NULL;
    scene.levelNameLength = 0;
    scene.levelDescriptionLength = 0;
    scene.imageID = 0;
    scene.flags = 0;
    scene.expBonus = 0;
    scene.goalOffenseMrI = 0;
    scene.goalOffenseFrozone = 0;
    scene.goalDefenseMrI = 0;
    scene.goalDefenseFrozone = 0;
    scene.bonusCount = 0;

    numScenes++;
}

void zUIAllScenesAdded()
{
    allsoaklevels[numScenes - 1] = NULL;
}

void zUILevelsUpdate()
{
    static U32 cMissionCounterID = xStrHash("ZZZ_LSELECT COUNTER");
    static U32 cMissionValidCounterID = xStrHash("UI MISSION VALID COUNTER");
    xCounter* mission = (xCounter*)zSceneFindObject(cMissionCounterID);
    xCounter* missionValid = (xCounter*)zSceneFindObject(cMissionValidCounterID);
    if (mission && missionValid)
        missionValid->count = zUISceneExists(mission) ? 1 : 0;
}
