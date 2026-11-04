#ifndef XPLAYBACKBUFFERS_H
#define XPLAYBACKBUFFERS_H

struct PlaybackStruct;

struct PlaybackBuffers
{
    PlaybackStruct* PlaybackBuf;
    PlaybackStruct* PlaybackCur;
    PlaybackStruct* PlaybackEnd;
};

#endif
