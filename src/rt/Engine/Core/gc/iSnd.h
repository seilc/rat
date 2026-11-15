#ifndef ISND_H
#define ISND_H

#include "xMath.h"

#include <fmod.h>

struct xSndVoiceInfo;

#define ISND_MAX_VOICES (48)
#define ISND_STATIC_BUFFER_SIZE (840 * 1024)

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
void iSndInitSceneLoaded();
void iSndSceneExit();
void iSndExit();
iSndFileInfo* iSndLookup(U32 id);
bool initBytesToConsume(FSOUND_STREAM* stream);
bool releaseBytesToConsume(FSOUND_STREAM* stream);
S32 getBytesToConsume(FSOUND_STREAM* stream);
S32 getFrontBuffer(FSOUND_STREAM* stream);
void* getDataPtr(FSOUND_STREAM* stream);
bool setBytesToConsume(FSOUND_STREAM* stream, S32 bytesToConsume);
bool setFrontBuffer(FSOUND_STREAM* stream, S32 frontBuffer);
bool setDataPtr(FSOUND_STREAM* stream, void* pData);
S8 iSndExternalCB(FSOUND_STREAM* stream, void* buff, S32 len, void* userdata);
bool iSndPlay(xSndVoiceInfo* vp);
void iSndStartStereo(xSndHandle, xSndHandle);
void iSndSetMode();
void iSndSetEnvironmentalEffect(S32 effectType);
bool iSndGetFreeStreamInfo(U32 priority);
xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, S32 flags);
void iSndUpdate();
bool iSndIsReady(xSndHandle);
void iSndStop(xSndVoiceInfo* vp, bool bFreeRef);
void iSndPause(xSndVoiceInfo* vp, U32 paused);
void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume);
void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch);
void iSndKill();
void iSndSetExternalCallback(void*);
void iSndDelayFrame(); // UNUSED
void iSndSuspendCD(U32);
void iSndSuspend();
void iSndResume();
void iSndDebugRender(); // UNUSED

inline U8 iSndVolFromX(F32 fVolume)
{
    U8 vol;
    fVolume = CLAMP(fVolume, -1.0f, 1.0f);
    vol = 255.0f * fVolume;
    return vol;
}

#endif
