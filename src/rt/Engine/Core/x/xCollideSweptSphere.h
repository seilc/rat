#ifndef XCOLLIDESWEPTSPHERE_H
#define XCOLLIDESWEPTSPHERE_H

#include "xModel.h"
#include "xQuickCull.h"
#include "xJSP.h"

struct RpCollTree;
struct xBound;
struct xEnv;
struct xScene;
struct zPlayer;

typedef RwBool (*RpCollTreeLeafCB)(RwInt32 numTriangles, RwInt32 triOffset, void* data);

enum xCollideSphereHitType
{
    exCOLLIDESPHEREHITTYPE_UNKNOWN,
    exCOLLIDESPHEREHITTYPE_INTERIOR,
    exCOLLIDESPHEREHITTYPE_EDGE,
    exCOLLIDESPHEREHITTYPE_VERTEX
};

struct xSweptSphere
{
    bool detect_initial_penetration;
    bool init_collide;
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

void xSweptSphereDetectInitialPenetration(xSweptSphere& sws);
void xSweptSpherePrepare(xSweptSphere& sws, const xVec3& start, const xVec3& dir, F32 dist, F32 radius, U32 filterFlags);
void xSweptSpherePrepare(xSweptSphere* sws, const xVec3* start, const xVec3* end, F32 radius, U32 filterFlags);
void xSweptSphereResetCollision(xSweptSphere* sws);
void xSweptSphereGetResults(xSweptSphere* sws);
S32 xSweptSphereToEnv(xSweptSphere* sws, xEnv* env);
void RpCollTreeForAllCapsuleIntsec(RpCollTree* tree, RwLine* line, RwReal radius, RpCollTreeLeafCB callBack, void* data);
S32 xSweptSphereToModel(xSweptSphere* sws, RpAtomic* model, const RwMatrix* mat, bool singleSided);
S32 xSweptSphereToSphere(xSweptSphere& sws, const xVec3& center, F32 radius);
S32 xSweptSphereToBound(xSweptSphere* sws, const xBound* bound);
S32 xSweptSphereToTriangle(xSweptSphere& sws, const xVec3& v0, const xVec3& v1, const xVec3& v2);
S32 xSweptSphereToEnt(xSweptSphere* sws, xEnt* ent);
S32 xNPCSweptSphereToEnt(xSweptSphere* sws, xEnt* ent);
S32 xSweptSphereToEnt(xSweptSphere* sws, xEnt* ent, xRay3* ray);
S32 xNPCSweptSphereToEnt(xSweptSphere* sws, xEnt* ent, xRay3* ray);
S32 xSweptSphereToScene(xSweptSphere* sws, xScene* sc, const xEnt* mover, U8 collType, S32 penby);
S32 xSweptSphereToNonMoving(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby);
S32 xSweptSphereToStatDyn(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby);
S32 xNPCSweptSphereToStatDyn(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby, bool ignoreGoo);
S32 xSweptSphereToPlayer(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby);
S32 xSweptSphereToPlayer(xSweptSphere* sws, xScene* sc, xEnt* mover, zPlayer** player, U8 collType, S32 penby);
S32 xSweptSphereToNPC(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby);
#if DEBUG || RELEASE
void xSweptSphereDrawContact(xSweptSphere* sws);
#endif

inline S32 xSweptSphereToSphere(xSweptSphere* sws, const xSphere* sph)
{
    return xSweptSphereToSphere(*sws, sph->center, sph->r);
}

inline xSweptSphere::xSweptSphere() { xEntFilterFlags = 0; }

inline xSweptSphere::xSweptSphere(const xVec3& start, const xVec3& end, F32 radius, U16 filterFlags)
{
    xSweptSpherePrepare(this, &start, &end, radius, filterFlags);
}

#endif
