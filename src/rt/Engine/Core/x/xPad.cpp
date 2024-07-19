#include "xPad.h"

#include "xTRC.h"
#include "zScene.h"
#include "zMenu.h"
#include "zGame.h"
#include "zGlobals.h"

#include <string.h>

xPad mPad[k_XPAD_MAX];

S32 xPadInit()
{
    memset(mPad, 0, sizeof(mPad));

    for (S32 i = 0; i < k_XPAD_MAX; i++) {
        mPad[i].port = i;
        mPad[i].flags |= 0x3;
    }

    xTRCInit();

    if (!iPadInit()) {
        return 0;
    }

    return 1;
}

#ifndef NON_MATCHING
S32 xPadInit(S32 i)
{
    xASSERT(0, i > -1 && i < k_XPAD_MAX);
    return 1;
}
#endif

xPad* xPadEnable(S32 idx)
{
    xPad* p = &mPad[idx];
    return iPadEnable(p, idx);
}

void xPadRumbleEnable(S32 idx, S32 enable)
{
    xPad* p = &mPad[idx];

    if (p->state != 1) {
        enable = 0;
    }

    if (enable) {
        if (p->flags & 0x4) {
            p->flags |= 0x8;
        }
    } else {
        if (p->flags & 0x8) {
            p->flags ^= 0x8;
        }
    }
}

void xPadClear()
{
    for (S32 i = 0; i < k_XPAD_MAX; i++) {
        xPadClear(i);
    }
}

void xPadClear(S32 idx)
{
    xPad* p = &mPad[idx];
    p->pressed = 0;
    p->released = 0;
}

S32 xPadUpdate(S32 idx, F32 time_passed)
{
    U32 new_on = 0;

    xASSERT(206, idx >= 0 && idx < k_XPAD_MAX);

    if (zScene_ScreenAdjustMode()) {
        xPadAnalogIsDigital(idx, 0);
    } else if (zMenuRunning() || zGameIsPaused() || globals.mgManager->IsMiniGamePaused()) {
        xPadAnalogIsDigital(idx, 1);
    } else {
        xPadAnalogIsDigital(idx, 0);
    }

    xPad* p = &mPad[idx];

    if (!iPadUpdate(p, &new_on)) {
        p->pressed = 0;
        p->released = 0;
        return 1;
    }

    if ((p->flags & 0x10) && (p->flags & 0x1)) {
        U32 fake_dpad = 0;
        if (p->analog1.x >= 50) {
            fake_dpad |= 0x20;
        } else if (p->analog1.x <= -50) {
            fake_dpad |= 0x80;
        }
        if (p->analog1.y >= 50) {
            fake_dpad |= 0x40;
        } else if (p->analog1.y <= -50) {
            fake_dpad |= 0x10;
        }
        if (fake_dpad == 0) {
            p->al2d_timer = 0.0f;
        } else {
            p->al2d_timer -= time_passed;
            if (p->al2d_timer <= 0.0f) {
                new_on |= fake_dpad;
                p->al2d_timer = 0.35f;
            }
        }
    }

#ifdef DEBUGRELEASE
    static S32 submap = 0;
    if (submap) {
        SubMapAll((S32*)&new_on, p);
    }
#endif

    p->pressed = new_on & ~p->on;
    p->released = p->on & ~new_on;
    p->on = new_on;
    
    for (S32 i = 0; i < 22; i++) {
        if (p->pressed & (1<<i)) {
            p->down_tmr[i] = 0.0f;
        } else if (p->released & (1<<i)) {
            p->up_tmr[i] = 0.0f;
        }
        if (p->on & (1<<i)) {
            p->down_tmr[i] += time_passed;
        } else {
            p->up_tmr[i] += time_passed;
        }
    }

    if (p->flags & 0x10) {
        if (!(p->on & 0x10) && !(p->on & 0x40) && !(p->on & 0x80) && !(p->on & 0x20)) {
            p->d_timer = 0.0f;
        } else {
            p->d_timer -= time_passed;
            if (p->d_timer <= 0.0f) {
                p->d_timer = 0.35f;
                if (p->on & 0x10) {
                    p->pressed |= 0x10;
                } else if (p->on & 0x40) {
                    p->pressed |= 0x40;
                }
                if (p->on & 0x80) {
                    p->pressed |= 0x80;
                } else if (p->on & 0x20) {
                    p->pressed |= 0x20;
                }
            }
        }
    }

    return 1;
}

void xPadUpdateAllPads(F32 time_passed)
{
    iPadUpdateDataAll();

    for (S32 j = 0; j < k_XPAD_MAX; j++) {
        xPadUpdate(j, time_passed);
    }
}

void SubMapAll(S32* neuon, xPad* xp)
{
    S32 old = *neuon;
    S32 keys = 0;
    
    *neuon &= 0xFFF0880F;

    if (old & 0x10) keys |= 0x40;
    if (old & 0x40) keys |= 0x10;
    if (old & 0x80) keys |= 0x20;
    if (old & 0x20) keys |= 0x80;
    if (old & 0x10000) keys |= 0x80000;
    if (old & 0x20000) keys |= 0x10000;
    if (old & 0x40000) keys |= 0x20000;
    if (old & 0x80000) keys |= 0x40000;
    if (old & 0x100) keys |= 0x1000;
    if (old & 0x200) keys |= 0x2000;
    if (old & 0x400) keys |= 0x4000;
    if (old & 0x800) keys |= 0x8000;
    if (old & 0x1000) keys |= 0x100;
    if (old & 0x2000) keys |= 0x200;
    if (old & 0x4000) keys |= 0x400;
    if (old & 0x8000) keys |= 0x800;

    xp->analog1.x = -xp->analog1.x;
    xp->analog1.y = -xp->analog1.y;
    xp->analog2.x = -xp->analog2.x;
    xp->analog2.y = -xp->analog2.y;

    *neuon |= keys;
}

void xPadNormalizeAnalog(xPad& pad, S32 inner_zone, S32 outer_zone)
{
    const xPadAnalog* src = &pad.analog1;
    for (S32 i = 0; i < 2; i++) {
        pad.analog[i].offset.x = normalize_analog(src[i].x, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        pad.analog[i].offset.y = normalize_analog(src[i].y, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        if (pad.analog[i].offset.x == 0.0f && pad.analog[i].offset.y == 0.0f) {
            pad.analog[i].mag = 0.0f;
            pad.analog[i].dir.assign(1.0f, 0.0f);
            pad.analog[i].ang = 0.0f;
        } else {
            if (pad.analog[i].offset.x > 1.0f) {
                pad.analog[i].offset.x = 1.0f;
            } else if (pad.analog[i].offset.x < -1.0f) {
                pad.analog[i].offset.x = -1.0f;
            }
            if (pad.analog[i].offset.y > 1.0f) {
                pad.analog[i].offset.y = 1.0f;
            } else if (pad.analog[i].offset.y < -1.0f) {
                pad.analog[i].offset.y = -1.0f;
            }
            pad.analog[i].mag = pad.analog[i].offset.length();
            pad.analog[i].dir = pad.analog[i].offset;
            pad.analog[i].dir /= pad.analog[i].mag;
            pad.analog[i].ang = xatan2(pad.analog[i].dir.y, pad.analog[i].dir.x);
        }
    }
}

void xPadKill()
{
    iPadKill();
}

void xPadAnalogIsDigital(S32 idx, S32 enable)
{
    xPad* pad = &mPad[idx];

    if (enable) {
        pad->flags |= 0x10;
    } else {
        pad->flags &= ~0x10;
    }

    if (pad->al2d_timer >= 0.35f) {
        pad->al2d_timer = 0.35f;
    }

    if (pad->ar2d_timer >= 0.35f) {
        pad->ar2d_timer = 0.35f;
    }

    if (pad->d_timer >= 0.35f) {
        pad->d_timer = 0.35f;
    }
}
