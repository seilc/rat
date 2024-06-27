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
    static void UpdateSounds();
    static void TweaksInit();

    bool NoInhibitions() const;
    bool IsProbable() const;
    F32 CalcTimeToWait() const;
    bool PlaySoundCore();

    bool HasValidHandle() const
    {
        return (mSoundGroupHdl != ISND_INVALID_SND_GROUP_HANDLE);
    }
};

#endif
