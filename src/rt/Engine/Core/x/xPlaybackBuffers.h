#ifndef XPLAYBACKBUFFERS_H
#define XPLAYBACKBUFFERS_H

#include "types.h"

struct PlaybackStruct;

struct PlaybackBuffers
{
    PlaybackStruct* PlaybackBuf;
    PlaybackStruct* PlaybackCur;
    PlaybackStruct* PlaybackEnd;
};

#endif
