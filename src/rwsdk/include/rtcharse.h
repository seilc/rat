#ifndef RTCHARSE_H
#define RTCHARSE_H

#include "rwcore.h"

typedef RwRaster RtCharset;

#ifdef __cplusplus
extern "C" {
#endif

RtCharset* RtCharsetCreate(const RwRGBA* foreGround, const RwRGBA* backGround);
RwBool RtCharsetOpen(void);

#ifdef __cplusplus
}
#endif

#endif
