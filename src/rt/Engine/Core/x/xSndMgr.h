#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "iSnd.h"
#include "xMath3.h"

struct xEnt;

enum xSndHandle
{
};

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

void xSndMgrUpdate(F32 dt);
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused, bool bInReset);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
F32 xSndMixerGetOutputVolume(S16 eSoundCategory);
iSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash);
xSndHandle xSndMgrPlay(iSndGroupHandle hSoundToPlay, S32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch);
void xSndMgrStop(xSndHandle& hSound);

bool xSndMgrIsDialogPlaying();

#endif
