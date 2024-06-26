#ifndef XMATH3_H
#define XMATH3_H

#include "iMath3.h"
#include "xVec3.h"

struct xVec4
{
    F32 x, y, z, w;
};

struct xBox
{
    xVec3 upper;
    xVec3 lower;
};

struct xBBox
{
    xVec3 center;
    xBox box;
};

struct xSphere
{
    xVec3 center;
    F32 r;
};

struct xCylinder
{
    xVec3 center;
    F32 r;
    F32 h;
};

#define XRAY3_USE_MAX (1<<11)

struct xRay3
{
    xVec3 origin;
    xVec3 dir;
    F32 min_t;
    F32 max_t;
    S32 flags;
};

struct xIsect
{
    U32 flags;
    F32 penned;
    F32 contained;
    F32 lapped;
    xVec3 point;
    xVec3 norm;
    F32 dist;
};

struct xMat3x3
{
    xVec3 right;
    S32 flags;
    xVec3 up;
    U32 pad1;
    xVec3 at;
    U32 pad2;
} ALIGN(16);

struct xMat4x3 : xMat3x3
{
    xVec3 pos;
    U32 pad3;
} ALIGN(16);

struct xQuat
{
    xVec3 v;
    F32 s;
};

struct xRot
{
    xVec3 axis;
    F32 angle;
};

extern xMat4x3 g_I3;

inline void xMat3x3Copy(xMat3x3* o, const xMat3x3* m)
{
    *o = *m;
}

inline void xMat3x3Identity(xMat3x3* m)
{
    xMat3x3Copy(m, &g_I3);
}

#endif
