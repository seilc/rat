#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "iSnd.h"
#include "xMath3.h"
#include "xSnd.h"
#include "xSndGroup.h"

struct xEnt;

#define XSNDMGR_INVALID_SND_HANDLE ((xSndHandle)0xFFFFFFFF)

typedef iSndGroupHandle xSndGroupHandle;

enum xSndMixerFaderName
{
    xSndMixerFader_NONE = -1,
    xSndMixerFader_CATEGORY_FADERS_START,
    xSndMixerFader_GAME_3D = 0,
    xSndMixerFader_GAME_2D,
    xSndMixerFader_DIALOG,
    xSndMixerFader_DIALOG_3D,
    xSndMixerFader_MUSIC,
    xSndMixerFader_CUTSCENE,
    xSndMixerFader_UI,
    xSndMixerFader_GROUP_FADERS_START,
    xSndMixerFader_MUSIC_GROUP = xSndMixerFader_GROUP_FADERS_START,
    xSndMixerFader_SFX_GROUP,
    xSndMixerFader_FADERS_END
};

enum
{
    xSndCat_ALL = -3,
    xSndCat_ALL_BUT_MUSIC = -2,
    xSndCat_GAME_3D = 0,
    xSndCat_GAME_2D,
    xSndCat_DIALOG,
    xSndCat_DIALOG_3D,
    xSndCat_MUSIC,
    xSndCat_CUTSCENE,
    xSndCat_UI,
    xSndCat_FIRST_CATEGORY = xSndCat_GAME_3D,
    xSndCat_LAST_CATEGORY = xSndCat_UI
};

enum xSndEffect
{
    xSndEffect_NONE,
    xSndEffect_CAVE,
    xSndEffect_MAX_TYPES
};

extern xSndEffect m_eCurrentEffect;
extern U32 gCurrSoundMemoryUsed;
extern U32 gHighwaterSoundMemoryUsed;
extern U32 gNumStreamsOpened;
extern U32 gNumStreamsClosed;

struct xIniFile;

void xSndMgr_ParseINI(xIniFile* ini);
S32 xSndMgrGetVoiceIndex(xSndHandle& xHandle);
xSndGroup* xSndMgr_AllocGroup(U8 nSoundAssets);
void xSndMgrInit();
void xSndMgrSetListenerSpringType(eSndListenerSpringType type);
void xSndMgrSelectListenerMode(xSndListener Mode);
void xSndMgrSetEffect(xSndEffect Effect);
xSndEffect xSndMgrGetEffect();
void xSndMgrSetListenerMatrix(xSndListener ListenerType, const xMat4x3* pMat); // UNUSED
void xSndMgrProcessSoundPosition(const xVec3* pActual, xVec3* pProcessed);
void xSndMgr_UpdateListenerPosition(F32 dt);
void xSndMgr_InitListenerPosition();
bool xSndMgr_UpdateVoicePosition(xSndVoiceInfo* pVoice);
xSndVoiceInfo* xSndMgr_GetFreeVirtualVoiceInfo(U32 priority, S32 flags, bool virtualOnly);
void xSndMgr_Stop(xSndVoiceInfo* pVoice, bool bFreeRef);
void xSndMgr_Pause(xSndVoiceInfo* pVoice, bool bPaused);
void xSndMgr_CutscenePause(xSndVoiceInfo* pVoice, bool bPaused);
bool xSndMgr_SetInaudible(xSndVoiceInfo* pVoice);
bool xSndMgr_SetAudible(xSndVoiceInfo* pVoiceStorage);
void xSndMgr_DelayFrame(); // UNUSED
void xSndMgrSceneInit();
void xSndMgrUpdatePreScene();
void xSndMgrUpdate(F32 dt);
void xSndMgrSceneExit();
void xSndMgrExit();
bool xSndMgrDoesEffects(S16 eSoundCategory);
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused, bool bInReset);
void xSndMgrStopPausedSounds(S16 eSoundCategory);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
void xSndMgrCutscenePause(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
void xSndMixerSetVolume(xSndMixerFaderName eMixerFader, F32 fVolume);
F32 xSndMixerGetVolume(xSndMixerFaderName eMixerFader);
void xSndMixerSetDampVolume(xSndMixerFaderName eMixerFader, F32 fVolume);
void xSndMixerSetDampVolumeOthers(xSndMixerFaderName eMixerFader, F32 fVolume); // UNUSED
void xSndMixerResetDampVolumes();
F32 xSndMixerGetOutputVolume(S16 eSoundCategory);
void xSndMgrStopSounds(xEnt* pParent); // UNUSED
void xSndMgrPauseSounds(xEnt* pParent, bool bPaused); // UNUSED
void xSndMgrSetVolume(xEnt* pParent, F32 fVolume); // UNUSED
void xSndMgrStopSounds(xSndGroupHandle hActiveSound); // UNUSED
void xSndMgrPauseSounds(xSndGroupHandle hActiveSound, bool bPaused); // UNUSED
void xSndMgrSetVolume(xSndGroupHandle hActiveSound, F32 fVolume); // UNUSED
U32 xSndMgrGetGroupId(xSndGroupHandle hSound); // UNUSED
U8 xSndMgrGetSoundCount(xSndGroupHandle hSound);
U32 xSndMgrGetSoundId(xSndGroupHandle hSound, U32 uIndexOfSound);
xSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash);
bool xSndMgrIsPlaying(xSndGroupHandle hSoundGroup);
U8 xSndMgrGetPriority(xSndGroupHandle hSound, bool bStreaming, bool bLooping);
xSndVoiceInfo* xSndGetFreeVoiceInfo(const xSndGroupHeader* pSnd, S32 iFlags);
xSndHandle xSndMgrPlay(xSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch);
U32 xSndMgrGetSoundId(xSndHandle& hSound);
bool xSndMgrIsPlaying(xSndHandle& hSound);
bool xSndMgrIsLooping(xSndHandle& hSound);
bool xSndMgrIsStreamed(xSndHandle& hSound);
U32 xSndMgrIsReady(xSndHandle& hSound);
void xSndMgrPause(xSndHandle& hSound, bool bPause);
void xSndMgrStop(xSndHandle& hSound);
void xSndMgrSetVolume(xSndHandle& hSound, F32 fVolume, bool assetRelative);
void xSndMgrSetPitch(xSndHandle& hSound, F32 fPitch); // UNUSED
void xSndMgrStartStereo(xSndHandle hSnd00, xSndHandle hSnd01);
void xSndMgrUpdatePosition(xSndHandle& hSound, const xVec3* pVector); // UNUSED
void xSndMgrChangeAttachedPosition(xSndHandle& hSound, const xVec3* pVector);
void xSndMgrChangeParent(xSndHandle& hSound, xEnt* pParentEnt); // UNUSED
void xSndMgrStopChildren(xEnt* pParentEnt);
void xSndMgrPauseChildren(xEnt* pParentEnt, bool bPaused); // UNUSED
xSndHandle xSndMgrFadeOn(xSndGroupHandle hSoundToPlay, F32 fadeTime, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch); // UNUSED
void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 startVolume, F32 destVolume);
void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 destVolume);
void xSndMgrFadeOff(xSndHandle& hSound, F32 fadeTime); // UNUSED
void xSndMgrDebugRender();
void xSndMgrDebugRenderWorld();
void xSndMgr_SetPaused(S16 whichCat, bool bPause);
bool xSndMgrIsPaused(S16 eSoundCategory);
S32 xSndMgrAllocRef(U32 uniqueID, S32 voiceIndex);
void xSndMgrFreeRef(xSndHandle& xHandle);
void xSndMgrUpdateRef(xSndHandle& xHandle, S32 voiceIndex);
void debugSndRender(); // UNUSED

inline xSndGroup* xSndMgr_GetGroup(xSndGroupHandle hHandle) { return (xSndGroup*)hHandle; }
inline xSndGroupHandle xSndMgr_GetHandle(xSndGroup* pSnd) { return (xSndGroupHandle)pSnd; }

inline bool xSndMgrIsDialogPlaying()
{
    return (gSnd.m_uSndMgrFlags >> 1) & 0x1;
}

inline bool xSndMgrIsLoopedStream(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) return false;
    return xSndMgr_GetGroup(hSound)->header.uFlags & 0x1;
}

inline bool xSndMgrIsStream(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) return false;
    if (xSndMgr_GetGroup(hSound)->header.uFlags & 0x3) {
        return true;
    }
    iSndFileInfo* ip = iSndLookup(xSndMgr_GetGroup(hSound)->aSndGroupInfo->uSoundNameHash);
    if (!ip) {
        return false;
    }
    return ip->isStreamed();
}

template <S32 unknown>
class sound_queue
{
public:
    sound_queue() { head = 0; tail = 0; }

    void play(xSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, F32* pfOverrideVolume, F32* pfOverridePitch)
    {
        xSndHandle handle = xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, pfOverrideVolume, pfOverridePitch);
        push(handle);
    }

    void push(xSndHandle handle)
    {
        _playing[tail] = handle;

        S32 begin = head;
        S32 end = tail+1;
        if (end <= begin) end += unknown+1;

        S32 len = end - begin;
        if (len > unknown) {
            xSndMgrStop(_playing[begin]);
            begin++;
            head = begin % (unknown+1);
        }

        tail = end % (unknown+1);
    }

private:
    xSndHandle _playing[unknown+1];
    S32 head;
    S32 tail;
};

#endif
