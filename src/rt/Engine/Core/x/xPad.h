#ifndef XPAD_H
#define XPAD_H

#include "iPad.h"
#include "xMath2.h"
#include "xDebug.h"

#define k_XPAD_MAX 4

typedef struct _tagPadAnalog
{
    S8 x;
    S8 y;
} xPadAnalog;

typedef enum _tagPadState
{
    ePad_Disabled,
    ePad_Enabled,
    ePad_Total
} xPadState;

typedef struct _tagxPad
{
    struct analog_data
    {
        xVec2 offset;
        xVec2 dir;
        F32 mag;
        F32 ang;
    };

    U8 value[22];
    U8 last_value[22];
    U32 on;
    U32 pressed;
    U32 released;
    xPadAnalog analog1;
    xPadAnalog analog2;
    xPadState state;
    U32 flags;
    S16 port;
    S16 slot;
    iPad context;
    F32 al2d_timer;
    F32 ar2d_timer;
    F32 d_timer;
    F32 up_tmr[22];
    F32 down_tmr[22];
    analog_data analog[2];
} xPad;

extern xPad mPad[k_XPAD_MAX];

S32 xPadInit();
xPad* xPadEnable(S32 idx);
void xPadRumbleEnable(S32 idx, S32 enable);
void xPadClear();
void xPadClear(S32 idx);
S32 xPadUpdate(S32 idx, F32 time_passed);
void xPadUpdateAllPads(F32 time_passed);
void SubMapAll(S32* neuon, xPad* xp);
void xPadNormalizeAnalog(xPad& pad, S32 inner_zone, S32 outer_zone);
void xPadKill();
void xPadAnalogIsDigital(S32 idx, S32 enable);

inline F32 normalize_analog(S32 v, S32 v_min, S32 v_max, S32 dead_center, S32 dead_min, S32 dead_max)
{
    xASSERT(248, dead_min > v_min);
    xASSERT(249, dead_max < v_max);
    if (v <= dead_center) {
        if (v >= dead_min) {
            return 0.0f;
        } else if (v <= v_min) {
            return -1.0f;
        } else {
            return (F32)(v - dead_min) / (dead_min - v_min);
        }
    } else {
        if (v <= dead_max) {
            return 0.0f;
        } else if (v >= v_max) {
            return 1.0f;
        } else {
            return (F32)(v - dead_max) / (v_max - dead_max);
        }
    }
}

#endif
