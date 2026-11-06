#ifndef XFXHIGHDYNAMICRANGE_H
#define XFXHIGHDYNAMICRANGE_H

#include "iDraw.h"

#include <rwcore.h>
#include <rpworld.h>

struct xFXHighDynamicRangeConfiguration
{
    S32 glow;
    S32 darken;
    S32 downsamples;
    F32 overbrighten;
    F32 overbrighten_decay;
    bool blur_faster;
};

extern bool HDR_brightening;

void xFXHighDynamicRangeSetColorize(F32 r, F32 g, F32 b, F32 alpha);
void xFXHighDynamicRangeInit();
void xFXHighDynamicRangeSceneEnter();
void xFXHighDynamicRangeSceneExit();
void xFXHighDynamicRangeReset();
void xFXHighDynamicRangeStartRender();
void xFXHighDynamicRangeRender(RwCamera*);
void xFXHighDynamicRangeRestoreSettings();
U32 xFXHighDynamicRangeBrightenPtank(RpAtomic* ptank, bool bright, S32);
void xFXHighDynamicRangeRestorePtank(RpAtomic* ptank, U32 oldtest);
void xFXHighDynamicRangeTunePalette(RwRaster*, F32, bool);
void xFXHighDynamicRangeUpdate(F32 dt);
void xFXHighDynamicRangeEnable(bool enable); // UNUSED
S32 xFXHighDynamicRangeGetBackgroundGlow();
S32 xFXHighDynamicRangeGetBackgroundDarken(); // UNUSED
void xFXHighDynamicRangeSetConfiguration(const xFXHighDynamicRangeConfiguration& configuration);
void xFXHighDynamicRangeGetConfiguration(xFXHighDynamicRangeConfiguration& configuration);
void xFXHighDynamicRangeSetGlowClamp(S32, bool);
void xFXHighDynamicRangeResetGlowClamp();
void xFXHighDynamicRangeSetBackground(S32 glow, S32 darken, F32 time);
void xFXHighDynamicRangeSetBackgroundDefault(S32 glow, S32 darken, bool apply);

inline void xFXHighDynamicRangeBrighten(bool bright, bool force_change)
{
    if (HDR_brightening != bright || force_change) {
        HDR_brightening = bright;
        iDrawSetFBMSK(bright ? 0 : 0xFF000000, 0xFFFFFFFF, true);
    }
}

#endif
