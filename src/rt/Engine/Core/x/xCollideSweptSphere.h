#ifndef XCOLLIDESWEPTSPHERE_H
#define XCOLLIDESWEPTSPHERE_H

#include "xModel.h"
#include "xQuickCull.h"
#include "xJSP.h"

struct xScene;

enum xCollideSphereHitType
{
    exCOLLIDESPHEREHITTYPE_UNKNOWN,
    exCOLLIDESPHEREHITTYPE_INTERIOR,
    exCOLLIDESPHEREHITTYPE_EDGE,
    exCOLLIDESPHEREHITTYPE_VERTEX
};

struct xSweptSphere
{
    U8 detect_initial_penetration;
    U8 init_collide;
    xVec3 start;
    xVec3 end;
    F32 radius;
    U16 xEntFilterFlags;
    F32 dist;
    xVec3 dir;
    xBox box;
    xQCData qcd;
    F32 curdist;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    xVec3 worldContact;
    S32 hitIt;
    xVec3 worldPos;
    xVec3 worldNormal;
    xVec3 worldTangent;
    xJSPHeader* jsp;
    xClumpCollBSPTriangle* bspTriangle;
    xCollideSphereHitType type;
    xPlane tri_plane;

    xSweptSphere();
    xSweptSphere(const xVec3& start, const xVec3& end, F32 radius, U16 filterFlags);
};

struct xCollideLineSegment
{
    xVec3 start;
    xVec3 dir;
    F32 length;
};

struct xCollideProcessedTri
{
    struct xEdge
    {
        xCollideLineSegment segment;
        xPlane edge;
    };

    xPlane surface;
    xEdge edges[3];
};

void xSweptSphereGetResults(xSweptSphere* sws);
void xSweptSpherePrepare(xSweptSphere* sws, const xVec3* start, const xVec3* end, F32 radius, U32 filterFlags);
void xSweptSphereResetCollision(xSweptSphere* sws);
S32 xSweptSphereToScene(xSweptSphere* sws, xScene* sc, const xEnt* mover, U8 collType, S32 penby);
S32 xSweptSphereToPlayer(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby);
S32 xSweptSphereToSphere(xSweptSphere& sws, const xVec3& center, F32 radius);

inline S32 xSweptSphereToSphere(xSweptSphere* sws, const xSphere* sph)
{
    return xSweptSphereToSphere(*sws, sph->center, sph->r);
}

inline xSweptSphere::xSweptSphere(const xVec3& start, const xVec3& end, F32 radius, U16 filterFlags)
{
    xSweptSpherePrepare(this, &start, &end, radius, filterFlags);
}

#endif
