#include "xCollideSweptSphere.h"

#include "xClumpColl.h"
#include "xCollide.h"
#include "xCollideFast.h"
#include "xCollideSphereBox.h"
#include "xCollideSpherePolygon.h"
#include "xCollideSpherePolygonUnprocessed.h"
#include "xDebug.h"
#include "xEnv.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xScene.h"
#include "zEntSimpleObj.h"
#include "zGoo.h"
#include "zGrid.h"
#include "zPlayer.h"
#include "zScene.h"
#include "zReactiveAnimation.h"

#include <rpcollis.h>

struct zSweptSphereGlobals
{
    S32 hitFound;
    xMat4x3* modelMat;
};

static zSweptSphereGlobals sSwsGlobal;

namespace {
    F32 Normalize(xVec3& v)
    {
        F32 len = v.length();

        if (len < 0.0001f) {
            return 0.0f;
        } else {
            xVec3ScaleC(v, 1.0f / len);
        }

        return len;
    }

    inline void xSweptSphereFillFromCollision(xSweptSphere* sws, const xCollis* coll)
    {
        sws->init_collide = true;
        sws->worldPos = sws->start;
        sws->worldNormal = coll->norm;
        sws->worldContact.Add(coll->tohit, sws->start);
        sws->optr = coll->optr;
        sws->mptr = coll->mptr;
        sws->oid = coll->oid;
    }

    inline bool GrossCullTri(xSweptSphere* sws, xVec3* v0, xVec3* v1, xVec3* v2)
    {
        F32 minX = MIN(v0->x, v1->x), maxX = MAX(v0->x, v1->x);
        minX = MIN(minX, v2->x), maxX = MAX(maxX, v2->x);
        if (sws->box.upper.x <= minX || sws->box.lower.x >= maxX) {
            return true;
        }

        F32 minY = MIN(v0->y, v1->y), maxY = MAX(v0->y, v1->y);
        minY = MIN(minY, v2->y), maxY = MAX(maxY, v2->y);
        if (sws->box.upper.y <= minY || sws->box.lower.y >= maxY) {
            return true;
        }

        F32 minZ = MIN(v0->z, v1->z), maxZ = MAX(v0->z, v1->z);
        minZ = MIN(minZ, v2->z), maxZ = MAX(maxZ, v2->z);
        if (sws->box.upper.z <= minZ || sws->box.lower.z >= maxZ) {
            return true;
        }

        return false;
    }
}

void xSweptSphereDetectInitialPenetration(xSweptSphere& sws)
{
    sws.detect_initial_penetration = true;
}

void xSweptSpherePrepare(xSweptSphere& sws, const xVec3& start, const xVec3& dir, F32 dist, F32 radius, U32 filterFlags)
{
    sws.detect_initial_penetration = false;

    sws.xEntFilterFlags = filterFlags;
    sws.jsp = NULL;
    sws.bspTriangle = NULL;
    sws.type = exCOLLIDESPHEREHITTYPE_UNKNOWN;

    sws.start = start;
    sws.end.AddScale(start, dir, dist);
    sws.radius = radius;
    sws.optr = NULL;
    sws.mptr = NULL;

    sws.dist = dist;
    if (sws.dist < 0.0001f) {
        sws.dist = 0.0f;
        sws.curdist = 0.0f;
        return;
    }

    sws.dir = dir;

    sws.curdist = sws.dist;
    sws.init_collide = false;
    sws.detect_initial_penetration = false;

    xCapsule tmpC;
    tmpC.start = start;
    tmpC.end = sws.end;
    tmpC.r = radius;
    xBoxInitBoundCapsule(&sws.box, &tmpC);
    xQuickCullForBox(&sws.qcd, &sws.box);
}

void xSweptSpherePrepare(xSweptSphere* sws, const xVec3* start, const xVec3* end, F32 radius, U32 filterFlags)
{
    sws->xEntFilterFlags = filterFlags;
    sws->jsp = NULL;
    sws->bspTriangle = NULL;
    sws->type = exCOLLIDESPHEREHITTYPE_UNKNOWN;

    sws->start = *start;
    sws->end = *end;
    sws->radius = radius;
    sws->optr = NULL;
    sws->mptr = NULL;

    xVec3Sub(sws->dir, sws->end, sws->start);
    sws->dist = xVec3NormalizeSafe(sws->dir);

    if (sws->dist < 0.0001f) {
        sws->dist = 0.0f;
        sws->curdist = 0.0f;
        return;
    }

    sws->curdist = sws->dist;
    sws->init_collide = false;
    sws->detect_initial_penetration = false;

    xCapsule tmpC;
    tmpC.start = *start;
    tmpC.end = *end;
    tmpC.r = radius;
    xBoxInitBoundCapsule(&sws->box, &tmpC);
    xQuickCullForBox(&sws->qcd, &sws->box);
}

void xSweptSphereResetCollision(xSweptSphere* sws)
{
    sws->curdist = sws->dist;
    sws->optr = NULL;
    sws->mptr = NULL;
    sws->init_collide = false;
    sws->bspTriangle = NULL;
    sws->type = exCOLLIDESPHEREHITTYPE_UNKNOWN;
}

void xSweptSphereGetResults(xSweptSphere* sws)
{
    sws->hitIt = FALSE;
    if (sws->init_collide) {
        return;
    }

    if (!sws->dist) {
        return;
    }

    if (sws->curdist == sws->dist) return;

    F32 tandot;
    xVec3 tanplane;

    sws->hitIt = TRUE;

    sws->worldPos.AddScale(sws->start, sws->dir, sws->curdist);

    sws->worldNormal.x = sws->worldPos.x - sws->worldContact.x;
    sws->worldNormal.y = sws->worldPos.y - sws->worldContact.y;
    sws->worldNormal.z = sws->worldPos.z - sws->worldContact.z;
    xVec3Normalize(&sws->worldNormal, &sws->worldNormal);

    xVec3Cross(&tanplane, &sws->dir, &sws->worldNormal);
    if (xVec3Length2(&tanplane) < 0.0000001f) {
        xVec3 in_plane;
        xMat3x3LookVec(sws->dir, in_plane, tanplane);
    }
    xVec3Cross(&sws->worldTangent, &tanplane, &sws->worldNormal);
    xVec3Normalize(&sws->worldTangent, &sws->worldTangent);
    tandot = sws->worldTangent.x * sws->dir.x + sws->worldTangent.y * sws->dir.y + sws->worldTangent.z * sws->dir.z;
    if (tandot < 0.0f) {
        sws->worldTangent.x = -sws->worldTangent.x;
        sws->worldTangent.y = -sws->worldTangent.y;
        sws->worldTangent.z = -sws->worldTangent.z;
    }
}

inline bool TestSingleEnvTri(xSweptSphere* sws, xClumpCollBSPTriangle* triangle, xCollideSphereToPolygonResults& tri_collide, xPlane& surface)
{
    if (!xClumpColl_IsCollidableTriangle(triangle->flags)) {
        return false;
    }

    xVec3* p0, *p1, *p2;

    if (triangle->flags & 0x2) {
        p0 = (xVec3*)triangle->v.p;
        p1 = (xVec3*)(triangle->v.p + 1);
        p2 = (xVec3*)(triangle->v.p + 2);
    } else {
        p0 = (xVec3*)triangle->v.p;
        p1 = (xVec3*)(triangle->v.p + 2);
        p2 = (xVec3*)(triangle->v.p + 1);
    }

    xBox tri_box;
    xBoxForTriangle(tri_box, *p0, *p1, *p2);

    if (!xBoxHitsBox(tri_box, sws->box)) {
        return false;
    }

    return xSweptSphereToTriangleUnprocessed(*sws, *p0, *p1, *p2, tri_collide, surface);
}

static S32 SweptSphereLeafNodeCB(xClumpCollBSPTriangle* triangles, void* data)
{
    xSweptSphere* sws = (xSweptSphere*)data;

    for (;; triangles++) {
        xCollideSphereToPolygonResults tri_collide;
        xPlane surface;

        if (TestSingleEnvTri(sws, triangles, tri_collide, surface) == true) {
            sws->worldContact = tri_collide.contact;
            sws->tri_plane = surface;
            sws->curdist = tri_collide.dist;
            sws->oid = triangles->matIndex;
            sws->optr = NULL;
            sws->mptr = NULL;
            sws->bspTriangle = triangles;
            sws->type = tri_collide.type;
            sSwsGlobal.hitFound = TRUE;
        }

        if (!(triangles->flags & 0x1)) {
            break;
        }
    }

    return TRUE;
}

S32 xSweptSphereToEnv(xSweptSphere* sws, xEnv* env)
{
    if (!sws->curdist) {
        return FALSE;
    }

    if (sws->detect_initial_penetration) {
        xCollis coll; xSphere s;
        coll.optr = NULL;
        coll.mptr = NULL;
        coll.flags = 0x600;
        s.center = sws->start;
        s.r = sws->radius;
        xSphereHitsEnv(&s, env, &coll);

        if (coll.flags & k_HIT_IT) {
            xSweptSphereFillFromCollision(sws, &coll);
            return TRUE;
        }
    }

    xASSERT_L(385, sws);
    xASSERT_L(386, env);
    sSwsGlobal.hitFound = FALSE;

    xASSERT_L(389, env->geom->jsp_count > 0);

    RwLine line;
    RwV3d delta;
    xClumpCollV3dGradient grad;

    line.start = *(RwV3d*)&sws->start;
    line.end = *(RwV3d*)&sws->end;
    RwV3dSub(&delta, &line.end, &line.start);
    xClumpCollV3dGradientMacro(&grad, &delta);

    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (!xEnvIsJSPActive(env, i)) {
            continue;
        }

        if (!xBoxHitsBox(sws->box, env->ienv.jsp_bound[i])) {
            continue;
        }

        F32 testdist = sws->curdist;

        sws->bspTriangle = NULL;

        sws->jsp = env->geom->jsp_list[i];
        xClumpColl_ForAllCapsuleLeafNodeIntersections(env->geom->jsp_list[i]->colltree, &line, sws->radius, &grad, SweptSphereLeafNodeCB, sws);

        if (sws->curdist >= testdist) {
            continue;
        }

        sws->oid = get_matid(env->geom->jsp_list[i], sws->oid, 0);

        xSweptSphere newsws = *sws;

        for (i++; i < env->geom->jsp_count; i++) {
            if (!xEnvIsJSPActive(env, i)) {
                continue;
            }

            if (!xBoxHitsBox(newsws.box, env->ienv.jsp_bound[i])) {
                continue;
            }

            F32 testdist = newsws.curdist;

            newsws.bspTriangle = NULL;

            newsws.jsp = env->geom->jsp_list[i];
            xClumpColl_ForAllCapsuleLeafNodeIntersections(env->geom->jsp_list[i]->colltree, &line, newsws.radius, &grad, SweptSphereLeafNodeCB, &newsws);

            if (newsws.curdist >= testdist) {
                continue;
            }

            *sws = newsws;
        }

        break;
    }

    return sSwsGlobal.hitFound;
}

struct SweptSphereCollParam
{
    RpGeometry* geometry;
    xSweptSphere* sws;
    bool singleSided;
};

static RwBool SweptSphereModelCB(RwInt32 numTriangles, RwInt32 triOffset, void* data)
{
    SweptSphereCollParam* isData = (SweptSphereCollParam*)data;
    RpGeometry* geometry = isData->geometry;
    xSweptSphere* sws = isData->sws;
    RwV3d* vertices = geometry->morphTarget->verts;
    RpTriangle* triangles = geometry->triangles;
    S32 triSlot;
    U16* triIndex = _rpCollisionGeometryGetDataMacro(geometry)->tree->map;

    if (triIndex) {
        triIndex += triOffset;
        triSlot = 0;
    } else {
        triSlot = triOffset;
    }

    while (numTriangles--) {
        if (triIndex) {
            triSlot = *triIndex;
            triIndex++;
        } else {
            triSlot++;
        }

        RpTriangle* tri = &triangles[triSlot];
        S32 vertIndex0 = tri->vertIndex[0];
        S32 vertIndex1 = tri->vertIndex[1];
        S32 vertIndex2 = tri->vertIndex[2];
        RwV3d* v0 = &vertices[vertIndex0];
        RwV3d* v1 = &vertices[vertIndex1];
        RwV3d* v2 = &vertices[vertIndex2];

        xBox tri_box;
        xBoxForTriangle(tri_box, *(xVec3*)v0, *(xVec3*)v1, *(xVec3*)v2);

        if (!xBoxHitsBox(tri_box, sws->box)) {
            continue;
        }

        xCollideSphereToPolygonResults tri_collide;

        if ((isData->singleSided && xSweptSphereToTriangleUnprocessed(*sws, *(xVec3*)v0, *(xVec3*)v1, *(xVec3*)v2, tri_collide, sws->tri_plane)) ||
            (!isData->singleSided && xSweptSphereToTriangleDoubleSidedUnprocessed(*sws, *(xVec3*)v0, *(xVec3*)v1, *(xVec3*)v2, tri_collide, sws->tri_plane))) {
            xMat4x3ToWorld(&sws->worldContact, sSwsGlobal.modelMat, &tri_collide.contact);
            sws->curdist = tri_collide.dist;
            sws->type = tri_collide.type;
            sSwsGlobal.hitFound = TRUE;
        }
    }

    return TRUE;
}

struct RpCollSectorCapsuleIt
{
    RpCollTree* tree;
    RwV3d start;
    RwV3d delta;
    RwV3d recip;
    RwReal radius;
    RpCollSector dummy;
    RpCollSectorLineItNodeInfo stack[rpCOLLTREE_MAXDEPTH + 1];
    RpCollSectorLineItNodeInfo* curr;
};

#define _rpCollCapsuleIntervalTestMacro(_result, _start, _end, _inf, _sup, _radius, _pa, _pb)     \
MACRO_START                                                                                       \
{                                                                                                 \
    if ((_start) == (_end)) {                                                                     \
        (_result) = ((_start) >= (_inf) - (_radius) && (_start) <= (_sup) + (_radius));           \
    } else {                                                                                      \
        RwReal recip = 1.0f / ((_end) - (_start));                                                \
        RwReal p0, p1;                                                                            \
        if (recip > 0.0f) {                                                                       \
            p0 = recip * ((_inf) - (_radius) - (_start));                                         \
            p1 = recip * ((_sup) + (_radius) - (_start));                                         \
        } else {                                                                                  \
            p1 = recip * ((_inf) - (_radius) - (_start));                                         \
            p0 = recip * ((_sup) + (_radius) - (_start));                                         \
        }                                                                                         \
        (_result) = ((_pa) <= p1) && ((_pb) >= p0);                                               \
        if (_result) {                                                                            \
            (_pa) = RwRealMax2(_pa, p0);                                                          \
            (_pb) = RwRealMin2(_pb, p1);                                                          \
        }                                                                                         \
    }                                                                                             \
}                                                                                                 \
MACRO_STOP

#define _rpCollCapsuleBBoxTestMacro(_result, _line, _bb, _radius, _pa, _pb)                                                                         \
MACRO_START                                                                                                                                         \
{                                                                                                                                                   \
    _rpCollCapsuleIntervalTestMacro(_result, (_line)->start.x, (_line)->end.x, (_bb)->inf.x, (_bb)->sup.x, _radius, _pa, _pb);                      \
    if (_result) {                                                                                                                                  \
        _rpCollCapsuleIntervalTestMacro(_result, (_line)->start.y, (_line)->end.y, (_bb)->inf.y, (_bb)->sup.y, _radius, _pa, _pb);                  \
        if (_result) {                                                                                                                              \
            _rpCollCapsuleIntervalTestMacro(_result, (_line)->start.z, (_line)->end.z, (_bb)->inf.z, (_bb)->sup.z, _radius, _pa, _pb);              \
        }                                                                                                                                           \
    }                                                                                                                                               \
}                                                                                                                                                   \
MACRO_STOP

#define _rpCollSectorItInitCollTreeMacro(_it, _tree)                                              \
{                                                                                                 \
    (_it)->tree = (_tree);                                                                        \
    (_it)->dummy.index = 0;                                                                       \
    (_it)->dummy.contents = rpCOLLSECTOR_CONTENTS_SPLIT;                                          \
    if ((_tree)->numSplits == 0) {                                                                \
        (_it)->dummy.contents = (RwUInt8)(_tree)->numEntries;                                     \
    }                                                                                             \
}

#define _rpCollCapsuleItInitMacro(_it, _root, _line, _radius, _pa, _pb)                           \
MACRO_START                                                                                       \
{                                                                                                 \
    (_it)->start = (_line)->start;                                                                \
    RwV3dSub(&(_it)->delta, &(_line)->end, &(_line)->start);                                      \
    (_it)->recip.x = ((_it)->delta.x != 0.0f) ? (1.0f / (_it)->delta.x) : 0.0f;                   \
    (_it)->recip.y = ((_it)->delta.y != 0.0f) ? (1.0f / (_it)->delta.y) : 0.0f;                   \
    (_it)->recip.z = ((_it)->delta.z != 0.0f) ? (1.0f / (_it)->delta.z) : 0.0f;                   \
    (_it)->radius = (_radius);                                                                    \
    (_it)->stack[0].sector = (_root);                                                             \
    (_it)->stack[0].pa = (_pa) - 0.00001f;                                                        \
    (_it)->stack[0].pb = (_pb) + 0.00001f;                                                        \
    (_it)->curr = (_it)->stack + 1;                                                               \
}                                                                                                 \
MACRO_STOP

#ifdef RWDEBUG

inline void _rpCollSectorCapsuleItInitMacro(RpCollSectorCapsuleIt* _it, RpCollTree* _tree, RwLine* _line, RwReal _radius)
{
    RwReal pa = 0.0f, pb = 1.0f;
    RwBool result;

    _rpCollCapsuleBBoxTestMacro(result, _line, &_tree->bbox, _radius, pa, pb);
    if (result) {
        _rpCollSectorItInitCollTreeMacro(_it, _tree);
        _rpCollCapsuleItInitMacro(_it, &_it->dummy, _line, _radius, pa, pb);
    } else {
        _it->curr = _it->stack;
    }
}

inline void _rpCollCapsuleItStepMacro(RpCollSectorCapsuleIt* _it, RwUInt32 _axis, RpCollSector* _left, RpCollSector* _right, RwReal _leftHi, RwReal _rightLo)
{
    RwReal del, sta, end, rec, rad;

    del = GETCOORD(_it->delta, _axis);
    sta = GETCOORD(_it->start, _axis);
    rad = _it->radius;
    end = sta + _it->curr->pb * del;
    sta = sta + _it->curr->pa * del;

    if (del > 0.0f) {
        if (_rightLo > end + rad) {
            _it->curr->sector = _left;
        }
        else if (_leftHi < sta - rad) {
            _it->curr->sector = _right;
        } else {
            _it->curr++;
            rec = GETCOORD(_it->recip, _axis);
            _it->curr->sector = _left;
            _it->curr->pa = (_it->curr - 1)->pa;
            _it->curr->pb = (_it->curr - 1)->pb;
            if (_leftHi < end) {
                _it->curr->pb += rec * (_leftHi - end) + 0.00001f;
            }
            (_it->curr - 1)->sector = _right;
            if (_rightLo > sta) {
                (_it->curr - 1)->pa += rec * (_rightLo - sta) - 0.00001f;
            }
        }
    } else {
        if (_leftHi < end - rad) {
            _it->curr->sector = _right;
        }
        else if (_rightLo > sta + rad) {
            _it->curr->sector = _left;
        } else {
            _it->curr++;
            rec = GETCOORD(_it->recip, _axis);
            _it->curr->sector = _right;
            _it->curr->pa = (_it->curr - 1)->pa;
            _it->curr->pb = (_it->curr - 1)->pb;
            if (_rightLo > end) {
                _it->curr->pb += rec * (_rightLo - end) + 0.00001f;
            }
            (_it->curr - 1)->sector = _left;
            if (_leftHi < sta) {
                (_it->curr - 1)->pa += rec * (_leftHi - sta) - 0.00001f;
            }
        }
    }
}

inline void _rpCollSectorCapsuleItFindNextEntriesMacro(RpCollSectorCapsuleIt* _it, RwUInt16* _start, RwUInt16* _count)
{
    _it->curr--;
    while (_it->curr->sector->contents == rpCOLLSECTOR_CONTENTS_SPLIT) {
        RpCollSplit* split;
        RwUInt32 axis;
        split = _it->tree->splits + _it->curr->sector->index;
        axis = split->leftSector.type & rpCOLLSECTOR_TYPE_AXISMASK;
        _rpCollCapsuleItStepMacro(_it, axis, &split->leftSector, &split->rightSector, split->leftSector.value, split->rightSector.value);
    }
    *_start = _it->curr->sector->index;
    *_count = _it->curr->sector->contents;
}

#else

#define _rpCollCapsuleItStepMacro(_it, _axis, _left, _right, _leftHi, _rightLo)                   \
MACRO_START                                                                                       \
{                                                                                                 \
    RwReal del, sta, end, rec, rad;                                                               \
                                                                                                  \
    del = GETCOORD((_it)->delta, (_axis));                                                        \
    sta = GETCOORD((_it)->start, (_axis));                                                        \
    rad = (_it)->radius;                                                                          \
    end = sta + (_it)->curr->pb * del;                                                            \
    sta = sta + (_it)->curr->pa * del;                                                            \
                                                                                                  \
    if (del > 0.0f) {                                                                             \
        if ((_rightLo) > end + rad) {                                                             \
            (_it)->curr->sector = (_left);                                                        \
        } else if ((_leftHi) < sta - rad) {                                                       \
            (_it)->curr->sector = (_right);                                                       \
        } else {                                                                                  \
            (_it)->curr++;                                                                        \
            rec = GETCOORD((_it)->recip, (_axis));                                                \
            (_it)->curr->sector = (_left);                                                        \
            (_it)->curr->pa = ((_it)->curr - 1)->pa;                                              \
            (_it)->curr->pb = ((_it)->curr - 1)->pb;                                              \
            if ((_leftHi) < end) {                                                                \
                (_it)->curr->pb += rec * ((_leftHi) - end) + 0.00001f;                            \
            }                                                                                     \
            ((_it)->curr - 1)->sector = (_right);                                                 \
            if ((_rightLo) > sta) {                                                               \
                ((_it)->curr - 1)->pa += rec * ((_rightLo) - sta) - 0.00001f;                     \
            }                                                                                     \
        }                                                                                         \
    } else {                                                                                      \
        if ((_leftHi) < end - rad) {                                                              \
            (_it)->curr->sector = (_right);                                                       \
        } else if ((_rightLo) > sta + rad) {                                                      \
            (_it)->curr->sector = (_left);                                                        \
        } else {                                                                                  \
            (_it)->curr++;                                                                        \
            rec = GETCOORD((_it)->recip, (_axis));                                                \
            (_it)->curr->sector = (_right);                                                       \
            (_it)->curr->pa = ((_it)->curr - 1)->pa;                                              \
            (_it)->curr->pb = ((_it)->curr - 1)->pb;                                              \
            if ((_rightLo) > end) {                                                               \
                (_it)->curr->pb += rec * ((_rightLo) - end) + 0.00001f;                           \
            }                                                                                     \
            ((_it)->curr - 1)->sector = (_left);                                                  \
            if ((_leftHi) < sta) {                                                                \
                ((_it)->curr - 1)->pa += rec * ((_leftHi) - sta) - 0.00001f;                      \
            }                                                                                     \
        }                                                                                         \
    }                                                                                             \
}                                                                                                 \
MACRO_STOP

#define _rpCollSectorCapsuleItFindNextEntriesMacro(_it, _start, _count)                                                                             \
MACRO_START                                                                                                                                         \
{                                                                                                                                                   \
    (_it)->curr--;                                                                                                                                  \
    while ((_it)->curr->sector->contents == rpCOLLSECTOR_CONTENTS_SPLIT) {                                                                          \
        RpCollSplit* split;                                                                                                                         \
        RwUInt32 axis;                                                                                                                              \
        split = (_it)->tree->splits + (_it)->curr->sector->index;                                                                                   \
        axis = split->leftSector.type & rpCOLLSECTOR_TYPE_AXISMASK;                                                                                 \
        _rpCollCapsuleItStepMacro(_it, axis, &split->leftSector, &split->rightSector, split->leftSector.value, split->rightSector.value);           \
    }                                                                                                                                               \
    *(_start) = (_it)->curr->sector->index;                                                                                                         \
    *(_count) = (_it)->curr->sector->contents;                                                                                                      \
}                                                                                                                                                   \
MACRO_STOP

#define _rpCollSectorCapsuleItInitMacro(_it, _tree, _line, _radius)                               \
MACRO_START                                                                                       \
{                                                                                                 \
    RwReal pa = 0.0f, pb = 1.0f;                                                                  \
    RwBool result;                                                                                \
                                                                                                  \
    _rpCollCapsuleBBoxTestMacro(result, _line, &(_tree)->bbox, _radius, pa, pb);                  \
    if (result) {                                                                                 \
        _rpCollSectorItInitCollTreeMacro(_it, _tree);                                             \
        _rpCollCapsuleItInitMacro(_it, &(_it)->dummy, _line, _radius, pa, pb);                    \
    } else {                                                                                      \
        (_it)->curr = (_it)->stack;                                                               \
    }                                                                                             \
}                                                                                                 \
MACRO_STOP

#endif

void RpCollTreeForAllCapsuleIntsec(RpCollTree* tree, RwLine* line, RwReal radius, RpCollTreeLeafCB callBack, void* data)
{
    RpCollSectorCapsuleIt it;

    _rpCollSectorCapsuleItInitMacro(&it, tree, line, radius);
    while (it.curr > it.stack) {
        RwUInt16 start, count;
        _rpCollSectorCapsuleItFindNextEntriesMacro(&it, &start, &count);
        callBack(count, start, data);
    }
}

S32 xSweptSphereToModel(xSweptSphere* sws, RpAtomic* model, const RwMatrix* mat, bool singleSided)
{
    xASSERT_L(1237, sws);
    xASSERT_L(1238, model);
    xASSERT_L(1239, mat);

    if (sws->dist <= 0.0f || sws->curdist <= 0.0f) {
        return FALSE;
    }

    sSwsGlobal.hitFound = FALSE;
    sSwsGlobal.modelMat = (xMat4x3*)mat;

    xVec3 old_start = sws->start, old_end = sws->end, old_dir = sws->dir;
    F32 old_radius = sws->radius;
    xBox old_box = sws->box;
    F32 old_dist = sws->dist;
    F32 model_scale = ((const xMat4x3*)mat)->at.length();

    xMat4x3ToLocal(&sws->start, (const xMat4x3*)mat, &old_start);
    xMat4x3ToLocal(&sws->end, (const xMat4x3*)mat, &old_end);

    xVec3Sub(sws->dir, sws->end, sws->start);
    sws->dist = Normalize(sws->dir);

    if (sws->dist == 0.0f) {
        sws->start = old_start;
        sws->end = old_end;
        sws->dir = old_dir;
        sws->dist = old_dist;
        return FALSE;
    }

    F32 old_curdist = sws->curdist;
    sws->radius /= model_scale;
    sws->curdist /= model_scale;
    F32 start_curdist = sws->curdist;

    xBoxFromCapsule(sws->box, sws->start, sws->end, sws->radius);

    RpGeometry* geom = model->geometry;
    RpCollisionData* colldata = _rpCollisionGeometryGetDataMacro(geom);
    if (colldata && colldata->tree) {
        RwLine line;
        F32 scale;

        xsqrtfast(scale, RwV3dDotProductMacro(&mat->right, &mat->right));
        xASSERT_L(1308, scale > 0.0000001f);

        (xVec3&)line.start = sws->start;
        (xVec3&)line.end = sws->end;

        SweptSphereCollParam isData;
        isData.geometry = geom;
        isData.sws = sws;
        isData.singleSided = singleSided;

        RpCollTreeForAllCapsuleIntsec(colldata->tree, &line, sws->radius, SweptSphereModelCB, &isData);
    } else {
        RpTriangle* tri = geom->triangles;
        RwV3d* vert = geom->morphTarget->verts;

        for (S32 i = 0; i < geom->numTriangles; i++) {
            xVec3* p0 = (xVec3*)&vert[tri->vertIndex[0]];
            xVec3* p1 = (xVec3*)&vert[tri->vertIndex[1]];
            xVec3* p2 = (xVec3*)&vert[tri->vertIndex[2]];

            if (!GrossCullTri(sws, p0, p1, p2)) {
                xCollideSphereToPolygonResults tri_collide;
                xPlane surface;

                if ((singleSided && xSweptSphereToTriangleUnprocessed(*sws, *p0, *p1, *p2, tri_collide, surface)) ||
                    (!singleSided && xSweptSphereToTriangleDoubleSidedUnprocessed(*sws, *p0, *p1, *p2, tri_collide, surface))) {
                    xMat4x3ToWorld(&sws->worldContact, sSwsGlobal.modelMat, &tri_collide.contact);
                    sws->curdist = tri_collide.dist;
                    sws->type = tri_collide.type;
                    sSwsGlobal.hitFound = TRUE;
                }
            }

            tri++;
        }
    }

    sws->start = old_start;
    sws->end = old_end;
    sws->dir = old_dir;
    sws->radius = old_radius;
    sws->dist = old_dist;

    if (sws->curdist < start_curdist)
        sws->curdist *= model_scale;
    else
        sws->curdist = old_curdist;

    sws->box = old_box;

    return sSwsGlobal.hitFound;
}

#if DEBUG || RELEASE
#define _xFIX_NAN_VEC3_ELEM(line, x)            \
if ((x) && !((x) > 0.0f) && !((x) < 0.0f)) {    \
    xFAIL_FMT_L(line, "%s", "NAN");             \
    (x) = 0.0f;                                 \
}

#define xFIX_NAN_VEC3_L(line, v)                \
    _xFIX_NAN_VEC3_ELEM(line, (v).x);           \
    _xFIX_NAN_VEC3_ELEM(line, (v).y);           \
    _xFIX_NAN_VEC3_ELEM(line, (v).z)
#else
#define xFIX_NAN_VEC3_L(line, v)
#endif

#define xFIX_NAN_VEC3(v) xFIX_NAN_VEC3_L(__LINE__, v)

S32 xSweptSphereToSphere(xSweptSphere& sws, const xVec3& center, F32 radius)
{
    xFIX_NAN_VEC3_L(1423, (xVec3&)center);
    xFIX_NAN_VEC3_L(1424, sws.start);

    if (sws.curdist <= 0.0f) {
        return FALSE;
    }

    xVec3 center_to_center;
    xVec3Sub(center_to_center, center, sws.start);

    F32 dist_along = center_to_center.dot(sws.dir);

    if (dist_along < 0.001f) {
        return FALSE;
    }

    xVec3 along;
    xVec3SMul(along, sws.dir, dist_along);

    xVec3 out;
    xVec3Sub(out, center_to_center, along);

    F32 out_dist_squ = out.length2();

    F32 combined_raidus = sws.radius + radius;
    F32 combined_radius_squ = xsqr(combined_raidus);

    if (0.0001f + out_dist_squ >= combined_radius_squ) {
        return FALSE;
    }

    F32 stop_dist = xsqrt(combined_radius_squ - out_dist_squ);
    F32 dist_to_collide = dist_along - stop_dist;

    if (dist_to_collide < 0.0001f) {
        sws.curdist = 0.0f;

        F32 dist_between = Normalize(center_to_center);

        sws.worldContact.AddScale(center, center_to_center, -dist_between * (radius / combined_raidus));

        return TRUE;
    }

    if (sws.curdist <= dist_to_collide) {
        return FALSE;
    }

    sws.curdist = dist_to_collide;

    F32 ratio = sws.radius / combined_raidus;
    sws.worldContact.AddScale(sws.start, out, ratio);
    sws.worldContact.AddScale(sws.dir, ratio * stop_dist + sws.curdist);

    return TRUE;
}

S32 xSweptSphereToBound(xSweptSphere* sws, const xBound* bound)
{
    switch (bound->type) {
    case k_XBOUNDTYPE_SPHERE:
        return xSweptSphereToSphere(sws, &bound->sph);
    case k_XBOUNDTYPE_BOX:
        return xSweptSphereToBox(sws, &bound->box.box);
    case k_XBOUNDTYPE_OBB:
        return xSweptSphereToOBB(sws, &bound->box.box, bound->mat);
    default:
        xFAIL_FMT_L(1558, "%s", "unsupported bound type");
        return FALSE;
    }
}

static S32 sSweptSphereEntFound;
static const xEnt* sSweptSphereMover;
static U8 sSweptSphereCollType;
static S32 sSweptSpherePenby;
static U32 sSweptSphereIgnoreMovers;
static bool sSweptSphereNPCIgnoreGoo;

static void SweptSphereHitsEntCB(xScene*, xRay3* ray, xQCData* qcd, xEnt* ent, void* data)
{
    xSweptSphere* sws = (xSweptSphere*)data;

    if ((ent->flags | (U8)sws->xEntFilterFlags) != ent->flags) {
        return;
    }

    if (!xQuickCullIsects(qcd, &ent->bound.qcd)) {
        return;
    }

    if (ent == sSweptSphereMover) {
        return;
    }

    if (!(sSweptSphereCollType & ent->chkby)) {
        return;
    }

    if (sSweptSpherePenby && !(sSweptSpherePenby & ent->penby)) {
        return;
    }

    if (sSweptSphereIgnoreMovers) {
        xModelInstance* collmod = ent->collModel ? ent->collModel : ent->model;
        if (collmod->Flags & 0x800) {
            return;
        }

        if (ent->frame) {
            xMat4x3* m1 = &ent->frame->oldmat;
            xMat4x3* m2 = (xMat4x3*)ent->model->Mat;
            if (m1->pos.x != m2->pos.x || m1->pos.y != m2->pos.y || m1->pos.z != m2->pos.z ||
                m1->right.x != m2->right.x || m1->right.y != m2->right.y || m1->right.z != m2->right.z ||
                m1->up.y != m2->up.y || m1->up.z != m2->up.z || m1->at.z != m2->at.z) {
                return;
            }
        }
    }

    sSweptSphereEntFound |= xSweptSphereToEnt(sws, ent, ray);
}

static void NPCSweptSphereHitsEntCB(xScene*, xRay3* ray, xQCData* qcd, xEnt* ent, void* data)
{
    xSweptSphere* sws = (xSweptSphere*)data;
    S32 goo = zGooIs(ent);

    if (goo && sSweptSphereNPCIgnoreGoo) {
        return;
    }

    if (!goo) {
        if ((ent->flags | (U8)sws->xEntFilterFlags) != ent->flags) return;
    }

    if (!xQuickCullIsects(qcd, &ent->bound.qcd)) {
        return;
    }

    if (ent == sSweptSphereMover) {
        return;
    }

    if (!(sSweptSphereCollType & ent->chkby)) {
        return;
    }

    if (sSweptSpherePenby && !(sSweptSpherePenby & ent->penby)) {
        return;
    }

    if (sSweptSphereIgnoreMovers) {
        xModelInstance* collmod = ent->collModel ? ent->collModel : ent->model;
        if (collmod->Flags & 0x800) {
            return;
        }

        if (ent->frame) {
            xMat4x3* m1 = &ent->frame->oldmat;
            xMat4x3* m2 = (xMat4x3*)ent->model->Mat;
            if (m1->pos.x != m2->pos.x || m1->pos.y != m2->pos.y || m1->pos.z != m2->pos.z ||
                m1->right.x != m2->right.x || m1->right.y != m2->right.y || m1->right.z != m2->right.z ||
                m1->up.y != m2->up.y || m1->up.z != m2->up.z || m1->at.z != m2->at.z) {
                return;
            }
        }
    }

    sSweptSphereEntFound |= xNPCSweptSphereToEnt(sws, ent, ray);
}

namespace {
    inline bool sphereToEntResults(xSweptSphere* sws, xEnt* ent, bool react)
    {
        if (react) {
            zReactiveDamage((zEntSimpleObj&)*ent, zHS_GENERAL, 10.0f);
            return false;
        }

        sws->mptr = ent->model;
        sws->oid = ent->id;
        sws->optr = ent;
        return true;
    }
}

S32 xSweptSphereToEnt(xSweptSphere* sws, xEnt* ent, xRay3* ray)
{
    F32 testdist = sws->curdist;

    if ((ent->flags | (U8)sws->xEntFilterFlags) != ent->flags) {
        return FALSE;
    }

    bool react = sSweptSphereCollType == 0x8 && zReactiveIsReactive(*ent);
    xSweptSphere sweptSphere;
    if (react) {
        sweptSphere = *sws;
        sws = &sweptSphere;
    }

    if (ent->collLev == 5) {
        U32 result = FALSE;

        switch (ent->bound.type) {
        case k_XBOUNDTYPE_SPHERE: {
            F32 oldrad = ent->bound.sph.r;
            ent->bound.sph.r += sws->radius;
            result = xRayHitsSphereFast(ray, &ent->bound.sph);
            ent->bound.sph.r = oldrad;
        }
            break;
        case k_XBOUNDTYPE_BOX: {
            xBox tmpbox;
            tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius,
            tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius,
            tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius,
            tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius,
            tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius,
            tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius;
            result = xRayHitsBoxFast(ray, &tmpbox);
        }
            break;
        case k_XBOUNDTYPE_OBB: {
            xBox tmpbox;
            xRay3 lr;
            xMat3x3 mn;
            F32 ms = xVec3Length(&ent->bound.mat->right);
            xVALIDATE_L(1811, xeq(ms*ms,xVec3Length2(&ent->bound.mat->up),1e-3f));
            xVALIDATE_L(1812, xeq(ms*ms,xVec3Length2(&ent->bound.mat->at),1e-3f));
            xMat3x3Normalize(&mn, ent->bound.mat);
            xMat4x3ToLocal(&lr.origin, ent->bound.mat, &ray->origin);
            xMat3x3Tolocal(&lr.dir, &mn, &ray->dir);
            lr.max_t = ray->max_t / ms,
            lr.min_t = ray->min_t / ms,
            lr.flags = ray->flags,
            tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius / ms,
            tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius / ms,
            tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius / ms,
            tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius / ms,
            tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius / ms,
            tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius / ms;
            result = xRayHitsBoxFast(&lr, &tmpbox);
        }
            break;
        case k_XBOUNDTYPE_CYLINDER:
            xFAIL_FMT_L(1830, "%s", "Unsupported bound type!!!");
            break;
        }

        if (result == FALSE) {
            return FALSE;
        }

        xModelInstance* collmod = ent->collModel ? ent->collModel : ent->model;

        if (collmod->Flags & 0x800) {
            xModelAnimCollApply(*collmod);
        }

        result = FALSE;
        if (xModelIsPreinstanced(collmod)) {
            xASSERT_DESIGN_L(1849, !xModelIsPreinstanced(collmod), "xSweptSphereToEnt: Entity is using pre-instanced model: %s", xSTAssetNameOrID(ent->id));
            return FALSE;
        }

        if (xSweptSphereToModel(sws, collmod->Data, collmod->Mat, ent->collType == 0x20)) {
            result = sphereToEntResults(sws, ent, react);
        }

        if (collmod->Flags & 0x800) {
            xModelAnimCollRestore(*collmod);
        }

        return result;
    } else {
        switch (ent->bound.type) {
        case k_XBOUNDTYPE_SPHERE:
            if (xSweptSphereToSphere(sws, &ent->bound.sph)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_BOX:
            if (xSweptSphereToBox(sws, &ent->bound.box.box)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_OBB:
            if (xSweptSphereToOBB(sws, &ent->bound.box.box, ent->bound.mat)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_CYLINDER:
            xFAIL_FMT_L(1884, "%s", "Unsupported bound type!!!");
            break;
        }
    }

    return FALSE;
}

S32 xNPCSweptSphereToEnt(xSweptSphere* sws, xEnt* ent, xRay3* ray)
{
    F32 testdist = sws->curdist;
    S32 goo = zGooIs(ent);

    if (sSweptSphereNPCIgnoreGoo && goo) {
        return FALSE;
    }

    if (!goo) {
        if ((ent->flags | (U8)sws->xEntFilterFlags) != ent->flags) {
            return FALSE;
        }
    }

    bool react = sSweptSphereCollType == 0x8 && zReactiveIsReactive(*ent);
    xSweptSphere sweptSphere;
    if (react) {
        sweptSphere = *sws;
        sws = &sweptSphere;
    }

    if (ent->collLev == 5) {
        U32 result = FALSE;

        switch (ent->bound.type) {
        case k_XBOUNDTYPE_SPHERE: {
            F32 oldrad = ent->bound.sph.r;
            ent->bound.sph.r += sws->radius;
            result = xRayHitsSphereFast(ray, &ent->bound.sph);
            ent->bound.sph.r = oldrad;
        }
            break;
        case k_XBOUNDTYPE_BOX: {
            xBox tmpbox;
            tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius,
            tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius,
            tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius,
            tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius,
            tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius,
            tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius;
            result = xRayHitsBoxFast(ray, &tmpbox);
        }
            break;
        case k_XBOUNDTYPE_OBB: {
            xBox tmpbox;
            xRay3 lr;
            xMat3x3 mn;
            F32 ms = xVec3Length(&ent->bound.mat->right);
            xVALIDATE_L(1958, xeq(ms*ms,xVec3Length2(&ent->bound.mat->up),1e-3f));
            xVALIDATE_L(1959, xeq(ms*ms,xVec3Length2(&ent->bound.mat->at),1e-3f));
            xMat3x3Normalize(&mn, ent->bound.mat);
            xMat4x3ToLocal(&lr.origin, ent->bound.mat, &ray->origin);
            xMat3x3Tolocal(&lr.dir, &mn, &ray->dir);
            lr.max_t = ray->max_t / ms,
            lr.min_t = ray->min_t / ms,
            lr.flags = ray->flags,
            tmpbox.upper.x = ent->bound.box.box.upper.x + sws->radius / ms,
            tmpbox.upper.y = ent->bound.box.box.upper.y + sws->radius / ms,
            tmpbox.upper.z = ent->bound.box.box.upper.z + sws->radius / ms,
            tmpbox.lower.x = ent->bound.box.box.lower.x - sws->radius / ms,
            tmpbox.lower.y = ent->bound.box.box.lower.y - sws->radius / ms,
            tmpbox.lower.z = ent->bound.box.box.lower.z - sws->radius / ms;
            result = xRayHitsBoxFast(&lr, &tmpbox);
        }
            break;
        case k_XBOUNDTYPE_CYLINDER:
            xFAIL_FMT_L(1977, "%s", "Unsupported bound type!!!");
            break;
        }

        if (result == FALSE) {
            return FALSE;
        }

        xModelInstance* collmod = ent->collModel ? ent->collModel : ent->model;

        if (collmod->Flags & 0x800) {
            xModelAnimCollApply(*collmod);
        }

        result = FALSE;
        if (xModelIsPreinstanced(collmod)) {
            xASSERT_DESIGN_L(1996, !xModelIsPreinstanced(collmod), "xSweptSphereToEnt: Entity is using pre-instanced model: %s", xSTAssetNameOrID(ent->id));
            return FALSE;
        }

        if (xSweptSphereToModel(sws, collmod->Data, collmod->Mat, ent->collType == 0x20)) {
            result = sphereToEntResults(sws, ent, react);
        }

        if (collmod->Flags & 0x800) {
            xModelAnimCollRestore(*collmod);
        }

        return result;
    } else {
        switch (ent->bound.type) {
        case k_XBOUNDTYPE_SPHERE:
            if (xSweptSphereToSphere(sws, &ent->bound.sph)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_BOX:
            if (xSweptSphereToBox(sws, &ent->bound.box.box)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_OBB:
            if (xSweptSphereToOBB(sws, &ent->bound.box.box, ent->bound.mat)) {
                return sphereToEntResults(sws, ent, react);
            }
            break;
        case k_XBOUNDTYPE_CYLINDER:
            xFAIL_FMT_L(2031, "%s", "Unsupported bound type!!!");
            break;
        }
    }

    return FALSE;
}

#define xSweptSphereInitRay(ray, sws)                                                             \
MACRO_START                                                                                       \
{                                                                                                 \
    xVec3Copy(&(ray).origin, &(sws)->start);                                                      \
    xVec3Sub(&(ray).dir, &(sws)->end, &(sws)->start);                                             \
    (ray).max_t = xVec3Length(&(ray).dir);                                                        \
    F32 one_len = 1.0f / MAX((ray).max_t, 0.00001f);                                              \
    xVec3SMul(&(ray).dir, &(ray).dir, one_len);                                                   \
    (ray).flags = XRAY3_USE_MAX;                                                                  \
}                                                                                                 \
MACRO_STOP

S32 xSweptSphereToScene(xSweptSphere* sws, xScene* sc, const xEnt* mover, U8 collType, S32 penby)
{
    if (sws->curdist < 0.0001f) {
        return FALSE;
    }

    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    S32 envcollfound = xSweptSphereToEnv(sws, sc->env);

    xRay3 ray;
    xSweptSphereInitRay(ray, sws);
    if (!(ray.flags & XRAY3_USE_MIN)) {
        ray.flags |= XRAY3_USE_MIN;
        ray.min_t = 0.0f;
    }

    xRayHitsGrid(&colls_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);
    xRayHitsGrid(&colls_oso_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);
    xRayHitsGrid(&npcs_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);

    return sSweptSphereEntFound || envcollfound;
}

S32 xSweptSphereToNonMoving(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby)
{
    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    S32 envcollfound = xSweptSphereToEnv(sws, sc->env);

    xVec3 diff;
    diff.Sub(sws->end, sws->start);
    if (diff.length() <= 0.00001f) {
        iprintf("The start and end positions of the swept sphere are the same. Returning no collision.");
        return envcollfound;
    }

    xRay3 ray = { 0 };
    xSweptSphereInitRay(ray, sws);
    if (!(ray.flags & XRAY3_USE_MIN)) {
        ray.flags |= XRAY3_USE_MIN;
        ray.min_t = 0.0f;
    }

    sSweptSphereIgnoreMovers = TRUE;
    xRayHitsGrid(&colls_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);
    xRayHitsGrid(&colls_oso_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);
    sSweptSphereIgnoreMovers = FALSE;

    return sSweptSphereEntFound || envcollfound;
}

S32 xSweptSphereToStatDyn(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby)
{
    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    if (!(sws->start.x - sws->end.x) && !(sws->start.y - sws->end.y) && !(sws->start.z - sws->end.z)) {
        return sSweptSphereEntFound;
    }

    xRay3 ray;
    xSweptSphereInitRay(ray, sws);
    if (!(ray.flags & XRAY3_USE_MIN)) {
        ray.flags |= XRAY3_USE_MIN;
        ray.min_t = 0.0f;
    }

    xRayHitsGrid(&colls_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);
    xRayHitsGrid(&colls_oso_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);

    return sSweptSphereEntFound;
}

S32 xNPCSweptSphereToStatDyn(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby, bool ignoreGoo)
{
    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSphereNPCIgnoreGoo = ignoreGoo;
    sSweptSpherePenby = penby;

    if (!(sws->start.x - sws->end.x) && !(sws->start.y - sws->end.y) && !(sws->start.z - sws->end.z)) {
        return sSweptSphereEntFound;
    }

    xRay3 ray;
    xSweptSphereInitRay(ray, sws);
    if (!(ray.flags & XRAY3_USE_MIN)) {
        ray.flags |= XRAY3_USE_MIN;
        ray.min_t = 0.0f;
    }

    xRayHitsGrid(&colls_grid, sc, &ray, NPCSweptSphereHitsEntCB, &sws->qcd, sws);
    xRayHitsGrid(&colls_oso_grid, sc, &ray, NPCSweptSphereHitsEntCB, &sws->qcd, sws);

    return sSweptSphereEntFound;
}

S32 xSweptSphereToPlayer(xSweptSphere* sws, xScene*, xEnt* mover, U8 collType, S32 penby)
{
    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        if (CURPLAYERS[i] == mover) {
            continue;
        }

        xSphere* extra_spheres[10];
        S32 numSpheres = CURPLAYERS[i]->GetCollisionSpheres((const xSphere**)extra_spheres);

        xASSERT_L(2208, CURPLAYERS[i]->numExtraSpheres + 1 < 10);

        for (S32 j = 0; j < numSpheres; j++) {
            if (xSweptSphereToSphere(sws, extra_spheres[j])) {
                sws->mptr = CURPLAYERS[i]->model;
                sws->oid = CURPLAYERS[i]->id;
                sws->optr = CURPLAYERS[i];
                sSweptSphereEntFound = TRUE;
            }
        }
    }

    return sSweptSphereEntFound;
}

S32 xSweptSphereToPlayer(xSweptSphere* sws, xScene*, xEnt* mover, zPlayer** player, U8 collType, S32 penby)
{
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    for (S32 i = 0; i < CURPLAYERS.size(); i++) {
        if (CURPLAYERS[i] == mover) {
            continue;
        }

        xSphere* extra_spheres[10];
        S32 numSpheres = CURPLAYERS[i]->GetCollisionSpheres((const xSphere**)extra_spheres);

        xASSERT_L(2240, CURPLAYERS[i]->numExtraSpheres + 1 < 10);

        for (S32 j = 0; j < numSpheres; j++) {
            if (xSweptSphereToSphere(sws, extra_spheres[j])) {
                sws->mptr = CURPLAYERS[i]->model;
                sws->oid = CURPLAYERS[i]->id;
                sws->optr = CURPLAYERS[i];
                *player = CURPLAYERS[i];
                return TRUE;
            }
        }
    }

    return FALSE;
}

S32 xSweptSphereToNPC(xSweptSphere* sws, xScene* sc, xEnt* mover, U8 collType, S32 penby)
{
    sSweptSphereEntFound = FALSE;
    sSweptSphereMover = mover;
    sSweptSphereCollType = collType;
    sSweptSpherePenby = penby;

    if (!(sws->start.x - sws->end.x) && !(sws->start.y - sws->end.y) && !(sws->start.z - sws->end.z)) {
        return sSweptSphereEntFound;
    }

    xRay3 ray;
    xSweptSphereInitRay(ray, sws);
    if (!(ray.flags & XRAY3_USE_MIN)) {
        ray.flags |= XRAY3_USE_MIN;
        ray.min_t = 0.0f;
    }

    xRayHitsGrid(&npcs_grid, sc, &ray, SweptSphereHitsEntCB, &sws->qcd, sws);

    return sSweptSphereEntFound;
}
