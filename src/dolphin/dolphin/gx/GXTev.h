#ifndef __GXTEV_H__
#define __GXTEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <dolphin/types.h>
#include <dolphin/gx/GXEnum.h>

void GXSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1);

#ifdef __cplusplus
}
#endif

#endif
