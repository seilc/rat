#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "iSnd.h"
#include "xMath3.h"
#include "xSndGroup.h"
#include "xSpringy.h"

struct xEnt;

enum xSndHandle
{
};

#define XSNDMGR_INVALID_SND_HANDLE ((xSndHandle)0xFFFFFFFF)

enum xSndEffect
{
    xSndEffect_NONE,
    xSndEffect_CAVE,
    xSndEffect_MAX_TYPES
};

template <S32 unknown>
class sound_queue
{
public:
    sound_queue()
    {
        head = 0;
        tail = 0;
    }

    void play(iSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, F32* pfOverrideVolume, F32* pfOverridePitch)
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

struct xSndVoiceInfo
{
    S32 flags;
    union
    {
        xSndGroup* soundGroup;
        iSndGroupHandle soundGroupHandle;
    };
    U32 assetID;
    union
    {
        xSndHandle sndUniqueID;
        iSndHandle sndID;
    };
    xEnt* pParent;
    const xVec3* pPosition;
    F32 vol;
    F32 assetVol;
    F32 xpitch;
    F32 dopplerFreqCoef;
    S16 category;
    U8 bformerlyAPartOfPriority;
    U8 priority;
    xVec3 actualPos;
    xVec3 playPos;
    const xVec3* pVelocity;
    xVec3 actualVelocity;
    F32 distToListener;
    F32 innerRadius;
    F32 outerRadius;
    iSndInfo ps;
    iSndFileInfo* pSndFileInfo;
};

struct xSndVoiceRefInfo
{
private:
    U32 voiceIndex : 8;
    U32 uniqueId : 24;
};

enum eSndListenerSpringType
{
    eListenerSpringType_None,
    eListenerSpringType_Soft,
    eListenerSpringType_Hard
};

enum xSndMode
{
    xSndMode_Mono,
    xSndMode_Stereo,
    xSndMode_Dolby,
    xSndMode_Count
};

enum xSndListener
{
    xSndListener_CAMERA,
    xSndListener_PLAYER,
    xSndListener_MAX_TYPES
};

struct xSndGlobals
{
    xSndVoiceInfo voice[256];
    xSndVoiceRefInfo voiceRefs[256];
    xMat4x3 listenerMat[2];
    xVec3 listenerVel[2];
    xVec3 right;
    xVec3 up;
    xVec3 at;
    xVec3 pos;
    xVec3 velocity;
    xSpringyVec3 springyPos;
    eSndListenerSpringType sndListenerSpringType;
    U32 m_uSndMgrFlags;
    xSndMode sndMode;
    U32 SndCount;
    xSndListener listenerMode;
};

extern xSndGlobals gSnd;

void xSndMgrUpdate(F32 dt);
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused, bool bInReset);
void xSndMgrStopPausedSounds(S16 eSoundCategory);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
F32 xSndMixerGetOutputVolume(S16 eSoundCategory);
iSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash);
xSndHandle xSndMgrPlay(iSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch);
void xSndMgrStop(xSndHandle& hSound);
void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 startVolume, F32 destVolume);
void xSndMgrFade(xSndHandle& hSound, F32 fadeTime, F32 destVolume);

xSndGroup* xSndMgr_GetGroup(iSndGroupHandle hHandle);

inline bool xSndMgrIsDialogPlaying()
{
    return (gSnd.m_uSndMgrFlags >> 1) & 0x1;
}

inline bool xSndMgrIsLoopedStream(iSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return false;
    }
    return xSndMgr_GetGroup(hSound)->header.uFlags & 0x1;
}

inline bool xSndMgrIsStream(iSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return false;
    }
    if (xSndMgr_GetGroup(hSound)->header.uFlags & 0x3) {
        return true;
    }
    iSndFileInfo* ip = iSndLookup(xSndMgr_GetGroup(hSound)->aSndGroupInfo->uSoundNameHash);
    if (!ip) {
        return false;
    }
    return ip->isStreamed();
}

#endif
