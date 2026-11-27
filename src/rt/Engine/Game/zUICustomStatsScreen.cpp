#include "xCommon.h"
#include "zUICustomStatsScreen.h"

#include "xEvent.h"
#include "xFile.h"
#include "xSndMgr.h"
#include "xString.h"
#include "xTextAsset.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zPlayerContainer.h"
#include "zScene.h"
#include "zUI.h"
#include "zUIController.h"
#include "zUICustomLevelManager.h"
#include "zUIIncredibles.h"
#include "zUIText.h"

#include "decomp.h"

#include <stdio.h>

namespace {
S32 cheapCreditsCheat;
}

static const en_zStatistics PlayerStats[] = {
    eStatMeleeDamage,
    eStatAirMeleeDamage,
    eStatRangeDamage,
    eStatDodgeDamage,
    eStatIncrediMoves,
    eStatKilled,
    eStatDeaths,
    eStatCount
};

zUICustomStatsScreen::zUICustomStatsScreen(zUI* ui) : zUICustom(ui)
{
    statCycleSound = XSNDMGR_INVALID_SND_HANDLE;
}

void zUICustomStatsScreen::Init()
{
    zUICustom::Init();
    totalTime = 0.0f;
    lastLine = -1;
    doneUpdating = false;
}

void zUICustomStatsScreen::Setup()
{
    zUICustom::Setup();

    char temp[128];

    for (S32 i = 0; i < NUM_STATS_LINES; i++) {
        sprintf(temp, "UI STATS LEFT TEXT %d", i + 1);
        stats1[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI STATS RIGHT TEXT %d", i + 1);
        stats2[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI STATS LEFT NUMBERS %d", i + 1);
        numbers1[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI STATS RIGHT NUMBERS %d", i + 1);
        numbers2[i] = (zUIText*)zSceneFindObject(xStrHash(temp));
    }

    totalTime = 0.0f;
    doneUpdating = false;
    lastLine = -1;
}

void zUICustomStatsScreen::Reset()
{
    zUICustom::Reset();
    totalTime = 0.0f;
    doneUpdating = false;
    lastLine = -1;
}

void zUICustomStatsScreen::Exit()
{
    DeAllocateBuffers();
    zUICustom::Exit();
}

void zUICustomStatsScreen::PreUpdate(F32 dt)
{
    zUICustom::PreUpdate(dt);
    S32 CurLine = totalTime;
    totalTime += 1.5f * dt;
    if (PlayerStats[CurLine] == eStatCount) {
        doneUpdating = true;
    }

    doneUpdating = true;

    if (!doneUpdating) {
        F32 fraction = totalTime - CurLine;

        FormatTextLines(stats1, 0, CurLine);
        FormatTextLines(stats2, 1, CurLine);

        FormatPrevNumberLines(numbers1, numBuff1, 0, CurLine);
        FormatPrevNumberLines(numbers2, numBuff2, 1, CurLine);

        bool MRICounting = FormatLastNumberLine(numbers1, numBuff1, 0, CurLine, fraction);
        bool FROCounting = FormatLastNumberLine(numbers2, numBuff2, 1, CurLine, fraction);

        if (CurLine != lastLine) {
            xSndMgrPlay(xSndMgrGetSoundGroup("STATS_STOP_SGRP"), 0, NULL, NULL, NULL, NULL, NULL);
        }

        if (xSndMgrIsPlaying(statCycleSound)) {
            if (!MRICounting && !FROCounting)
                xSndMgrStop(statCycleSound);
        }
        else if (MRICounting || FROCounting) {
            statCycleSound = xSndMgrPlay(xSndMgrGetSoundGroup("STATS_CYCLE_SGRP"), 0, NULL, NULL, NULL, NULL, NULL);
        }

        lastLine = CurLine;
    } else {
        xSndMgrStop(statCycleSound);
        zUIText* acceptText = (zUIText*)zSceneFindObject(xStrHash("UI STATS CONTINUE TEXT"));
        xASSERT_L(156, acceptText);
        zEntEvent(acceptText, eEventVisible);
    }
}

void zUICustomStatsScreen::FormatTextLines(zUIText** lines, S32 playerIndex, S32 curLine)
{
    char temp[128];
    S32 i;
    const char* playerString = (playerIndex == 0) ? "MRI" : "FRO";

    for (i = 0; i <= curLine; i++) {
        if (i != 6)
            sprintf(temp, "%s STAT STRING %d", playerString, i);
        else
            sprintf(temp, "%s STAT STRING %d", playerString, i + 1);

        const char* line = xTextFindString(xStrHash(temp), NULL);

        if (!line) {
            break;
        }

        lines[i]->SetText(line);
        lines[i]->SetCached(false);
    }
}

void zUICustomStatsScreen::FormatPrevNumberLines(zUIText** lines, char (*buffers)[10], S32 playerIndex, S32 curLine)
{
    for (S32 i = 0; i < curLine; i++) {
        if (PlayerStats[i] == eStatCount) {
            break;
        }

        S32 stat = GetStat(PlayerStats[i], playerIndex);

        sprintf(buffers[i], "%d", stat);
        lines[i]->SetText(buffers[i]);
        lines[i]->SetCached(false);
    }
}

bool zUICustomStatsScreen::FormatLastNumberLine(zUIText** lines, char (*buffers)[10], S32 playerIndex, S32 curLine, F32 fraction)
{
    S32 realStat, dispStat;
    bool stillCounting = true;

    if (PlayerStats[curLine] == eStatCount) {
        return false;
    }

    realStat = GetStat(PlayerStats[curLine], playerIndex);

    if (realStat < 2) {
        dispStat = 10.0f * (fraction - 0.15f);
        if (dispStat < 0)
            dispStat = 0;
    }
    else if (realStat < 10.0f) {
        dispStat = 10.0f * fraction;
    } else {
        dispStat = fraction * realStat;
    }

    if (dispStat >= realStat) {
        dispStat = realStat;
        stillCounting = false;
    }

    sprintf(buffers[curLine], "%d", dispStat);
    lines[curLine]->SetText(buffers[curLine]);
    lines[curLine]->SetCached(false);
    return stillCounting;
}

S32 zUICustomStatsScreen::GetStat(en_zStatistics statIndex, S32 playerIndex)
{
    switch (statIndex) {
    case eStatMeleeDamage:
        return CURPLAYERS[playerIndex]->playStats.meleeDamage;
    case eStatAirMeleeDamage:
        return CURPLAYERS[playerIndex]->playStats.airMeleeDamage;
    case eStatRangeDamage:
        return CURPLAYERS[playerIndex]->playStats.rangeDamage;
    case eStatDodgeDamage:
        return CURPLAYERS[playerIndex]->playStats.dodgeDamage;
    case eStatIncrediMoves:
        return CURPLAYERS[playerIndex]->playStats.finishingMoves;
    case eStatKilled:
        return CURPLAYERS[playerIndex]->playStats.enemiesDefeated;
    case eStatDeaths:
        return CURPLAYERS[playerIndex]->playStats.timesDefeated;
    case eStatDamageTaken:
        return CURPLAYERS[playerIndex]->playStats.damageTaken;
    default:
        xFAIL_MSG_L(268, "Requested invalid statistic");
        return -1;
    }
}

bool zUICustomStatsScreen::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventPadPressX:
        if (!doneUpdating) {
            return false;
        }
        break;
    case eEventVisible:
    {
        zUIText* acceptText = (zUIText*)zSceneFindObject(xStrHash("UI STATS CONTINUE TEXT"));
        xASSERT_L(286, acceptText);
        zEntEvent(acceptText, eEventInvisible);
        if (!ui->visible)
            AllocateBuffers();
        break;
    }
    case eEventInvisible:
        if (ui->visible) {
            DeAllocateBuffers();
        }
        break;
    default:
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void zUICustomStatsScreen::AllocateBuffers()
{
}

void zUICustomStatsScreen::DeAllocateBuffers()
{
}

zUICustomParrBestScreen* pUICustomParrBestScreen;

zUICustomParrBestScreen::zUICustomParrBestScreen(zUI* ui) : zUICustom(ui)
{
}

void zUICustomParrBestScreen::Init()
{
    zUICustom::Init();
    totalTime = 0.0f;
    doneUpdating = false;
}

void zUICustomParrBestScreen::Exit()
{
    DeAllocateBuffers();
    zUICustom::Exit();
}

void zUICustomParrBestScreen::Setup()
{
    zUICustom::Setup();

    char temp[128];

    for (S32 i = 0; i < NUM_PARRBEST_LINES; i++) {
        sprintf(temp, "UI PARRBEST POINTS TEXT %d", i + 1);
        scoreText[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI PARRBEST POINTS NUM %d", i + 1);
        scoreNum[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI PARRBEST GOAL TEXT %d", i + 1);
        goalText[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI PARRBEST GOAL NUM %d", i + 1);
        goalNum[i] = (zUIText*)zSceneFindObject(xStrHash(temp));

        sprintf(temp, "UI PARRBEST STATUS %d", i + 1);
        statusText[i] = (zUIText*)zSceneFindObject(xStrHash(temp));
    }

    totalTime = 0.0f;
    doneUpdating = false;
}

void zUICustomParrBestScreen::Reset()
{
    zUICustom::Reset();
    totalTime = 0.0f;
    doneUpdating = false;
}

void zUICustomParrBestScreen::PreUpdate(F32 dt)
{
    zUICustom::PreUpdate(dt);
}

bool zUICustomParrBestScreen::HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    switch (toEvent) {
    case eEventPadPressX:
        if (globals.sceneCur->sceneID != IDTAG('B','3','0','1')) {
            if (!doneUpdating) {
                return false;
            }
        } else {
            zUIController* ref = (zUIController*)zSceneFindObject(xStrHash("UI CREDITS FINAL HELPER"));
            xASSERT_L(413, ref);

            bool cheapCredits = true;
            if (cheapCreditsCheat == 0) {
                if (zUIIncrediblesAllCompleted())
                    cheapCredits = false;
            }
            else if (cheapCreditsCheat == 1) {
                cheapCredits = false;
            }

            if (cheapCredits)
                zEntEvent(ref, eEventTalkBox_OnSignal1);
            else
                zEntEvent(ref, eEventTalkBox_OnSignal2);
            return false;
        }
    case eEventVisible:
        if (!ui->visible)
            AllocateBuffers();
        break;
    case eEventUISignalDeactivatedScreen:
        doneUpdating = false;
        break;
    case eEventUISignalActivatedScreen:
        SetupText();
        doneUpdating = true;
        break;
    case eEventInvisible:
        if (ui->visible) {
            DeAllocateBuffers();
        }
        break;
    }

    return zUICustom::HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

bool zUICustomParrBestScreen::IsSignalAllowed(U32 toEvent)
{
    switch (toEvent) {
    case eEventPadPressX:
        if (!doneUpdating) {
            return false;
        }
        break;
    }

    return true;
}

void zUICustomParrBestScreen::SetCompletedGoalsFlags()
{
    return;

    S32 points, goal;
    bool passed;
    U32 targetFlag;
    Scene* sceneCur;
    S32 sceneFlags, i;
    DECOMP_FORCEREGISTER(points);
    DECOMP_FORCEREGISTER(goal);
    DECOMP_FORCEREGISTER(passed);
    DECOMP_FORCEREGISTER(passed);
    (void)passed;
    DECOMP_FORCEREGISTER(targetFlag);
    (void)targetFlag;
    DECOMP_FORCEREGISTER(sceneCur);
    DECOMP_FORCEREGISTER(sceneCur);
    DECOMP_FORCEREGISTER(sceneFlags);
    DECOMP_FORCEREGISTER(sceneFlags);
    (void)sceneFlags;
    DECOMP_FORCEREGISTER(i);
    DECOMP_FORCEREGISTER(i);
    DECOMP_FORCEREGISTER(i);
    (void)i;
    (void)this;
    (void)(passed ? points : goal);
}

void zUICustomParrBestScreen::SetupText()
{
    doneUpdating = true;
    return;

    S32 points, goal;
    bool passed;
    Scene* sceneCur;
    S32 sceneFlags, i;
    DECOMP_FORCEREGISTER(points);
    DECOMP_FORCEREGISTER(goal);
    DECOMP_FORCEREGISTER(passed);
    DECOMP_FORCEREGISTER(passed);
    DECOMP_FORCEREGISTER(sceneCur);
    DECOMP_FORCEREGISTER(sceneCur);
    DECOMP_FORCEREGISTER(sceneFlags);
    DECOMP_FORCEREGISTER(sceneFlags);
    DECOMP_FORCEREGISTER(sceneFlags);
    (void)sceneFlags;
    DECOMP_FORCEREGISTER(i);
    DECOMP_FORCEREGISTER(i);
    DECOMP_FORCEREGISTER(i);
    DECOMP_FORCEREGISTER(i);
    (void)i;
    (void)this;
    (void)(passed ? points : goal);
    (void)(passed ? points : goal);
    (void)(passed ? points : goal);
    (void)(passed ? points : goal);
}

void zUICustomParrBestScreen::AllocateBuffers()
{
}

void zUICustomParrBestScreen::DeAllocateBuffers()
{
}
