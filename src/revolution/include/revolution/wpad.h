#ifndef _REVOLUTION_WPAD_H_
#define _REVOLUTION_WPAD_H_

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WPAD_ERR_INVALID -4

void WPADSetDataFormat(s32 chan, u32 fmt);
void WPADControlMotor(s32 chan, u32 command);
s32 WPADProbe(s32 chan, u32* devType);

#ifdef __cplusplus
}
#endif

#endif
