#include "xCommon.h"
#include "zCompSound.h"

#include "xSndMgr.h"

static zCompSoundParams gDefaultSoundParams;

zCompSound::zCompSound()
{
}

void zCompSound::Attached()
{
    if (parameters == NULL) {
        parameters = &gDefaultSoundParams;
    }

    loopingSoundGroupPlaying = ISND_INVALID_SND_GROUP_HANDLE;
    loopingSoundPlaying = XSNDMGR_INVALID_SND_HANDLE;
}

void zCompSound::AllAttached()
{
}

void zCompSound::Detached()
{
    if (loopingSoundGroupPlaying != ISND_INVALID_SND_GROUP_HANDLE) {
        if (xSndMgrIsPlaying(loopingSoundPlaying)) {
            xSndMgrStop(loopingSoundPlaying);
        }
    }
}

void zCompSound::PreUpdate(xScene*, F32)
{
    if (loopingSoundGroupPlaying != Params()->loopingSoundGroup) {
        if (loopingSoundGroupPlaying != ISND_INVALID_SND_GROUP_HANDLE) {
            if (xSndMgrIsPlaying(loopingSoundPlaying)) {
                xSndMgrStop(loopingSoundPlaying);
            }
        }

        loopingSoundGroupPlaying = ISND_INVALID_SND_GROUP_HANDLE;
        loopingSoundPlaying = XSNDMGR_INVALID_SND_HANDLE;

        if (Params()->loopingSoundGroup != ISND_INVALID_SND_GROUP_HANDLE) {
            loopingSoundPlaying = xSndMgrPlay(Params()->loopingSoundGroup, 0, Params()->loopingSoundPosition, NULL, NULL, NULL, NULL);
            xASSERT_L(86, loopingSoundPlaying != XSNDMGR_INVALID_SND_HANDLE);
            loopingSoundGroupPlaying = Params()->loopingSoundGroup;
        }
    }
}

#if DEBUG
void zCompSound::AddStaticTweaks()
{
}
#endif
