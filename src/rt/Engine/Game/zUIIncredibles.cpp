#include "xCommon.h"
#include "zUIIncredibles.h"

#include "xCounter.h"
#include "xFont.h"
#include "xIni.h"
#include "xMath.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xTextureManager.h"
#include "xserializer.h"
#include "xutil.h"
#include "zBase.h"
#include "zEvent.h"
#include "zFMV.h"
#include "zGame.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zLoadingScreen.h"
#include "zMain.h"
#include "zMusic.h"
#include "zPortal.h"
#include "zScene.h"
#include "zUIController.h"
#include "zUICustomArtworkManager.h"
#include "zUICustomCheckpoints.h"
#include "zUICustomLevelManager.h"
#include "zUICustomSceneImage.h"
#include "zUICustomWireframe.h"

#include "decomp.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define FINAL_SCENES_COUNT 21
#define MOVIES_COUNT 53

static bool unlockedMovies[MOVIES_COUNT];
char sceneBaseName[32];
char sceneBaseDescription[256];

namespace {

const char* SCENE_STATUS_BONUS;
bool unlockCheatOn;
bool sceneUnlockAll;

U32 GetSceneNumber(const char* str, U32 size, bool* isBM01)
{
    if (isBM01) {
        *isBM01 = false;
    }

    U32 sceneCurrent = zUIIncrediblesGetSceneCurrent();

    if (size == 7 && imemcmp(str, "current", 7) == 0) {
        return sceneCurrent == 0 ? 1 : sceneCurrent;
    }

    bool isNumber = true;
    for (U32 i = 0; i < size; i++)
        if (!isdigit(str[i])) {
            isNumber = false;
            break;
        }

    U32 sceneNumber = 0;
    if (isNumber) {
        for (U32 i = 0; i < size; i++) {
            sceneNumber *= 10;
            sceneNumber += str[i] - '0';
        }
    } else {
        U32 id = xStrHash(str, size);
        xBase* base = zSceneFindObject(id);
        if (!base) {
            return 0;
        }

        if (base->baseType == eBaseTypeUIController) {
            zUIController* controller = (zUIController*)base;
            if (!controller->custom) {
                return 0;
            }
        }

        if (base->baseType != eBaseTypeCounter)
            return 0;
        sceneNumber = ((xCounter*)base)->count;
    }

    if (sceneNumber >= (U32)numScenes)
        sceneNumber = 0;
    return sceneNumber;
}

void ParseTagSceneName(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size) {
        return;
    }

    U32 sceneNumber = GetSceneNumber(ti.value.text, ti.value.size, NULL);

    j.context = NULL;
    if (sceneNumber == 0) {
        char full[64], sceneStr[8];
        SCENEIDSTR(xglobals->sceneCur->sceneID, sceneStr);
        sprintf(full, "%s %s", sceneBaseName, sceneStr);
        U32 len = 0;
        j.context = (void*)xTextFindString(full, &len);
        j.context_size = len;
    }

    if (!j.context) {
        Scene* sceneCur = zUIIncrediblesGetScene(sceneNumber);
        j.context = (void*)sceneCur->levelName;
        j.context_size = sceneCur->levelNameLength;
    }

    j.flag.insert = true;
}

void ParseTagSceneDescription(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size) {
        return;
    }

    U32 sceneNumber = GetSceneNumber(ti.value.text, ti.value.size, NULL);
    Scene* sceneCur = zUIIncrediblesGetScene(sceneNumber);
    j.context = (void*)sceneCur->levelDescription;
    j.context_size = sceneCur->levelDescriptionLength;
    j.flag.insert = true;
}

void ParseTagSceneStatus(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag& ti)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    if (ti.action.size != 1 || ti.action.text[0] != ':' || !ti.value.size) {
        return;
    }

    bool isBM01 = false;
    U32 sceneNumber = GetSceneNumber(ti.value.text, ti.value.size, &isBM01);

    Scene* sceneCur = zUIIncrediblesGetScene(sceneNumber);
    U32 bonusCount = sceneCur->bonusCount;

    if (bonusCount) {
        static char buffer[48];
        sprintf(buffer, SCENE_STATUS_BONUS, bonusCount);
        j.context = buffer;
        j.context_size = strlen(buffer);
    } else {
        j.context = (void*)"";
        j.context_size = 0;
    }

    j.flag.insert = true;
}

void ParseTagLoadingSceneName(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;

    const char* sceneName = zSceneGetLevelName(loadSceneID);
    j.context = (void*)sceneName;
    j.context_size = strlen(sceneName);
    j.flag.insert = true;
}

void ParseTagIgnore(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
}

void ParseTagSpace(xtextbox::jot& j, const xtextbox&, const xtextbox&, const xtextbox::split_tag&)
{
    j.reset_flags();
    j.flag.invisible = j.flag.ethereal = true;
    j.context = (void*)" ";
    j.context_size = 1;
    j.flag.insert = true;
}

xtextbox::tag_type newTags[] = {
    { { "loadingscenename", 16 }, ParseTagLoadingSceneName, NULL, NULL },
    { { "scenedesc", 9 }, ParseTagSceneDescription, NULL, NULL },
    { { "scenename", 9 }, ParseTagSceneName, NULL, NULL },
    { { "scenestatus", 11 }, ParseTagSceneStatus, NULL, NULL },
    { { "tlh", 3 }, ParseTagIgnore, NULL, NULL },
    { { "tli", 3 }, ParseTagIgnore, NULL, NULL },
    { { "tls", 3 }, ParseTagSpace, NULL, NULL },
};

void zUIUnlockRelatedStages(U32 sceneID)
{
    for (S32 i = 1; i < numScenes; i++) {
        SceneUnlock* scene = zUIGetSceneUnlock(i);
        if (scene->sceneID == sceneID) {
            S32 j = 0;
            // BUG: reads one past sceneID_Unlock
            while (scene->sceneID_Unlock[j] && j < 5) {
                Scene* sceneToUnlock = zUIGetSceneByID(scene->sceneID_Unlock[j++]);
                sceneToUnlock->flags |= SCENEFLAGS_UNLOCKED;
            }
            break;
        }
    }
}

} // namespace

bool zUIIncrediblesIsSceneCompleted(S32 idx)
{
    return zUIIncrediblesGetScene(idx)->flags & SCENEFLAGS_COMPLETED ? true : false;
}

bool zUIIncrediblesIsSceneUnlocked(S32 idx)
{
    return zUIIncrediblesGetScene(idx)->flags & SCENEFLAGS_UNLOCKED ? true : false;
}

bool zUIIncrediblesAdvanceStage()
{
    bool sceneWasCompleted = true;
    for (S32 i = 1; i < numScenes; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        U32 sceneID = IDTAG(scene->levelCode[0], scene->levelCode[1], scene->levelCode[2], scene->levelCode[3]);
        if (globals.sceneCur->sceneID == sceneID) {
            if (!(scene->flags & SCENEFLAGS_COMPLETED)) {
                scene->flags |= SCENEFLAGS_COMPLETED;
                sceneWasCompleted = false;
            } else {
                scene->flags |= SCENEFLAGS_WAS_COMPLETED;
            }

            latestScene = MAX(latestScene, i + 1);
            zUIUnlockRelatedStages(sceneID);
            return sceneWasCompleted;
        }
    }
    xFAIL_MSG_L(329, "Crap! Advance stage logic is screwed up");
    return !sceneWasCompleted;
}

bool zUIIncrediblesAllGoalsCompleted(U32 i)
{
    Scene* scene = zUIIncrediblesGetScene(i);
    S32 j;

    for (j = 0; j <= 2; j += 2) {
        if (!(scene->flags & (0x4 << j)))
            return false;
    }

    for (j = 1; j < 4; j += 2) {
        if (!(scene->flags & (0x4 << j)))
            return false;
    }
    return true;
}

bool zUIIncrediblesAllGoalsCompleted()
{
    xASSERT_L(353, numScenes == FINAL_SCENES_COUNT);

    for (S32 i = 1; i < FINAL_SCENES_COUNT; i++) {
        if (!zUIIncrediblesAllGoalsCompleted(i))
            return false;
    }
    return true;
}

S32 zUIIncrediblesGoalsCompletedInPercent()
{
    S32 pointsPerGoal = 1;
    S32 totalGoals = 0;
    S32 totalPoints = 0;
    xASSERT_L(368, numScenes == FINAL_SCENES_COUNT);

    for (S32 i = 1; i < FINAL_SCENES_COUNT; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        S32 j;

        for (j = 0; j <= 2; j += 2) {
            totalGoals++;
            if (scene->flags & (0x4 << j)) {
                totalPoints += pointsPerGoal;
            }
        }

        for (j = 1; j < 4; j += 2) {
            totalGoals++;
            if (scene->flags & (0x4 << j))
                totalPoints += pointsPerGoal;
        }
    }
    return totalPoints;
}

S32 zUIIncrediblesScenesCompletedInPercent()
{
    S32 pointsPerScene = 1, totalPoints = 0;
    xASSERT_L(397, numScenes == FINAL_SCENES_COUNT);

    for (S32 i = 1; i < FINAL_SCENES_COUNT; i++) {
        if (zUIIncrediblesIsSceneCompleted(i))
            totalPoints += pointsPerScene;
    }
    return totalPoints;
}

bool zUIIncrediblesAllScenesCompleted()
{
    xASSERT_L(409, numScenes == FINAL_SCENES_COUNT);
    for (S32 i = 1; i < FINAL_SCENES_COUNT; i++) {
        if (!zUIIncrediblesIsSceneCompleted(i))
            return false;
    }
    return true;
}

bool zUIIncrediblesAllCompleted()
{
    return zUIIncrediblesAllScenesCompleted() && zUIIncrediblesAllGoalsCompleted();
}

// UNUSED: needed to match (code from RotU GC)
void zUIIncrediblesUnlockStage(U32 sceneID)
{
    for (S32 i = 1; i < numScenes; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        U32 id = IDTAG(scene->levelCode[0], scene->levelCode[1], scene->levelCode[2], scene->levelCode[3]);
        if (id == sceneID) {
            scene->flags |= SCENEFLAGS_UNLOCKED;
            return;
        }
    }

    char buffer[64];
    sprintf(
        buffer, "Tried to unlock scene %c%c%c%c, but scene doesn't exist", sceneID >> 24, (sceneID >> 16) & 0xFF,
        (sceneID >> 8) & 0xFF, sceneID & 0xFF);
    xFAIL_MSG(buffer);
}

// UNUSED: needed to match (code from RotU GC)
void zUIIncrediblesUnlock(UnlockFeature feature)
{
    switch (feature) {
    case Unlock_Scenes:
        for (S32 i = 1; i < numScenes; i++) {
            Scene* scene = zUIIncrediblesGetScene(i);
            scene->flags |= SCENEFLAGS_UNLOCKED;
        }
        unlockCheatOn = true;
        latestScene = numScenes;
        break;
    default:
        xFAIL_MSG("Attempted to Unlock unknown feature!");
        break;
    }
}

void zUIIncrediblesInitINI(xIniFile* ini)
{
    sceneUnlockAll = xIniGetInt(ini, "UnlockAll", 0) != 0;

    Scene* sceneMain = zUIIncrediblesGetScene(0);
    strcpy(sceneMain->levelCode, "XXXX");
    sceneMain->flags = SCENEFLAGS_UNLOCKED | SCENEFLAGS_COMPLETED;
    sceneMain->expBonus = 0;

    numScenes = 1;
    numStoryScenes = 0;
    numArenaScenes = 0;
    allsoaklevels[numScenes - 1] = NULL;

    strcpy(sceneBaseName, xIniGetString(ini, "SceneBaseName", ""));
    strcpy(sceneBaseDescription, xIniGetString(ini, "SceneBaseDescription", ""));
    strcpy(sceneBaseImage, xIniGetString(ini, "SceneBaseImage", ""));
}

void zUIIncrediblesStartup()
{
    xtextbox::register_tags(newTags, ARRAY_LENGTH(newTags));
    zUIIncrediblesResetGame();
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("Debugging Options|Scene Flags|Completed|%s", "Debugging Options|Scene Flags|Unlocked|%s");
#endif

void zUIIncrediblesResetGame()
{
    unlockCheatOn = false;

    for (S32 i = 1; i < numScenes; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        scene->flags = sceneUnlockAll ? SCENEFLAGS_UNLOCKED : 0;
        scene->bonusCount = 0;
    }

    for (S32 i = 0; i < MOVIES_COUNT; i++)
        unlockedMovies[i] = sceneUnlockAll;

    for (U32 i = 0; i < zUIIncrediblesGet_TOTAL_BONUS(); i++)
        zUIIncrediblesSetUnlockedBonus(i, sceneUnlockAll);

    latestScene = sceneUnlockAll ? numScenes : 1;
}

void zUIIncrediblesSceneSetup()
{
    SCENE_STATUS_BONUS = xTextFindString("UI TXT SCENE STATUS BONUS", NULL);
}

U32 zUIIncrediblesGetTotalCheckpoints()
{
    return zUIIncrediblesGetSceneCheckPointTotal();
}

U32 zUIIncrediblesGetHitCheckpoints()
{
    return zUIIncrediblesGetSceneCheckPointHit();
}

void zUIIncrediblesWarp(xCounter* counter)
{
    xASSERT_L(741, counter->count != 0);
    zUIIncrediblesWarpDirect(zUISceneNumberFromCounter(counter));
}

void zUIIncrediblesWarpDirect(U32 sceneIndex)
{
    if (sceneIndex == 0 || sceneIndex >= (U32)numScenes) {
        xASSERT_FMT_L(749, sceneIndex < (U32)numScenes, "Scene Index out of bounds, max is %d", numScenes - 1);
        xASSERT_FMT_L(750, sceneIndex != 0, "Attempted warp to non-registered Scene. Please make sure the scene is listed in the ScenePlayerMapping section of the .ini file");
        return;
    }

    Scene* scene = zUIIncrediblesGetScene(sceneIndex);
    U32 sceneCode = IDTAG(scene->levelCode[0], scene->levelCode[1], scene->levelCode[2], scene->levelCode[3]);
    zUIIncrediblesWarp(sceneCode);
}

namespace {

bool zUIIncrediblesGameIsBeaten() { return false; }

} // namespace

void zUIIncrediblesWarp(U32 sceneCode)
{
    zLoadingScreen::UnSetShowStats();

    if (globals.demoType != zDT_NONE && globals.sceneCur->sceneID == IDTAG('M', 'N', 'U', 'S')) {
        globals.sceneStart[0] = (sceneCode & 0xFF000000) >> 24;
        globals.sceneStart[1] = (sceneCode & 0xFF0000) >> 16;
        globals.sceneStart[2] = (sceneCode & 0xFF00) >> 8;
        globals.sceneStart[3] = sceneCode & 0xFF;
    }

    static zPortal portal;
    static xPortalAsset portalAsset;
    memset(&portalAsset, 0, sizeof(portalAsset));
    portalAsset.baseType = eBaseTypePortal;
    portalAsset.assetCameraID = HASH_START;
    portalAsset.assetMarkerID = HASH_WARP_MARKER;
    portalAsset.sceneID = sceneCode;
    zMusicKill();
    xTextureManager::Get().FreeMemory();
    zPortalInit(&portal, &portalAsset);
    zSceneSwitch(&portal, 1, false, 0);
    if (sceneCode != SCENE_ID_MNU_START)
        zGameStateSwitchEvent(eEventDispatcher_SetGameState_SceneSwitch);
    else {
        if (zUIIncrediblesGameIsBeaten()) {
            gExitAfterPause = true;
        } else {
            zGameStateSwitchEvent(eEventDispatcher_SetGameState_Exit);
            zUIIncrediblesResetGame();
        }
    }
}

void zUIIncrediblesHitCheckPoint()
{
    zUIIncrediblesSetSceneCheckPointHit(zUIIncrediblesGetSceneCheckPointHit() + 1);
}

void zUIIncrediblesPlayedMovie(S32 index)
{
    if (index >= 0 && index < MOVIES_COUNT)
        unlockedMovies[index] = true;
}

void zUIIncrediblesOpenBonus(S32 index)
{
    xASSERT_L(819, index >= 0 && index < (S32)zUIIncrediblesGet_TOTAL_BONUS());
    if (!zUIIncrediblesGetUnlockedBonus(index)) {
        Scene* scene = zUIIncrediblesGetScene(zUIIncrediblesGetSceneCurrent());
        scene->bonusCount++;
    }
    zUIIncrediblesSetUnlockedBonus(index, true);
}

void zUIIncrediblesSave(xSerial* serial)
{
    return;

    for (S32 i = 1; i < numScenes; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        serial->Write(scene->flags);
        serial->Write(scene->bonusCount);
    }

    for (S32 i = 0; i < MOVIES_COUNT; i++) {
        U8 b = unlockedMovies[i] ? 1 : 0;
        serial->Write(b);
    }

    const U32 TOTAL_BONUS_CONST = zUIIncrediblesGet_TOTAL_BONUS(), INDEX_COUNT = (TOTAL_BONUS_CONST + 31) / 32;
    U32 index[1024];

    for (S32 i = 0; i < INDEX_COUNT; i++) {
        index[i] = 0;
    }

    for (S32 i = 0; i < zUIIncrediblesGet_TOTAL_BONUS(); i++) {
        if (zUIIncrediblesGetUnlockedBonus(i)) {
            index[i >> 5] |= 1 << (i & 31);
        }
    }

    for (S32 i = 0; i < INDEX_COUNT; i++) {
        serial->Write(index[i]);
    }

    serial->Write_b1(globals.invertJoystick);
}

static ePauseType pause_type;

ePauseType zUIIncrediblesGetPauseType() { return pause_type; }

void zUIIncrediblesSetPauseType(ePauseType type)
{
    pause_type = type;
}

void zUIIncrediblesLoad(xSerial* serial)
{
    return;

    for (S32 i = 1; i < numScenes; i++) {
        Scene* scene = zUIIncrediblesGetScene(i);
        serial->Read(&scene->flags);
        serial->Read(&scene->bonusCount);
    }

    for (S32 i = 0; i < MOVIES_COUNT; i++) {
        U8 b;
        serial->Read(&b);
        unlockedMovies[i] = b;
    }

    const U32 TOTAL_BONUS_CONST = zUIIncrediblesGet_TOTAL_BONUS(), INDEX_COUNT = (TOTAL_BONUS_CONST + 31) / 32;
    U32 index[1024];
    (void)INDEX_COUNT;

    for (S32 i = 0; i < INDEX_COUNT; i++) {
        serial->Read(&index[i]);
    }

    for (S32 i = 0; i < zUIIncrediblesGet_TOTAL_BONUS(); i++) {
        zUIIncrediblesSetUnlockedBonus(i, index[i >> 5] & (1 << (i & 31)));
    }

    S32 val = 0;
    serial->Read_b1(&val);
    globals.invertJoystick = val;
    zUIIncrediblesSetLatestSceneFromSave();
}

U32 zUIIncrediblesGetGameProgress()
{
    return zUIIncrediblesScenesCompletedInPercent() + zUIIncrediblesGoalsCompletedInPercent();
}

U32 zUIIncrediblesGetAutoSaveStage()
{
    if (globals.sceneCur->pendingPortal) {
        return globals.sceneCur->pendingPortal->passet->sceneID;
    }
    return globals.sceneCur->sceneID;
}
