#ifndef ZMAIN_H
#define ZMAIN_H

#include "types.h"

extern U32 FB_XRES;
extern U32 FB_YRES;
extern F32 SECS_PER_VBLANK;
extern F32 VBLANKS_PER_SEC;

char* zStealthDebugGetAssertionHeader();
char* zStealthDebugGetAssertionInfo();

#endif
