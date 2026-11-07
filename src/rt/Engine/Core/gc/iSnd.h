#ifndef ISND_H
#define ISND_H

#include "types.h"

#include <fmod.h>

struct xSndVoiceInfo;

#define ISND_MAX_VOICES (48)

enum xSndHandle
{
};

enum iSndGroupHandle
{
};

#define ISND_INVALID_SND_GROUP_HANDLE ((iSndGroupHandle)0xFFFFFFFF)

struct iSndInfo
{
    FSOUND_STREAM* pStream;
    S32 nChannel;
};

struct gcWavInfo
{
    U32 uAssetID;
    U8 uFlags;
    U8 uAudioSampleIndex;
    U8 uFSBIndex;
    U8 uSoundInfoIndex;
};

struct iSndFileInfo : gcWavInfo
{
    bool isStreamed() { return (uFlags >> 1) & 0x1; }
    bool isLooping() { return uFlags & 0x1; }
};

struct iSndHandle
{
    void SetVoiceRef(S32 index)
    {
        if (index <= 0xFF)
            voiceRef = index;
    }
    S32 GetVoiceRef() const { return voiceRef; }
    void SetID(U32 ID) { uniqueId = ID; }
    U32 GetID() { return uniqueId; }

private:
    U32 voiceRef : 8;
    U32 uniqueId : 24;
};

void iSndInit();
void iSndSceneExit();
void iSndExit();
void iSndUpdate();
void iSndSuspendCD(U32);
void iSndSetEnvironmentalEffect(S32 effectType);
bool iSndPlay(xSndVoiceInfo* vp);
xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, S32 flags);
void iSndStop(xSndVoiceInfo* vp, bool bFreeRef);
void iSndPause(xSndVoiceInfo* vp, U32 paused);
void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume);
void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch);
void iSndStartStereo(xSndHandle, xSndHandle);
iSndFileInfo* iSndLookup(U32 id);
bool iSndIsReady(xSndHandle);
bool iSndGetFreeStreamInfo(U32 priority);
void iSndSetExternalCallback(void*);

#endif
