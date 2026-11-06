#ifndef XONELINER_H
#define XONELINER_H

#include "iSnd.h"

class xOneLiner
{
public:
    char* mAssetName;
    F32 mCycleTime;
    U32 mFlags;
    F32 mSoonestTimeToPlay;
    iSndGroupHandle mSoundGroupHdl;

    static xOneLiner* sDelayedSound;
    static bool sDebugIgnoreTime;
    static bool sDebugIgnoreProb;

    static void SceneInit();
    static void SceneReset();
    static void CalcLevelAdjustments(); // UNUSED
    static void UpdateSounds();
    static void TweaksInit();

    bool NoInhibitions() const;
    bool IsProbable() const;
    F32 CalcTimeToWait() const;
    bool PlaySoundCore();
    bool PlaySound(); // UNUSED
    bool PlaySoundDelayed(F32 delay); // UNUSED
    void Init(); // UNUSED
    void Reset(); // UNUSED

    bool HasValidHandle() const
    {
        return (mSoundGroupHdl != ISND_INVALID_SND_GROUP_HANDLE);
    }
};

class xOneLinerList
{
    U32 mNumSounds;
    xOneLiner* mSounds;

public:
    void InitWithSounds(xOneLiner* soundList, U32 maxCheck); // UNUSED
    void SetTimers(xOneLiner* soundList, U32 maxCheck, F32); // UNUSED
    void Reset(); // UNUSED

    inline bool PlaySound(U32);
    inline iSndGroupHandle GetSoundGroup(U32) const;
    inline void PlaySoundDelayed(U32, F32);
};

#endif
