#ifndef XSPLINE_H
#define XSPLINE_H

#include "xDebug.h"
#include "xVec3.h"

struct xCoef
{
    F32 a[4];
};

struct xCoef3
{
    xCoef x;
    xCoef y;
    xCoef z;
};

struct xSpline3
{
    U16 type;
    U16 flags;
    U32 N;
    U32 allocN;
    xVec3* points;
    F32* time;
    xVec3* p12;
    xVec3* bctrl;
    F32* knot;
    xCoef3* coef;
    U32 arcSample;
    F32* arcLength;
};

inline F32 xSpline3_ArcTotal(xSpline3* spl)
{
    xASSERT_L(124, spl->arcLength && spl->arcSample);
    return spl->arcLength[spl->N * spl->arcSample - 1];
}

void xSpline3_EvalSeg(xSpline3* spl, F32 u, U32 deriv, xVec3* o);
void xSpline3_EvalTime(xSpline3* spl, F32 time, U32 deriv, xVec3* o); // UNUSED
F32 xSpline3_EvalArc(xSpline3* spl, F32 s, U32 deriv, xVec3* o); // UNUSED
F32 xSpline3_EvalArcApprox(xSpline3* spl, F32 s, U32 deriv, xVec3* o);
xCoef3* xSpline3_Coef(xSpline3* spl, U32 seg, xCoef3* coef); // UNUSED
void xSpline3_ArcInit(xSpline3* spl, U32 sample);
F32 xSpline3_ArcSeg(xSpline3* spl, F32 u); // UNUSED
F32 xSpline3_ArcTime(xSpline3* spl, F32 time); // UNUSED
void xSpline3_UpdateKnot(xSpline3* spl, U32 knottype); // UNUSED
void xSpline3_AddSegment(xSpline3*, S32, xVec3*); // UNUSED
void xSpline3_RemoveSegment(xSpline3*, S32); // UNUSED
xSpline3* xSpline3_Bspline(xVec3* points, F32* time, U32 numpoints, U32 numalloc, U32 flags, U32 knottype, U32* data); // UNUSED
xSpline3* xSpline3_Bezier(xVec3* points, F32* time, U32 numpoints, U32 numalloc, xVec3* p1, xVec3* p2);
xSpline3* xSpline3_Hermite(xVec3* points, F32* time, U32 numpoints, U32 numalloc, xVec3* tan1, xVec3* tan2); // UNUSED
void xSpline3_Update(xSpline3* spl);
void xSpline3_Catmullize(xSpline3* spl);
xSpline3* xSpline3_Copy(xSpline3* spl); // UNUSED
void xSpline3_Destroy(xSpline3*); // UNUSED
xSpline3* xSpline3_ToCoef(xSpline3* spl, U32 inplace); // UNUSED
xSpline3* xSpline3_ToBezier(xSpline3* spl, U32 inplace); // UNUSED
xSpline3* xSpline3_ToHermite(xSpline3* spl, U32 inplace); // UNUSED
xSpline3* xSpline3_Setup(void* data); // UNUSED
void* xSpline3_RawData(xSpline3* spl, U32 type, U32* size); // UNUSED
void xCoef3_Bound(xCoef3* coef, xVec3* min, xVec3* max); // UNUSED

#if DEBUG
void xSpline3_Test(); // UNUSED
#endif

#endif
