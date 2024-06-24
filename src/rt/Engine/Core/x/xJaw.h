#ifndef XJAW_H
#define XJAW_H

#include "types.h"

enum xJawSpeaker
{
    exJawSpeaker_Remy,
    exJawSpeaker_Count
};

enum xJawFlags
{
    exJawFlags_None,
    exJawFlags_Remy
};

struct xJawHeader
{
    U32 numSamples;
    xJawFlags flags;
};

struct xJawData
{
    static xJawData* Find(U32 soundID);

    xJawSpeaker GetIndex(S32 index, F32& value);
    xJawSpeaker Eval(F32 time, F32& jawValue);

private:
    xJawHeader header;
    U8 samples[1];
};

#endif