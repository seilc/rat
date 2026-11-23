#ifndef ZECONOMICS_H
#define ZECONOMICS_H

#include "xIni.h"

struct xEnt;
struct xVec3;
struct zPlayer;

struct Threshold
{
    S32 low;
    S32 high;
};

struct AwardXP
{
    S32 normal;
    S32 below;
    S32 above;
};

class zEconomics
{
public:
    static void Init(xIniFile* ini);
    zEconomics();
    void ParseINI(xIniFile* ini);
    void CalcLastUpgradeValue();
    S32 GetPointsToUpgrade(S32 curPoints);
    S32 GetUpgradesEarned(S32 expPoints);
    void GivePlayerXPFromNPC(S32 NPCtype, xEnt* owner, xEnt* killer); // UNUSED
    S32 GetLevelCompletionBonus();
    void GiveLevelCompletionBonus();
    void GiveComboPoints(S32 enemyKilledCombo, zPlayer* player, xVec3);
    const char* GetLevelName(S32 level);
    const char* GetNPCName(S32 npc);
    S32 GetLevelNumber();
#if DEBUG || RELEASE
    S32 GetTotalPoints2UpgradeLevel(S32 level);
#endif

    static S32 instanceCount;
    static S32 lastUpgradeValue;

private:
    Threshold thresholdPointsRange[15];
    AwardXP NPCAwardXP[16][15];
    S32 missionCompleteBonus[15];
    AwardXP NPCCombo[10];
    S32 upgrades[12];
};

#endif
