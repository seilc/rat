#ifndef XBOUND_H
#define XBOUND_H

#include "xQuickCull.h"

#define k_XBOUNDTYPE_NONE 0
#define k_XBOUNDTYPE_SPHERE 1
#define k_XBOUNDTYPE_BOX 2
#define k_XBOUNDTYPE_CYLINDER 3
#define k_XBOUNDTYPE_OBB 4

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

void xBoundMakeSphere(xBound& b, const xVec3& center, F32 radius);
void xBoundUpdate(xBound* b);
void xBoundGetBox(xBox& box, const xBound& bound);
void xBoundGetCylinder(xCylinder& o, const xBound& bound);
void xBoundGetSphere(xSphere& o, const xBound& bound);
void xBoundGetSphere2(xSphere& o, const xBound& bound);
void xBoundSphereHitsOBB(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
void xBoundHitsBound(const xBound* a, const xBound* b, xCollis* c);
void xBoundHitsEnv(const xBound*, const xEnv*, xCollis*);
void xBoundHitsModel(const xBound* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth);
void xRayHitsBound(const xRay3* r, const xBound* b, xCollis* c);
void xRayHitsBound(const xRay3* r, const xQCData* q, const xBound* b, xCollis* c);
void xSphereHitsBound(const xSphere* o, const xBound* b, xCollis* c);
void xSphereHitsBound(const xSphere* o, const xQCData* q, const xBound* b, xCollis* c);
void xVecHitsBound(const xVec3* v, const xBound* b, xCollis* c);
void xBoundDraw(const xBound* b);
bool xBoundHitsBound(const xBound& a, const xBound& b);
bool xRayHitsBound(const xRay3& r, const xBound& b);
bool xSphereHitsBound(const xVec3& c, F32 r, const xBound& b);
bool xVecHitsBound(const xVec3& v, const xBound& b);
bool xOBBHitsBound(const xBox& box, const xMat4x3& mat, const xBound& b);
void xBoundGetCenter(const xBound* b, xVec3* center);
bool xOBBHitsTriangle(const xBox& box, const xMat4x3& mat, const xVec3* vertices);

inline xVec3* xBoundCenter(const xBound* b) { return (xVec3*)&b->sph.center; }

#endif
