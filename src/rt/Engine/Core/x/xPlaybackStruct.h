#ifndef XPLAYBACKSTRUCT_H
#define XPLAYBACKSTRUCT_H

#include "xPad.h"

struct PlaybackStruct
{
    U32 on;
    xPadAnalog analog1;
    xPadAnalog analog2;
    U32 randseed1;
    U32 randseed2;
};

#endif
