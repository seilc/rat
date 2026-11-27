#ifndef ZUICUSTOMSTATSSCREEN_H
#define ZUICUSTOMSTATSSCREEN_H

#include "xSnd.h"
#include "zUICustom.h"

class zUIText;

#define NUM_STATS_LINES ((U32)7)
#define NUM_PARRBEST_LINES ((U32)4)

enum en_zStatistics
{
    eStatTime,
    eStatKilled,
    eStatMeleeDamage,
    eStatAirMeleeDamage,
    eStatRangeDamage,
    eStatDodgeDamage,
    eStatIncrediMoves,
    eStatDamageTaken,
    eStatHealth,
    eStatAirTime,
    eStatSteps,
    eStatCalories,
    eStatDamageDealt,
    eStatAttackHits,
    eStatComboLevel,
    eStatIdleTime,
    eStatDeaths,
    eStatJumps,
    eStatDistance,
    eStatObjectsDestroyed,
    eStatCreated,
    eStatRotations,
    eStatMaxDamage,
    eStatThrowsHit,
    eStatThrows,
    eStatIncrediPoints,
    eStatMrIKilled,
    eStatMrIPunchDamage,
    eStatISlamDamage,
    eStatThrowDamage,
    eStatMrIDodgeDamage,
    eStatMrIFinMoves,
    eStatMrIDamageTaken,
    eStatFrozoneKilled,
    eStatFrozonePunchDamage,
    eStatFrozoneNukeDamage,
    eStatFrozoneThrowDamage,
    eStatFrozoneDodgeDamage,
    eStatFrozoneFinMoves,
    eStatFrozoneDamageTaken,
    eStatBounces,
    eStatMaxBounceHeight,
    eStatMaxSpeed,
    eStatTotalSpeed,
    eStatTotalSpeedTime,
    eStatAverageSpeed,
    eStatTimeBoosting,
    eStatTrips,
    eStatWaterTime,
    eStatWaterLostFriction,
    eStatTotalFriction,
    eStatCount
};

class zUICustomStatsScreen : public zUICustom
{
public:
    zUICustomStatsScreen(zUI* ui);

    virtual void Init();
    virtual void Setup();
    virtual void Reset();
    virtual void Exit();
    virtual void PreUpdate(F32 dt);
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

private:
    void FormatTextLines(zUIText** lines, S32 playerIndex, S32 curLine);
    void FormatPrevNumberLines(zUIText** lines, char (*buffers)[10], S32 playerIndex, S32 curLine);
    bool FormatLastNumberLine(zUIText** lines, char (*buffers)[10], S32 playerIndex, S32 curLine, F32 fraction);
    S32 GetStat(en_zStatistics statIndex, S32 playerIndex);
    void AllocateBuffers();
    void DeAllocateBuffers();

    zUIText* stats1[NUM_STATS_LINES];
    zUIText* stats2[NUM_STATS_LINES];
    zUIText* numbers1[NUM_STATS_LINES];
    zUIText* numbers2[NUM_STATS_LINES];
    char numBuff1[NUM_STATS_LINES][10];
    char numBuff2[NUM_STATS_LINES][10];
    xSndHandle statCycleSound;
    F32 totalTime;
    S32 lastLine;
    bool doneUpdating;
};

class zUICustomParrBestScreen : public zUICustom
{
public:
    zUICustomParrBestScreen(zUI* ui);

    virtual void Init();
    virtual void Exit();
    virtual void Setup();
    virtual void Reset();
    virtual void PreUpdate(F32 dt);
    virtual bool HandleEvent(xBase* from, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual bool IsSignalAllowed(U32 toEvent);

    void SetCompletedGoalsFlags();

private:
    void SetupText();
    S32 ComputeScore(S32 playerNum, S32 scoreType); // UNUSED
    void AllocateBuffers();
    void DeAllocateBuffers();

    zUIText* scoreText[NUM_PARRBEST_LINES];
    zUIText* scoreNum[NUM_PARRBEST_LINES];
    zUIText* goalText[NUM_PARRBEST_LINES];
    zUIText* goalNum[NUM_PARRBEST_LINES];
    zUIText* statusText[NUM_PARRBEST_LINES];
    char numBuff1[NUM_PARRBEST_LINES][10];
    char numBuff2[NUM_PARRBEST_LINES][10];
    F32 totalTime;
    bool doneUpdating;
};

void zUISetCreditsType(S32 value); // UNUSED

extern zUICustomParrBestScreen* pUICustomParrBestScreen;

#endif
