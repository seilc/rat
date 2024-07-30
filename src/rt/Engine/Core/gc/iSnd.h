#ifndef ISND_H
#define ISND_H

#include "types.h"

struct FSOUND_STREAM;

struct iSndHandle
{
private:
    U32 voiceRef : 8;
    U32 uniqueId : 24;
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
    bool isStreamed();
    bool isLooping();
};

iSndFileInfo* iSndLookup(U32 id);
bool iSndGetFreeStreamInfo(U32 priority);

#endif
