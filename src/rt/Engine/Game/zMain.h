#ifndef ZMAIN_H
#define ZMAIN_H

#include "types.h"

typedef enum _tagGameVidMode
{
    eGameVidModeNTSC,
    eGameVidModePAL
} eGameVidMode;

extern U32 FB_XRES;
extern U32 FB_YRES;
extern F32 SECS_PER_VBLANK;
extern F32 VBLANKS_PER_SEC;
extern eGameVidMode gVidMode;
extern S32 GCN_DEMO_PARAM;

char* zStealthDebugGetAssertionHeader();
char* zStealthDebugGetAssertionInfo();
void zMainShowProgressBar(S32 percentage);

inline bool zMainIsPAL() { return gVidMode == eGameVidModePAL; }

inline bool zMainIsNTSC() { return gVidMode == eGameVidModeNTSC; }

#endif
