#include "xCommon.h"
#include "zEconomics.h"

#include "xGlobals.h"
#include "xutil.h"
#include "zCommonPlayer.h"
#include "zGlobals.h"
#include "zScene.h"

#include <stdio.h>

S32 zEconomics::instanceCount;
S32 zEconomics::lastUpgradeValue;

void zEconomics::Init(xIniFile* ini)
{
    static zEconomics economics;
    globals.economics = &economics;
    economics.ParseINI(ini);
    economics.CalcLastUpgradeValue();
}

zEconomics::zEconomics()
{
    instanceCount++;
    xASSERT_FMT_L(29, instanceCount == 1, "Only one instance of zEconomics allowed");
}

void zEconomics::ParseINI(xIniFile* ini)
{
    char normal[64], below[64], above[64], low[64], high[64], upgrade[64], bonus[64];

    for (S32 i = 0; i < MAX_COMBOS; i++) {
        sprintf(normal, "Combo%d.Normal", i);
        sprintf(below, "Combo%d.Below", i);
        sprintf(above, "Combo%d.Above", i);

        NPCCombo[i].normal = xIniGetInt(ini, normal, 0);
        NPCCombo[i].below = xIniGetInt(ini, below, 0);
        NPCCombo[i].above = xIniGetInt(ini, above, 0);
    }

    for (S32 level = 0; level < MAX_LEVELS; level++) {
        const char* levelName = GetLevelName(level);

        for (S32 npc = 1; npc < MAX_NPC_TYPES; npc++) {
            const char* NPCName = GetNPCName(npc);

            sprintf(normal, "%s.%s.Normal", NPCName, levelName);
            sprintf(below, "%s.%s.Below", NPCName, levelName);
            sprintf(above, "%s.%s.Above", NPCName, levelName);

            NPCAwardXP[npc][level].normal = xIniGetInt(ini, normal, 0);
            NPCAwardXP[npc][level].below = xIniGetInt(ini, below, 0);
            NPCAwardXP[npc][level].above = xIniGetInt(ini, above, 0);
        }

        sprintf(low, "%s.threshold.low", levelName);
        sprintf(high, "%s.threshold.high", levelName);

        thresholdPointsRange[level].low = xIniGetInt(ini, low, 0);
        thresholdPointsRange[level].high = xIniGetInt(ini, high, 0);

        sprintf(bonus, "%s.bonus", levelName);

        missionCompleteBonus[level] = xIniGetInt(ini, bonus, 0);
    }

    for (S32 i = 0; i < MAX_UPGRADES; i++) {
        sprintf(upgrade, "Upgrade%d", i + 1);
        upgrades[i] = xIniGetInt(ini, upgrade, 0);
    }
}

void zEconomics::CalcLastUpgradeValue()
{
    lastUpgradeValue = 0;
    for (S32 i = 0; i < MAX_UPGRADES; i++) {
        lastUpgradeValue += upgrades[i];
    }
}

S32 zEconomics::GetPointsToUpgrade(S32 curPoints)
{
    S32 points = 0;
    S32 i = 0;

    while (points <= curPoints && i < MAX_UPGRADES) {
        points += upgrades[i];
        i++;
    }

    return points - curPoints;
}

S32 zEconomics::GetUpgradesEarned(S32 expPoints)
{
    S32 points = 0;
    S32 earnedUpgrades = 0;

    for (S32 i = 0; i < MAX_UPGRADES; i++) {
        if (points + upgrades[i] > expPoints)
            break;

        points += upgrades[i];
        earnedUpgrades++;
    }

    return earnedUpgrades;
}

S32 zEconomics::GetLevelCompletionBonus()
{
    S32 level = GetLevelNumber();

    return missionCompleteBonus[level];
}

void zEconomics::GiveLevelCompletionBonus()
{
    return;

    S32 level = GetLevelNumber(), expBonus = missionCompleteBonus[level];

    xASSERT(CURPLAYERS.size() == 1);

    S32 expBonusToGive = expBonus, currentExp = ((zCommonPlayer*)CURPLAYERS[0])->GetExperiencePoints();
    if (currentExp + expBonus > lastUpgradeValue) {
        expBonusToGive = lastUpgradeValue - currentExp;
    }
    ((zCommonPlayer*)CURPLAYERS[0])->GiveExperiencePoints(expBonusToGive);
}

void zEconomics::GiveComboPoints(S32 enemyKilledCombo, zPlayer* player, xVec3)
{
    S32 level, currentExp, expPoints, low, high;

    return;

    level = GetLevelNumber();
    currentExp = ((zCommonPlayer*)player)->GetExperiencePoints();
    low = thresholdPointsRange[level].low;
    high = thresholdPointsRange[level].high;

    xASSERT(enemyKilledCombo >= 0 && enemyKilledCombo < MAX_COMBOS);

    if (currentExp < low) {
        expPoints = NPCCombo[enemyKilledCombo].below;
    } else if (currentExp > high) {
        expPoints = NPCCombo[enemyKilledCombo].above;
    } else {
        expPoints = NPCCombo[enemyKilledCombo].normal;
    }

    {
        S32 currentExp = ((zCommonPlayer*)player)->GetExperiencePoints();
        if (currentExp + expPoints > lastUpgradeValue) {
            expPoints = lastUpgradeValue - currentExp;
        }
        ((zCommonPlayer*)player)->GiveExperiencePoints(expPoints);
    }
}

const char* zEconomics::GetLevelName(S32 level)
{
    const char* name;

    switch (level) {
    case 0:
        name = "UT";
        break;

    case 1:
        name = "DD";
        break;

    case 2:
        name = "MT";
        break;

    case 3:
        name = "BB";
        break;

    case 4:
        name = "B1";
        break;

    case 5:
        name = "MM";
        break;

    case 6:
        name = "RF";
        break;

    case 7:
        name = "B2";
        break;

    case 8:
        name = "UW";
        break;

    case 9:
        name = "CC";
        break;

    case 10:
        name = "B3";
        break;

    case 11:
        name = "A1";
        break;

    case 12:
        name = "A2";
        break;

    case 13:
        name = "A3";
        break;

    default:
        name = "??";
        break;
    }

    return name;
}

const char* zEconomics::GetNPCName(S32 npc)
{
    const char* name;

    switch (npc) {
    default:
        name = "??";
        break;
    }

    return name;
}

S32 zEconomics::GetLevelNumber()
{
    S32 level;
    U32 sceneID = globals.sceneCur->sceneID >> 16;

    switch (sceneID) {
    case IDTAG(0, 0, 'U', 'T'):
        level = 0;
        break;

    case IDTAG(0, 0, 'D', 'D'):
        level = 1;
        break;

    case IDTAG(0, 0, 'M', 'T'):
        level = 2;
        break;

    case IDTAG(0, 0, 'B', 'B'):
        level = 3;
        break;

    case IDTAG(0, 0, 'B', '1'):
        level = 4;
        break;

    case IDTAG(0, 0, 'M', 'M'):
        level = 5;
        break;

    case IDTAG(0, 0, 'R', 'F'):
        level = 6;
        break;

    case IDTAG(0, 0, 'B', '2'):
        level = 7;
        break;

    case IDTAG(0, 0, 'U', 'W'):
        level = 8;
        break;

    case IDTAG(0, 0, 'C', 'C'):
        level = 9;
        break;

    case IDTAG(0, 0, 'B', '3'):
        level = 10;
        break;

    case IDTAG(0, 0, 'A', '1'):
        level = 11;
        break;

    case IDTAG(0, 0, 'A', '2'):
        level = 12;
        break;

    case IDTAG(0, 0, 'A', '3'):
        level = 13;
        break;

    default:
        level = 14;
        break;
    }

    return level;
}

#if DEBUG || RELEASE
S32 zEconomics::GetTotalPoints2UpgradeLevel(S32 level)
{
    xASSERT_L(468, level <= MAX_UPGRADES);

    S32 count = 0;

    for (S32 i = 0; i < level; i++)
        count += upgrades[i];

    return count;
}
#endif
