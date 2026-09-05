#ifndef XDRAW_H
#define XDRAW_H

#include "xMath3.h"

#if DEBUG || RELEASE
void xDrawSetColor(U8 r, U8 g, U8 b, U8 a);
void xDrawLine(const xVec3* a, const xVec3* b);
void xDrawSphere(const xSphere* sph, U32 flags);
void xDrawSphere(const xVec3* center, F32 r, U32 flags);
#else
inline void xDrawSetColor(U8 r, U8 g, U8 b, U8 a) {}
inline void xDrawLine(const xVec3* a, const xVec3* b) {}
inline void xDrawSphere(const xSphere* sph, U32 flags) {}
inline void xDrawSphere(const xVec3* center, F32 r, U32 flags) {}
#endif

S32 Im2DRenderQuadAlpha(F32 x1, F32 y1, F32 x2, F32 y2, F32 alpha, F32 z, F32, F32 uvOffset);

#endif
