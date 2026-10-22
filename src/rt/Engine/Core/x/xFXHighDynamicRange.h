#ifndef XFXHIGHDYNAMICRANGE_H
#define XFXHIGHDYNAMICRANGE_H

#include "types.h"
#include "iDraw.h"

#include <rwcore.h>
#include <rpworld.h>

extern bool HDR_brightening;

S32 xFXHighDynamicRangeGetBackgroundGlow();
U32 xFXHighDynamicRangeBrightenPtank(RpAtomic* ptank, bool bright, S32);
void xFXHighDynamicRangeRestorePtank(RpAtomic* ptank, U32 oldtest);

inline void xFXHighDynamicRangeBrighten(bool bright, bool force_change)
{
    if (HDR_brightening != bright || force_change) {
        HDR_brightening = bright;
        iDrawSetFBMSK(bright ? 0 : 0xFF000000, 0xFFFFFFFF, true);
    }
}

#endif
