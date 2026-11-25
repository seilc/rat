#ifndef ZVAR_H
#define ZVAR_H

#include "types.h"

// zVarEntry::entry
enum
{
    eVarEntrySndMode,
    eVarEntrySndMusicVol,
    eVarEntrySndFXVol,
    eVarEntryMCAvailable,
    eVarEntryVibrationOnP1,
    eVarEntrySubtitlesOn,
    eVarEntryCurrentSceneLetter,
    eVarEntryCurrentRoom,
    eVarEntryDate,
    eVarEntryHour,
    eVarEntryMinute,
    eVarEntryCounterValue,
    eVarEntryIsEnabled,
    eVarEntryIsVisible,
    eVarEntryTimerSecondsLeftValue,
    eVarEntryTimerMilliSecondsLeftValue,
    eVarEntryIsMNUS,
    eVarEntryDemoType,
    eVarEntryIsReferenceNULL,
    eVarEntryHitCheckpoints,
    eVarEntryTotalCheckpoints,
    eVarEntryPauseType,
    eVarEntryUserSelectsYes,
    eVarEntryUserSelectsNo,
    eVarEntryUserSelectsBack,
    eVarEntryPlayerType,
    eVarEntryIsSignedIn,
    eVarEntryPendingFriendRequest,
    eVarEntryPendingGameInvite,
    eVarEntryStriaghtToMainMenuOnXbox,
    eVarEntryShowEnglishVideos,
    eVarEntryIsSceneUnlocked,
    eVarEntryIsSceneCompleted,
    eVarEntryShowHints,
    eVarEntryVibrationOnP2,
    eVarEntryCount
};

struct zVarEntry
{
    U32 entry;
    U32 varNameID;
    char* varName;
    U32 (*varCB)(void*);
};

extern zVarEntry zVarEntryTable[];
extern S32 bad_card_needed;
extern S32 bad_card_available;

U32 zVarEntryCB_SubtitlesOn(void*);

#endif
