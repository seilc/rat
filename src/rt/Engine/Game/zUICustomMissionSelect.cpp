#include "xCommon.h"
#include "zUICustomMissionSelect.h"

#include "xEvent.h"
#include "xString.h"
#include "xTextAsset.h"
#include "xstransvc.h"
#include "zDispatcher.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zUI.h"
#include "zUICustomLevelManager.h"
#include "zUICustomSysMessage.h"
#include "zUIImage.h"
#include "zUIIncredibles.h"
#include "zUIText.h"

#include <stdio.h>

zUICustomMissionSelect::zUICustomMissionSelect(zUI* ui) : zUICustom(ui)
{
    numStoryScenes = zUIIncrediblesGetNumStoryScenes();
    numArenaScenes = zUIIncrediblesGetNumArenaScenes();
}

void zUICustomMissionSelect::Init()
{
    zUICustom::Init();
}

void zUICustomMissionSelect::Exit()
{
    zUICustom::Exit();
}

// UNUSED: needed to match (speculative code)
const char* nameTextFormat = "WORLD UIF %02d";
// UNUSED: needed to match (speculative code)
const char* descTextFormat = "MISSION UIF %02d";
const char* bubbleTextFormat = "WARP_BUBBLE UII %02d";
const char* glowTextFormat = "WARP_BUBBLE GLOW UII %02d";

void zUICustomMissionSelect::Setup()
{
    zUICustom::Setup();

    counter = (xCounter*)zSceneFindObject(xStrHash("UI MISSION COUNTER"));

    activeDesc = (zUIText*)zSceneFindObject(xStrHash("MISSION UIF 01"));
    inactiveDesc = (zUIText*)zSceneFindObject(xStrHash("MISSION UIF 02"));

    activeName = (zUIText*)zSceneFindObject(xStrHash("WORLD UIF 01"));
    inactiveName = (zUIText*)zSceneFindObject(xStrHash("WORLD UIF 02"));

    levelGoals = (zUIText*)zSceneFindObject(xStrHash("UI GPAUSE CURRENT SCENE CHECKMARK"));

    char temp[64];
    S16 i;
    for (i = 0; i < MAX_BUBBLES; i++) {
        sprintf(temp, glowTextFormat, i + 1);
        blinkyLights[i] = (zUIImage*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, bubbleTextFormat, i + 1);
        bubbles[i] = (zUIImage*)zSceneFindObject(xStrHash(temp));
    }

    bubblesExtra[0] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE UII 15"));
    bubblesExtra[1] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE UII 16"));
    bubblesExtra[2] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE UII 17"));
    bubblesExtraGlow[0] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE GLOW UII 15"));
    bubblesExtraGlow[1] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE GLOW UII 16"));
    bubblesExtraGlow[2] = (zUIImage*)zSceneFindObject(xStrHash("WARP_BUBBLE GLOW UII 17"));

    char* dir[4] = { "UP", "DOWN", "RIGHT", "LEFT" };
    const char* arrowFormat = "UII ARROW %s%s";
    for (i = 0; i < 4; i++) {
        sprintf(temp, arrowFormat, "", dir[i]);
        arrows[i] = (zUIImage*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, arrowFormat, "GLOW ", dir[i]);
        arrowGlows[i] = (zUIImage*)zSceneFindObject(xStrHash(temp));
    }

    soundFXDeny = zSceneFindObject(xStrHash("SFX_ERROR"));
    soundFXPing = zSceneFindObject(xStrHash("SFX_LEFT"));
    soundFXConfirm = zSceneFindObject(xStrHash("SFX_CONFIRM"));

    motionOutLeft = (xBase*)xSTFindAsset(xStrHash("UIM FADE OUT, LEFT 03"), NULL);
    motionInLeft = (xBase*)xSTFindAsset(xStrHash("UIM FADE IN, LEFT 03"), NULL);
    motionOutRight = (xBase*)xSTFindAsset(xStrHash("UIM FADE OUT, RIGHT 03"), NULL);
    motionInRight = (xBase*)xSTFindAsset(xStrHash("UIM FADE IN, RIGHT 03"), NULL);

    motionOutUp = (xBase*)xSTFindAsset(xStrHash("UIM FADE OUT, DOWN 03"), NULL);
    motionInUp = (xBase*)xSTFindAsset(xStrHash("UIM FADE IN, DOWN 03"), NULL);
    motionOutDown = (xBase*)xSTFindAsset(xStrHash("UIM FADE OUT, UP 03"), NULL);
    motionInDown = (xBase*)xSTFindAsset(xStrHash("UIM FADE IN, UP 03"), NULL);

    motionInvisible = (xBase*)xSTFindAsset(xStrHash("UIM INVISIBLE"), NULL);
    motionBlink = (xBase*)xSTFindAsset(xStrHash("UIM BLINK 02"), NULL);
}

void zUICustomMissionSelect::Reset()
{
    zUICustom::Reset();
}

void zUICustomMissionSelect::PreUpdate(F32 dt)
{
    zUICustom::PreUpdate(dt);

    return;

    for (S32 i = numStoryScenes + numArenaScenes / 3; i < MAX_BUBBLES; i++)
        bubbles[i]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);

    for (S32 j = 0; j < 3; j++)
        bubblesExtraGlow[j]->visible = false;

    if (selectedStage <= numStoryScenes) {
        for (S32 j = 0; j < 3; j++)
            bubblesExtra[j]->visible = false;

        arrows[0]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
        arrows[1]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
    } else if (selectedStage >= numStoryScenes + 1) {
        if (ui->focus)
            for (S32 j = 0; j < 3; j++)
                bubblesExtra[j]->visible = true;

        arrows[0]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);
        arrows[1]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

        if (selectedMission % 3 == 0)
            arrows[0]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
        else
            arrows[0]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

        if (selectedMission % 3 == 2)
            arrows[1]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
        else
            arrows[1]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

        if (ui->focus)
            bubblesExtraGlow[selectedMission % 3]->visible = true;
    }

    if (selectedStage == 1)
        arrows[3]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
    else
        arrows[3]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

    if (selectedStage == numStoryScenes + numArenaScenes / 3)
        arrows[2]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
    else
        arrows[2]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

    FormatLevelGoals(descBuff);
    levelGoals->SetText(descBuff);
}

xBase* zUICustomMissionSelect::GetColorMotion(bool completed, bool unlocked)
{
    U32 size = 0;
    if (completed)
        return (xBase*)xSTFindAsset(xStrHash("_UIM RGB GREEN 01"), &size);
    else if (unlocked)
        return (xBase*)xSTFindAsset(xStrHash("_UIM RGB ORANGE 01"), &size);
    else
        return (xBase*)xSTFindAsset(xStrHash("_UIM RGB RED 01"), &size);
}

xBase* zUICustomMissionSelect::GetColorMotionScene(U32 idx)
{
    return GetColorMotion(zUIIncrediblesAllGoalsCompleted(idx), zUIIncrediblesIsSceneUnlocked(idx));
}

void zUICustomMissionSelect::GetArenasLevelCompletion(S32 i, bool& scene1Comp, bool& scene2Comp, bool& scene3Comp,
                                                      bool& scene1Unlock, bool& scene2Unlock, bool& scene3Unlock)
{
    S32 j = countFromSelectedStage(i);
    xASSERT_L(211, j > 0);
    scene1Comp = zUIIncrediblesAllGoalsCompleted(j);
    scene2Comp = zUIIncrediblesAllGoalsCompleted(j + 1);
    scene3Comp = zUIIncrediblesAllGoalsCompleted(j + 2);
    scene1Unlock = zUIIncrediblesIsSceneUnlocked(j);
    scene2Unlock = zUIIncrediblesIsSceneUnlocked(j + 1);
    scene3Unlock = zUIIncrediblesIsSceneUnlocked(j + 2);
}

void zUICustomMissionSelect::SetLevelBubblesExtraState(S32 i, bool& allComplete, bool& allUnlocked,
                                                       bool setExtraBubbles)
{
    bool scene1Comp, scene2Comp, scene3Comp, scene1Unlock, scene2Unlock, scene3Unlock;
    GetArenasLevelCompletion(i, scene1Comp, scene2Comp, scene3Comp, scene1Unlock, scene2Unlock, scene3Unlock);

    if (setExtraBubbles) {
        xBase* motionExtra = GetColorMotion(scene1Comp, scene1Unlock);
        zEntEvent(bubblesExtra[0], eEventUISetMotion, NULL, motionExtra, motionExtra->id);
        zEntEvent(bubblesExtraGlow[0], eEventUISetMotion, NULL, motionExtra, motionExtra->id);

        motionExtra = GetColorMotion(scene2Comp, scene2Unlock);
        zEntEvent(bubblesExtra[1], eEventUISetMotion, NULL, motionExtra, motionExtra->id);
        zEntEvent(bubblesExtraGlow[1], eEventUISetMotion, NULL, motionExtra, motionExtra->id);

        motionExtra = GetColorMotion(scene3Comp, scene3Unlock);
        zEntEvent(bubblesExtra[2], eEventUISetMotion, NULL, motionExtra, motionExtra->id);
        zEntEvent(bubblesExtraGlow[2], eEventUISetMotion, NULL, motionExtra, motionExtra->id);
    }

    allComplete = scene1Comp && scene2Comp && scene3Comp;
    allUnlocked = scene1Unlock || scene2Unlock || scene3Unlock;
}

void zUICustomMissionSelect::SetLevelImageState()
{
    return;

    xBase* motion;
    for (S32 i = 1; i <= MAX_BUBBLES; i++) {
        motion = GetColorMotionScene(i);
        if (i >= numStoryScenes + 1) {
            bool allComplete = false, allUnlocked = false;
            SetLevelBubblesExtraState(i, allComplete, allUnlocked, selectedStage == i);
            motion = GetColorMotion(allComplete, allUnlocked);
        }

        zEntEvent(blinkyLights[i - 1], eEventUISetMotion, NULL, motion, motion->id);
        zEntEvent(bubbles[i - 1], eEventUISetMotion, NULL, motion, motion->id);
    }
}

bool zUICustomMissionSelect::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                                         U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventUIMotionFinishedIn:
        SetLevelImageState();
        break;
    case eEventPadPressStart:
    case eEventPadPressTriangle:
    case eEventUIFocusOff_Unselect:
        break;
    case eEventPadPressUp:
    case eEventPadPressDown:
        if (selectedStage > numStoryScenes) {
            if ((selectedMission == 0 && toEvent == eEventPadPressUp) ||
                (selectedMission == 2 && toEvent == eEventPadPressDown)) {
                zEntEvent(soundFXDeny, eEventPlay);
                break;
            }

            zEntEvent(soundFXPing, eEventPlay);

            F32 params[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
            zEntEvent(arrowGlows[toEvent - eEventPadPressUp], eEventUISetMotion, params, motionBlink, motionBlink->id);

            selectedMission += toEvent == eEventPadPressUp ? -1 : 1;

            xBase* motionOut = toEvent == eEventPadPressUp ? motionOutUp : motionOutDown;
            xBase* motionIn = toEvent == eEventPadPressUp ? motionInUp : motionInDown;

            S32 count = countFromSelectedStage(selectedStage) + selectedMission;
            counter->count = count;

            SetLevelGoalsState(count);

            inactiveDesc->SetText(zUIIncrediblesGetScene(count)->levelDescription);
            zEntEvent(activeDesc, eEventUISetMotion, params, motionOut, motionOut->id);
            zEntEvent(inactiveDesc, eEventUISetMotion, params, motionIn, motionIn->id);

            zUIText* temp = activeDesc;
            activeDesc = inactiveDesc;
            inactiveDesc = temp;
        }
        break;
    case eEventPadPressRight:
    case eEventPadPressLeft:
    {
        if ((toEvent == eEventPadPressLeft && selectedStage <= 1) ||
            (toEvent == eEventPadPressRight && selectedStage >= numStoryScenes + numArenaScenes / 3)) {
            zEntEvent(soundFXDeny, eEventPlay);
            break;
        }

        selectedMission = 0;
        blinkyLights[selectedStage - 1]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
        zEntEvent(soundFXPing, eEventPlay);

        F32 params[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
        zEntEvent(arrowGlows[toEvent - eEventPadPressUp], eEventUISetMotion, params, motionBlink, motionBlink->id);

        selectedStage += toEvent == eEventPadPressLeft ? -1 : 1;

        U16 count = countFromSelectedStage(selectedStage) + selectedMission;
        counter->count = count;
        SetLevelGoalsState(count);

        xBase* motion = GetColorMotionScene(selectedStage);
        bool allComplete = false;
        bool allUnlocked = false;
        SetLevelBubblesExtraState(selectedStage, allComplete, allUnlocked, selectedStage >= numStoryScenes + 1);

        blinkyLights[selectedStage - 1]->HandleEvent(NULL, eEventVisible, NULL, NULL, 0);

        inactiveDesc->SetText(zUIIncrediblesGetScene(count)->levelDescription);
        inactiveName->SetText(zUIIncrediblesGetScene(count)->levelName);

        xBase* motionOut = toEvent == eEventPadPressLeft ? motionOutLeft : motionOutRight;
        xBase* motionIn = toEvent == eEventPadPressLeft ? motionInLeft : motionInRight;
        zEntEvent(activeName, eEventUISetMotion, params, motionOut, motionOut->id);
        zEntEvent(inactiveName, eEventUISetMotion, params, motionIn, motionIn->id);

        motionOut = toEvent == eEventPadPressLeft ? motionOutUp : motionOutDown;
        motionIn = toEvent == eEventPadPressLeft ? motionInUp : motionInDown;
        zEntEvent(inactiveDesc, eEventUISetMotion, params, motionIn, motionIn->id);
        zEntEvent(activeDesc, eEventUISetMotion, params, motionOut, motionOut->id);

        zUIText* temp = activeDesc;
        activeDesc = inactiveDesc;
        inactiveDesc = temp;

        temp = activeName;
        activeName = inactiveName;
        inactiveName = temp;
        break;
    }
    case eEventVisible:
        break;
    case eEventInvisible:
        for (U16 i = 0; i < MAX_BUBBLES; i++)
            blinkyLights[i]->HandleEvent(NULL, eEventInvisible, NULL, NULL, 0);
        break;
    case eEventPadPressX:
        if (zUIIncrediblesSceneIsUnlocked(counter->count)) {
            zEntEvent(soundFXConfirm, eEventPlay);
            F32 toParams[4] = { counter->count };
            if (globals.sceneCur->sceneID == IDTAG('M','N','U','S')) {
                ZDSP_elcb_event(from, from, eEventWarpGo, toParams, NULL, 0);
            } else {
                U32 missionTextHash = 0;
                if (globals.demoType != zDT_NONE)
                    missionTextHash = xStrHash("UI TXT SYS MSG CHANGE MISSION E3");
                else
                    missionTextHash = xStrHash("UI TXT SYS MSG CHANGE MISSION");
                xASSERT_L(448, missionTextHash);

                pSysMessageManager->Signal(ui, eEventUISysMessageWaitResponse);
                ((zUICustomSysMessageManager*)pSysMessageManager_Custom)->ShowSystemMessage(
                    ZDSP_elcb_event, eEventUISysMessageWaitResponse, eEventWarpGo, toParams, from, NULL, 0,
                    xTextFindString(missionTextHash, NULL));
                ((zUI*)from)->Signal(ui, eEventUIFocusOff_Unselect);
            }
        } else
            zEntEvent(soundFXDeny, eEventPlay);
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomMissionSelect::SetLevelGoalsState(S32 i)
{
    xBase* picImage = zSceneFindObject(xStrHash("UI GPAUSE PIC IMAGE"));
    xBase* conHider = zSceneFindObject(xStrHash("UI GPAUSE CURRENT SCENE HIDER"));
    xASSERT_L(476, picImage);
    xASSERT_L(477, conHider);
    if (!zUIIncrediblesAllGoalsCompleted(i) && !zUIIncrediblesIsSceneUnlocked(i)) {
        zEntEvent(picImage, eEventInvisible);
        zEntEvent(conHider, eEventInvisible);
    } else {
        zEntEvent(picImage, eEventVisible);
        zEntEvent(conHider, eEventVisible);
    }
}

S32 zUICustomMissionSelect::countFromSelectedStage(S32 stage)
{
    return stage <= numStoryScenes + 1 ? stage : numStoryScenes + 1 + (stage - numStoryScenes - 1) * 3;
}

// UNUSED: needed to match (code from RotU GC)
void zUICustomMissionSelect::FormatLevelGoals(char* buffer)
{
    buffer[0] = '\0';
    S32 count = counter->count;
    Scene* sceneCur = zUIIncrediblesGetScene(count);
    const char* checkBox = xTextFindString(xStrHash("UI CHECKBOX ICON"), NULL);
    const char* uncheckBox = xTextFindString(xStrHash("UI UNCHECKBOX ICON"), NULL);
    sprintf(buffer, "{n}%s{n}%s{n} {n}%s{n}%s", sceneCur->flags & 0x4 ? checkBox : uncheckBox,
            sceneCur->flags & 0x8 ? checkBox : uncheckBox, sceneCur->flags & 0x10 ? checkBox : uncheckBox,
            sceneCur->flags & 0x20 ? checkBox : uncheckBox);
}
