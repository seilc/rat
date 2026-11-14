#ifndef _REVOLUTION_KPAD_H_
#define _REVOLUTION_KPAD_H_

#include <dolphin/types.h>
#include <dolphin/mtx.h>
#include <revolution/wpad.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vec2 {
    f32 x, y;
} Vec2;

typedef union KPADEXStatus {
    struct {
        Vec2 stick;
        Vec acc;
        f32 acc_value;
        f32 acc_speed;
    } fs;
    struct {
        Vec2 stick;
        Vec2 substick;
        f32 ltrigger;
        f32 rtrigger;
    } gc;
} KPADEXStatus;

typedef struct KPADStatus {
    u32 hold;
    u32 trig;
    u32 release;
    Vec acc;
    f32 acc_value;
    f32 acc_speed;
    Vec2 pos;
    Vec2 vec;
    f32 speed;
    Vec2 horizon;
    Vec2 hori_vec;
    f32 hori_speed;
    f32 dist;
    f32 dist_vec;
    f32 dist_speed;
    u32 dev_type;
    KPADEXStatus ex_status;
    s8 dpd_valid_fg;
    s8 wpad_err;
} KPADStatus;

void KPADSetAccParam(s32 chan, f32 play_radius, f32 sensitivity);
s32 KPADRead(s32 chan, KPADStatus* sampling_bufs, s32 length);
void KPADInit(void);
void KPADReset(void);

#ifdef __cplusplus
}
#endif

#endif
