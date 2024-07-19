#ifndef XDRAW_H
#define XDRAW_H

#include "xMath3.h"

#ifdef DEBUGRELEASE
void xDrawSetColor(U8 r, U8 g, U8 b, U8 a);
void xDrawSphere(const xSphere* sph, U32 flags);
#else
inline void xDrawSetColor(U8 r, U8 g, U8 b, U8 a) {}
inline void xDrawSphere(const xSphere* sph, U32 flags) {}
#endif

#endif
