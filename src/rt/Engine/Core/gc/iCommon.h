#ifndef ICOMMON_H
#define ICOMMON_H

#include "types.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

static inline void SCENEIDSTR(U32 id, char* str)
{
    str[0] = (id >> 24) & 0xFF;
    str[1] = (id >> 16) & 0xFF;
    str[2] = (id >> 8) & 0xFF;
    str[3] = id & 0xFF;
    str[4] = '\0';
}

#endif
