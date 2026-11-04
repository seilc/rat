#ifndef XMATH3_H
#define XMATH3_H

#include "iMath3.h"
#include "xVec3.h"
#include "xDebug.h"

struct xVec4
{
    F32 x, y, z, w;
};

struct xFRect
{
    F32 x, y, w, h;
};

struct xLine3
{
    xVec3 p1;
    xVec3 p2;
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

#define XRAY3_USE_MIN (1<<10)
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

class xPlane
{
public:
    xVec3 norm;
    F32 d;

    void Init(const xVec3& pt)
    {
        d = norm.dot(pt);
    }

    void Init(const xVec3& normal, const xVec3& pt);

    F32 Dist(const xVec3& pt) const;
    void Project(xVec3& proj, const xVec3& pt, F32 dist_above) const;
};

extern xVec3 g_O3;
extern xVec3 g_X3;
extern xVec3 g_Y3;
extern xMat4x3 g_I3;

void xBoxInitBoundOBB(xBox* o, const xBox* b, const xMat4x3* m);
void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* a);
void xMat3x3RotY(xMat3x3* m, F32 t);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void xMat3x3ScaleC(xMat3x3* m, F32 x, F32 y, F32 z);
void xMat3x3Transpose(xMat3x3* o, const xMat3x3* m);
void xMat3x3Tolocal(xVec3* o, const xMat3x3* m, const xVec3* v);
F32 xMat3x3LookVec(xMat3x3* m, const xVec3* at);
void xMat3x3Euler(xMat3x3* m, const xVec3* ypr);
void xMat3x3Euler(xMat3x3* m, F32 yaw, F32 pitch, F32 roll);
void xMat3x3LMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatToMat(const xQuat* q, xMat3x3* m);
void xQuatSlerp(xQuat* o, const xQuat* a, const xQuat* b, F32 t);

inline void xMat3x3Copy(xMat3x3* o, const xMat3x3* m)
{
    *o = *m;
}

inline void xMat3x3Identity(xMat3x3* m)
{
    xMat3x3Copy(m, &g_I3);
}

void xMat3x3Rot(xMat3x3* m, const xVec3* a, F32 t);

static inline void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    F32 y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    F32 z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;
    o->x = x, o->y = y, o->z = z;
}

inline F32 xMat3x3LookAt(xMat3x3* m, const xVec3* from, const xVec3* to)
{
    xVec3 at;
    xVec3Sub(&at, to, from);
    return xMat3x3LookVec(m, &at);
}

inline void xMat3x3MulScaleC(xMat3x3* o, const xMat3x3* m, F32 x, F32 y, F32 z)
{
    xASSERT_L(2996, (o != 0) && (m != 0));
    o->right.assign(m->right.x * x, m->right.y * x, m->right.z * x);
    o->up.assign(m->up.x * y, m->up.y * y, m->up.z * y);
    o->at.assign(m->at.x * z, m->at.y * z, m->at.z * z);
}

inline void xMat3x3MulScale(xMat3x3* o, const xMat3x3* m, xVec3* s)
{
    xASSERT_L(3021, (o != 0) && (m != 0) && (s != 0));
    return xMat3x3MulScaleC(o, m, s->x, s->y, s->z);
}

inline void xMat4x3ScaleC(xMat4x3* m, F32 x, F32 y, F32 z)
{
    xMat3x3ScaleC(m, x, y, z);
    xVec3Copy(&m->pos, &g_O3);
}

inline void xMat4x3Scale(xMat4x3* m, const xVec3* s)
{
    xMat4x3ScaleC(m, s->x, s->y, s->z);
}

inline F32 xMat3x3LookVec3(xMat3x3& mat, const xVec3& at)
{
    F32 mag = at.length();
    if (xfeq0(mag)) { mat = g_I3; return 0.0f; }
    mat.at = at;
    mat.at *= 1.0f / mag;

    F32 ax = xabs(mat.at.x), ay = xabs(mat.at.y), az = xabs(mat.at.z);
    if (ax < ay && ax < az) mat.right.assign(0.0f, mat.at.z, -mat.at.y);
    else if (ay < az) mat.right.assign(-mat.at.z, 0.0f, mat.at.x);
    else mat.right.assign(mat.at.y, -mat.at.x, 0.0f);

    mat.right.normalize();
    mat.up.cross(mat.right, mat.at);

    return mag;
}

void xMat4x3Copy(xMat4x3* o, const xMat4x3* m);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);
void xMat4x3MoveLocalRight(xMat4x3* m, F32 mag);
void xMat4x3MoveLocalUp(xMat4x3* m, F32 mag);
void xMat4x3MoveLocalAt(xMat4x3* m, F32 mag);

inline void xMat4x3From3x3(xMat4x3* m, const xMat3x3* m3x3);

inline void xMat4x3Identity(xMat4x3* m)
{
    xMat4x3Copy(m, &g_I3);
}

inline void xMat4x3ToWorld(xVec3* o, const xMat4x3* m, const xVec3* v)
{
    xMat3x3RMulVec(o, m, v);
    o->x = o->x + m->pos.x, o->y = o->y + m->pos.y, o->z = o->z + m->pos.z;
}

inline void xMat4x3ToLocal(xVec3* o, const xMat4x3* m, const xVec3* v)
{
    o->x = v->x - m->pos.x, o->y = v->y - m->pos.y, o->z = v->z - m->pos.z;
    xMat3x3Tolocal(o, m, o);
}

#endif
