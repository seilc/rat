#include "xCommon.h"
#include "zMusic.h"

#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "zGameState.h"
#include "zScene.h"
#include "iTime.h"

enum eTrackReason
{
    eMusic,
    eNewMusic,
    eTempMusic
};

struct zMusicTrackInfo
{
    xSndHandle sndHandle;
    U32 sndAssetID;
    F32 volume;
    F32 startVolume;
    F32 targetVolume;
    F32 targetVolumeTime;
    F32 crossFadeStartTime;
    eTrackReason trackDesc;
    F32 playTime;
};

struct zMusicState
{
    U32 uMusicDefault;
    bool bPaused;
};

enum
{
    sSimulTracks = 2
};

static zMusicTrackInfo sMusicTrack[sSimulTracks];
static xSndGroup* spMusicGroup;
static zMusicState sMusic;
F32 gDialogueMusicLevel = 0.45f;

static void _zMusicStart(U32 uWhichSlot, U32 uTune)
{
    xASSERT_L(53, uWhichSlot < sSimulTracks);

    S32 uPlayFlags = xSnd_LOOP | 0x100000;
    spMusicGroup->aSndGroupInfo[0].uSoundNameHash = uTune;
    xSndGroupHandle hSoundGroup = xSndMgr_GetHandle(spMusicGroup);

    sMusicTrack[uWhichSlot].sndAssetID = uTune;
    sMusicTrack[uWhichSlot].sndHandle = xSndMgrPlay(hSoundGroup, uPlayFlags, NULL, NULL, NULL, &sMusicTrack[uWhichSlot].volume, NULL);
}

static void _zMusicPause(U32 uWhichSlot, bool bPause)
{
    xASSERT_L(65, uWhichSlot < sSimulTracks);
    if (sMusicTrack[uWhichSlot].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrPause(sMusicTrack[uWhichSlot].sndHandle, bPause);
    }
}

static void _zMusicStop(U32 uWhichSlot)
{
    xASSERT_L(74, uWhichSlot < sSimulTracks);
    if (sMusicTrack[uWhichSlot].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(sMusicTrack[uWhichSlot].sndHandle);
    }

    sMusicTrack[uWhichSlot].sndAssetID = 0;
}

void zMusicInit()
{
    sMusic.bPaused = false;
    sMusic.uMusicDefault = 0;

    for (S32 i = 0; i < sSimulTracks; i++) {
        sMusicTrack[i].sndHandle = XSNDMGR_INVALID_SND_HANDLE;
    }
    spMusicGroup = xSndMgr_AllocGroup(1);
    spMusicGroup->header.uPriority = 0xFF;
    spMusicGroup->header.eSoundCategory = xSndCat_MUSIC;

    if (xglobals->sceneCur->parameters->uDefaultMusicHash) {
        sMusic.uMusicDefault = xglobals->sceneCur->parameters->uDefaultMusicHash;
    } else {
        xSndGroupHandle hMusicDefGroup = xSndMgrGetSoundGroup("music");
        if (hMusicDefGroup != ISND_INVALID_SND_GROUP_HANDLE) {
            U8 uSoundCount = xSndMgrGetSoundCount(hMusicDefGroup);
            U8 randSound = xrand_RandomChoice(uSoundCount);
            sMusic.uMusicDefault = xSndMgrGetSoundId(hMusicDefGroup, randSound);
        }
    }

    xTWEAK("Sound|Conversation Music Dampening", &gDialogueMusicLevel, 0.2f, 0.8f, NULL, NULL, 0);
}

void zMusicUpdate(F32)
{
    if (xglobals->NoMusic) {
        return;
    }

    if ((gGameMode == eGameMode_Game || gGameMode == eGameMode_Title) && sMusicTrack[0].sndHandle == XSNDMGR_INVALID_SND_HANDLE) {
        if (sMusicTrack[1].sndHandle == XSNDMGR_INVALID_SND_HANDLE) {
            if (sMusic.uMusicDefault) {
                sMusicTrack[0].volume = 1.0f;
                sMusicTrack[0].targetVolume = 1.0f;
                sMusicTrack[0].targetVolumeTime = 0.0f;
                sMusicTrack[0].crossFadeStartTime = iTimeGetGame();
                sMusicTrack[0].trackDesc = eMusic;
                _zMusicStart(0, sMusic.uMusicDefault);
                xASSERT_L(135, sMusicTrack[0].sndHandle != XSNDMGR_INVALID_SND_HANDLE);
            }
        }
        else if (sMusicTrack[1].trackDesc == eNewMusic) {
            sMusicTrack[0] = sMusicTrack[1];
            sMusicTrack[0].trackDesc = eMusic;
            sMusicTrack[1].sndHandle = XSNDMGR_INVALID_SND_HANDLE;
            sMusicTrack[1].sndAssetID = 0;
        }
    }
    else if (!xSndMgrIsPlaying(sMusicTrack[0].sndHandle)) {
        sMusicTrack[0].sndHandle = XSNDMGR_INVALID_SND_HANDLE;
    }

    F32 fNowTime = iTimeGetGame();

    for (U32 i = 0; i < sSimulTracks; i++) {
        if (sMusicTrack[i].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            if (xeq(sMusicTrack[i].volume, sMusicTrack[i].targetVolume, EPSILON) == 0) {
                F32 fDeltaTime = fNowTime - sMusicTrack[i].crossFadeStartTime;
                F32 fPercentTime = sMusicTrack[i].targetVolumeTime ? fDeltaTime / sMusicTrack[i].targetVolumeTime : 1.0f;

                if (fPercentTime >= 1.0f) {
                    sMusicTrack[i].volume = sMusicTrack[i].targetVolume;

                    if (xfeq0(sMusicTrack[i].volume)) {
                        if (sMusicTrack[i].trackDesc == eMusic && sMusicTrack[1].trackDesc == eTempMusic) {
                            xASSERT_L(171, i==0);
                            xSndMgrSetVolume(sMusicTrack[i].sndHandle, 0.0f, false);
                            _zMusicPause(i, true);
                        } else
                            _zMusicStop(i);
                    } else {
                        xSndMgrSetVolume(sMusicTrack[i].sndHandle, sMusicTrack[i].volume, false);
                    }
                } else {
                    sMusicTrack[i].volume = fPercentTime * sMusicTrack[i].targetVolume + (1.0f - fPercentTime) * sMusicTrack[i].startVolume;
                    xSndMgrSetVolume(sMusicTrack[i].sndHandle, sMusicTrack[i].volume, false);
                }
            }

            if (sMusicTrack[i].trackDesc == eTempMusic) {
                F32 fHowLongPlaying = fNowTime - sMusicTrack[i].crossFadeStartTime;
                if (sMusicTrack[i].playTime && fHowLongPlaying > sMusicTrack[i].playTime) {
                    sMusicTrack[i].startVolume = sMusicTrack[i].volume;
                    sMusicTrack[i].targetVolume = 0.0f;
                    sMusicTrack[i].crossFadeStartTime = iTimeGetGame();
                    U32 nOtherTrack = !i;
                    sMusicTrack[nOtherTrack].targetVolume = 1.0f;
                    sMusicTrack[nOtherTrack].crossFadeStartTime = iTimeGetGame();
                    sMusicTrack[nOtherTrack].startVolume = 0.0f;
                    _zMusicPause(nOtherTrack, false);
                    sMusicTrack[i].playTime = 0.0f;
                }
            }
        }
    }
}

void zMusicKill()
{
    for (U32 i = 0; i < sSimulTracks; i++) {
        _zMusicStop(i);
    }
}

void zMusicPause()
{
    if (!sMusic.bPaused) {
        for (U32 i = 0; i < sSimulTracks; i++) {
            _zMusicPause(i, true);
        }
        sMusic.bPaused = true;
    }
}

void zMusicUnpause()
{
    if (sMusic.bPaused) {
        for (U32 i = 0; i < sSimulTracks; i++) {
            _zMusicPause(i, false);
        }
        sMusic.bPaused = false;
    }
}

void zMusicSetVolume(F32 vol, F32 delay)
{
    sMusicTrack[0].targetVolume = vol;
    sMusicTrack[0].targetVolumeTime = delay;
    sMusicTrack[0].crossFadeStartTime = iTimeGetGame();
    sMusicTrack[0].startVolume = sMusicTrack[0].volume;
}

static U32 _zMusicCrossFade(F32, F32, F32 fNewVolume, F32 fNewVolTime, U32 newTrack, eTrackReason eReason)
{
    struct ZeroVolumeHack
    {
        static void StopZeroVolume(U32 slot)
        {
            if (sMusicTrack[slot].sndHandle != XSNDMGR_INVALID_SND_HANDLE && sMusicTrack[slot].targetVolume == 0.0f && xfeq0(sMusicTrack[slot].volume)) {
                _zMusicStop(slot);
            }
        }
    };

    for (U32 i = 0; i < sSimulTracks; i++) {
        if (sMusicTrack[i].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            if (newTrack == xSndMgrGetSoundId(sMusicTrack[i].sndHandle)) {
                sMusicTrack[i].crossFadeStartTime = iTimeGetGame();
                sMusicTrack[i].startVolume = sMusicTrack[i].volume;
                sMusicTrack[i].targetVolume = fNewVolume;
                sMusicTrack[i].targetVolumeTime = fNewVolTime;
                ZeroVolumeHack::StopZeroVolume(1 - i);
                return i;
            }
        }
    }

    U32 nFadeInSlot = 0;
    if (eReason == eNewMusic) {
        for (U32 j = 0; j < sSimulTracks; j++) {
            if (sMusicTrack[j].sndHandle == XSNDMGR_INVALID_SND_HANDLE) {
                nFadeInSlot = j;
                break;
            }

            if (sMusicTrack[j].volume < sMusicTrack[nFadeInSlot].targetVolume) {
                nFadeInSlot = j;
            }
        }

        if (sMusicTrack[nFadeInSlot].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            xSndMgrStop(sMusicTrack[nFadeInSlot].sndHandle);
            sMusicTrack[nFadeInSlot].sndAssetID = 0;
        }
    }
    else if (eReason == eTempMusic) {
        nFadeInSlot = 1;

        if (sMusicTrack[nFadeInSlot].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            if (sMusicTrack[nFadeInSlot].trackDesc == eNewMusic) {
                if (sMusicTrack[0].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
                    xSndMgrStop(sMusicTrack[0].sndHandle);
                }
                sMusicTrack[0] = sMusicTrack[nFadeInSlot];
                xSndMgrStop(sMusicTrack[nFadeInSlot].sndHandle);
                sMusicTrack[nFadeInSlot].sndAssetID = 0;
            } else {
                xSndMgrStop(sMusicTrack[nFadeInSlot].sndHandle);
                sMusicTrack[nFadeInSlot].sndAssetID = 0;
            }
        }
    } else {
        xFAIL_L(341);
    }

    sMusicTrack[nFadeInSlot].volume = 0.0f;
    sMusicTrack[nFadeInSlot].startVolume = 0.0f;
    sMusicTrack[nFadeInSlot].crossFadeStartTime = iTimeGetGame();
    sMusicTrack[nFadeInSlot].targetVolume = fNewVolume;
    sMusicTrack[nFadeInSlot].targetVolumeTime = fNewVolTime;
    sMusicTrack[nFadeInSlot].trackDesc = eReason;
    _zMusicStart(nFadeInSlot, newTrack);
    if (sMusicTrack[nFadeInSlot].sndHandle == XSNDMGR_INVALID_SND_HANDLE) {
        iprintf("zMusicNotifyEvent() : %X sound group id not found\n", newTrack);
    }

    ZeroVolumeHack::StopZeroVolume(1 - nFadeInSlot);

    return nFadeInSlot;
}

void zMusicNewSongEvent(const F32* toParam, U32 toParamWidgetID)
{
    if (xglobals->NoMusic) {
        return;
    }

    F32 fFadeVolTime = std::fabs(toParam[0]);
    F32 fNewVolTime = std::fabs(toParam[1]);
    F32 fFadeVolume = std::fabs(toParam[2]);
    F32 fNewVolume = std::fabs(toParam[3]);

    for (U32 i = 0; i < sSimulTracks; i++) {
        if (sMusicTrack[i].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            sMusicTrack[i].startVolume = sMusicTrack[i].volume;
            sMusicTrack[i].targetVolume = fFadeVolume;
            sMusicTrack[i].targetVolumeTime = fFadeVolTime;
            sMusicTrack[i].crossFadeStartTime = iTimeGetGame();
        }
    }

    if (toParamWidgetID) {
        xASSERT_L(388, !sMusic.bPaused &&"Don't send me an event if you've paused the music, eh?");

        if (!sMusic.bPaused)
            _zMusicCrossFade(fFadeVolume, fFadeVolTime, fNewVolume, fNewVolTime, toParamWidgetID, eNewMusic);
    }
    else if (xglobals->sceneCur->parameters->uDefaultMusicHash) {
        if (!sMusic.bPaused)
            _zMusicCrossFade(fFadeVolume, fFadeVolTime, fNewVolume, fNewVolTime, xglobals->sceneCur->parameters->uDefaultMusicHash, eNewMusic);
    }
}

void zMusicTempSongEvent(const F32* toParam, U32 toParamWidgetID)
{
    F32 fFadeVolume = 0.0f;
    F32 fFadeVolTime = std::fabs(toParam[0]);
    F32 fNewVolTime = std::fabs(toParam[1]);
    F32 fPlayTime = std::fabs(toParam[2]);
    F32 fNewVolume = std::fabs(toParam[3]);

    for (U32 i = 0; i < sSimulTracks; i++) {
        if (sMusicTrack[i].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            sMusicTrack[i].startVolume = sMusicTrack[i].volume;
            sMusicTrack[i].targetVolume = fFadeVolume;
            sMusicTrack[i].targetVolumeTime = fFadeVolTime;
            sMusicTrack[i].crossFadeStartTime = iTimeGetGame();
        }
    }

    if (toParamWidgetID) {
        xASSERT_L(424, !sMusic.bPaused &&"Don't send me an event if you've paused the music, eh?");

        if (!sMusic.bPaused) {
            U32 nSlot = _zMusicCrossFade(fFadeVolume, fFadeVolTime, fNewVolume, fNewVolTime, toParamWidgetID, eTempMusic);
            sMusicTrack[nSlot].playTime = fPlayTime;
        }
    }
}

void zMusicTempSongStopEvent()
{
    for (U32 i = 0; i < sSimulTracks; i++) {
        if (sMusicTrack[i].sndHandle != XSNDMGR_INVALID_SND_HANDLE) {
            if (sMusicTrack[i].trackDesc == eTempMusic) {
                sMusicTrack[i].playTime = -1.0f;
            }
        }
    }
}
