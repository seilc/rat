#ifndef XBOUND_H
#define XBOUND_H

#include "xQuickCull.h"

enum xBoundType
{
    k_XBOUNDTYPE_NONE,
    k_XBOUNDTYPE_SPHERE,
    k_XBOUNDTYPE_BOX,
    k_XBOUNDTYPE_CYLINDER,
    k_XBOUNDTYPE_OBB,
    k_XBOUNDTYPE_COUNT,
    k_XBOUNDTYPE_SHIFT = 3
};

struct xBound
{
    xQCData qcd;
    U8 type;
    U8 pad[3];
    union
    {
        xSphere sph;
        xBBox box;
        xCylinder cyl;
    };
    xMat4x3* mat;
};

struct xCollis;
struct xEnv;
struct xModelInstance;

void xBoundMakeSphere(xBound& b, const xVec3& center, F32 radius); // UNUSED
void xBoundUpdate(xBound* b);
void xBoundGetBox(xBox& box, const xBound& bound);
void xBoundGetCylinder(xCylinder& o, const xBound& bound);
void xBoundGetSphere(xSphere& o, const xBound& bound);
void xBoundGetSphere2(xSphere& o, const xBound& bound); // UNUSED
void xBoundSphereHitsOBB(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
void xBoundHitsBound(const xBound* a, const xBound* b, xCollis* c);
void xBoundHitsEnv(const xBound*, const xEnv*, xCollis*); // UNUSED
void xBoundHitsModel(const xBound* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth);
void xRayHitsBound(const xRay3* r, const xBound* b, xCollis* c);
void xRayHitsBound(const xRay3* r, const xQCData* q, const xBound* b, xCollis* c); // UNUSED
void xSphereHitsBound(const xSphere* o, const xBound* b, xCollis* c);
void xSphereHitsBound(const xSphere* o, const xQCData* q, const xBound* b, xCollis* c); // UNUSED
void xVecHitsBound(const xVec3* v, const xBound* b, xCollis* c);
void xBoundDraw(const xBound* b);
bool xBoundHitsBound(const xBound& a, const xBound& b);
bool xRayHitsBound(const xRay3& r, const xBound& b); // UNUSED
bool xSphereHitsBound(const xVec3& c, F32 r, const xBound& b);
bool xVecHitsBound(const xVec3& v, const xBound& b); // UNUSED
bool xOBBHitsBound(const xBox& box, const xMat4x3& mat, const xBound& b);
void xBoundGetCenter(const xBound* b, xVec3* center);
bool xOBBHitsTriangle(const xBox& box, const xMat4x3& mat, const xVec3* vertices);

inline xVec3* xBoundCenter(const xBound* b) { return (xVec3*)&b->sph.center; }
inline xVec3* xBoundCenter(xBound* b) { return &b->sph.center; }

inline void xBoundFromSphere(xBound& bound, const xVec3& c, F32 r)
{
    bound.type = k_XBOUNDTYPE_SPHERE;
    bound.sph.center = c;
    bound.sph.r = r;
    xQuickCullForBound(&bound.qcd, &bound);
}

inline void xBoundFromSphere(xBound& bound, const xSphere& o) { xBoundFromSphere(bound, o.center, o.r); }

inline void xBoundFromOBB(xBound& bound, const xVec3& lower, const xVec3& upper, xMat4x3* mat)
{
    bound.type = k_XBOUNDTYPE_OBB;
    bound.box.box.lower = lower;
    bound.box.box.upper = upper;
    bound.box.center.x = 0.5f * (upper.x + lower.x);
    bound.box.center.y = 0.5f * (upper.y + lower.y);
    bound.box.center.z = 0.5f * (upper.z + lower.z);
    bound.mat = mat;
    xMat4x3ToWorld(&bound.box.center, bound.mat, &bound.box.center);
    xQuickCullForBound(&bound.qcd, &bound);
}

inline void xBoundFromOBB(xBound& bound, const xBox& box, xMat4x3* mat) { xBoundFromOBB(bound, box.lower, box.upper, mat); }

inline bool xSphereHitsBound(const xSphere& o, const xBound& b) { return xSphereHitsBound(o.center, o.r, b); }

#endif
