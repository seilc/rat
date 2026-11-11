#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xModel.h"

struct xEnt;
struct xEnv;
struct xJSPHeader;
struct xQCData;
struct xScene;

struct xCollis
{
    struct tri_data
    {
        U32 index;
        F32 r;
        F32 d;
    };

    U32 flags;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    F32 dist;
    F32 test_dist;
    xVec3 norm;
    xVec3 tohit;
    xVec3 depen;
    xVec3 hdng;
    union
    {
        struct
        {
            F32 t;
            F32 u;
            F32 v;
        } tuv;
        tri_data tri;
    };
};

struct xParabola
{
    xVec3 initPos;
    xVec3 initVel;
    F32 gravity;
    F32 minTime;
    F32 maxTime;
};

inline void xParabolaEvalPos(const xParabola* p, xVec3* pos, F32 time)
{
    xVec3Copy(pos, &p->initPos);
    pos->AddScale(p->initVel, time);
    pos->y -= 0.5f * p->gravity * time * time;
}

inline void xParabolaEvalVel(const xParabola* p, xVec3* vel, F32 time)
{
    xVec3Copy(vel, &p->initVel);
    vel->y -= p->gravity * time;
}

typedef enum _xCollsIdx
{
    k_XCOLLS_IDX_FLOOR,
    k_XCOLLS_IDX_CEIL,
    k_XCOLLS_IDX_FRONT,
    k_XCOLLS_IDX_LEFT,
    k_XCOLLS_IDX_REAR,
    k_XCOLLS_IDX_RIGHT,
    k_XCOLLS_IDX_COUNT
} xCollsIdx;

#define k_HIT_IT ((U32)(1 << 0))
#define k_HIT_CALC_HDNG ((U32)(1 << 12))
#define k_HIT_CALC_TRI ((U32)(1 << 13))

xCollsIdx xCollideGetCollsIdx(const xCollis* coll, const xVec3* tohit, const xMat3x3* mat);
xCollsIdx xCollideGetCollsIdxWithNormal(const xCollis* coll, const xVec3* tohit, const xVec3* normal, const xMat3x3* mat); // UNUSED
U32 xSphereHitsSphere(const xSphere* a, const xSphere* b, xCollis* coll);
U32 xSphereHitsBox(const xSphere* a, const xBox* b, xCollis* coll);
U32 xSphereHitsOBB(const xSphere* a, const xBox* b, const xMat4x3* m, xCollis* coll); // UNUSED
U32 xSphereHitsOBB_nu(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
U32 xSphereHitsOBB_nu(const xSphere* sph, const xBox* box, const xMat4x3* mat, const xMat3x3* nmat, const xVec3* scale, xCollis* coll); // UNUSED
U32 xSphereHitsModel(const xSphere* b, const xModelInstance* m, xCollis* coll);
U32 xSphereHitsEntModel(const xSphere* b, const xEnt* ent, xCollis* coll);
S32 xSphereHitsEntModel3(const xSphere* b, const xEnt* ent, xCollis* colls, U8 ncolls, F32 sth); // UNUSED
void xParabolaRecenter(xParabola* p, F32 newZeroT);
S32 xParabolaHitsEnv(xParabola* p, const xEnv* env, xCollis* colls);
S32 xCapsuleIsectsModelClosestToLine(const xCapsule* c, const xModelInstance* model, xCollis* colls);
S32 xCapsuleIsectsModelClosestToStart(const xCapsule* c, const xModelInstance* model, xCollis* colls); // UNUSED
S32 xCapsuleIsectsModelAll(const xCapsule* c, const xModelInstance* model, xCollis* colls, S32); // UNUSED
S32 xCapsuleIsectsModelAllOrdered(const xCapsule* c, const xModelInstance* model, xCollis* colls, S32); // UNUSED
S32 xCapsuleIsectsEnvClosestToLine(const xCapsule* c, const xEnv* env, xCollis* colls);
S32 xCapsuleIsectsEnvClosestToStart(const xCapsule* c, const xEnv* env, xCollis* colls); // UNUSED
void xCapsuleHitsEnt(xScene*, xRay3*, xQCData*, xEnt* ent, void* data); // UNUSED
S32 xCapsuleIsectsSceneClosestToLine(const xCapsule* c, xScene* sc, xCollis* colls); // UNUSED
void xSphereForSpheres(xSphere* o, const xSphere* a, const xSphere* b); // UNUSED
U32 xBoxHitsSphere(const xBox* a, const xSphere* b, xCollis* coll);
U32 xObbHitsObb(const xBox* a, const xMat4x3* a_mat, const xBox* b, const xMat4x3* b_mat, xCollis* coll);
U32 xBoxHitsObb(const xBox* a, const xBox* b, const xMat4x3* mat, xCollis* coll);
void xCollisTranslate(xCollis* c, const xVec3* v, F32 r); // UNUSED
void xCollisTranslate(xCollis* c, F32 x, F32 y, F32 z, F32 r); // UNUSED
void xCollideCalcTri(xCollis::tri_data& tri, const xModelInstance& model, const xVec3& center, const xVec3& heading);
xVec3 xCollisTriHit(const xCollis::tri_data& tri, const xModelInstance& model);
U8 xSphereHitsCapsule(const xVec3& center, F32 radius, const xVec3& v1, const xVec3& v2, F32 width); // UNUSED
bool xSphereHitsBox(const xVec3& c, F32 r, const xBox& b);
bool xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat);
bool xSphereHitsVCylinder(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr, F32 ch);
U32 xSphereHitsVCylinder(const xSphere* sph, const xCylinder* cyl, xCollis* coll);
U32 xCylinderHitsSphere(const xCylinder* cyl, const xSphere* sph, xCollis* coll);
bool xSphereHitsVCircle(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr);
U8 xSphereHitsCone(const xVec3& scenter, F32 sradius, const xVec3& kvert, const xVec3& kaxis, F32 krsin, F32 kcos2, F32 ksin2); // UNUSED
bool xSphereHitsCone(const xVec3& center, F32 radius, const xVec3& kv0, F32 kr0, const xVec3& kv1, F32 kr1);
void xBoxForTriangle(xBox& box, const xVec3& p0, const xVec3& p1, const xVec3& p2);
void xBoxForModel(xBox& o, const xModelInstance* m, bool childModels);
void xBoxForIModelLocal(xBox& o, const RpAtomic* imodel, bool initBox);
void xBoxForModelLocal(xBox& o, const xModelInstance* m, bool childModels);
void xSphereForModel(xSphere& o, const xModelInstance* model, bool childModels);
U32 xRayHitsModelSingle(const xRay3* r, const xModelInstance* m, xCollis* coll);
U32 xRayHitsModel(const xRay3* r, const xModelInstance* m, xCollis* coll);
U32 xRayHitsJSP(const xRay3* r, const xJSPHeader* jsp, xCollis* coll);
U32 xRayHitsEnv(const xRay3* r, const xEnv* env, xCollis* coll);
S32 xSphereHitsModel3(const xSphere* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth);
S32 xSphereHitsEnv4(const xSphere* b, const xEnv* env, const xMat3x3* mat, xCollis* colls); // UNUSED
S32 xSphereHitsEnv3(const xSphere* b, const xEnv* env, xCollis* colls, U8 ncolls, F32 sth);
U32 xSphereHitsEnv(const xSphere* b, const xEnv* env, xCollis* coll);
bool xOBBHitsEnt(const xBox& box, const xMat4x3& mat, const xEnt* ent, const xQCData* quickCullData);
bool xOBBHitsScene(const xBox& box, const xMat4x3& mat, xScene* sc, const xEnt* object, U8 collType, U8 chk);
bool xOBBHitsEnv(const xBox& box, const xMat4x3& mat, const xEnv* env);
bool xOBBHitsModel(const xBox& box, const xMat4x3& mat, const xModelInstance* model);

inline bool xSphereHitsSphere(const xVec3& loc1, F32 r1, const xVec3& loc2, F32 r2)
{
    F32 dist2 = loc2.Distance2(loc1);
    F32 max_dist = r1 + r2;
    return dist2 <= max_dist * max_dist;
}

inline bool xSphereHitsSphere(const xSphere& o1, const xSphere& o2)
{
    return xSphereHitsSphere(o1.center, o1.r, o2.center, o2.r);
}

inline bool xSphereHitsBox(const xSphere& o, const xBox& b)
{
    return xSphereHitsBox(o.center, o.r, b);
}

inline bool xSphereHitsOBB(const xVec3& c, F32 r, const xBox& b, const xMat4x3& mat)
{
    xVec3 lc;
    xMat4x3ToLocal(&lc, &mat, &c);
    return xSphereHitsBox(lc, r, b);
}

inline bool xSphereHitsOBB(const xSphere& o, const xBox& b, const xMat4x3& mat)
{
    return xSphereHitsOBB(o.center, o.r, b, mat);
}

// UNUSED: speculative
inline bool xVecHitsSphere(const xVec3& v, const xVec3& c, F32 r)
{
    F32 dx = v.x - c.x;
    F32 dy = v.y - c.y;
    F32 dz = v.z - c.z;
    return dx * dx + dy * dy + dz * dz <= r * r;
}

// UNUSED: speculative
inline bool xVecHitsSphere(const xVec3& v, const xSphere& o)
{
    return xVecHitsSphere(v, o.center, o.r);
}

// UNUSED: speculative
inline bool xVecHitsBox(const xVec3& v, const xBox& b)
{
    return v.x >= b.lower.x && v.y >= b.lower.y && v.z >= b.lower.z &&
           v.x <= b.upper.x && v.y <= b.upper.y && v.z <= b.upper.z;
}

// UNUSED: speculative
inline bool xVecHitsOBB(const xVec3& v, const xBox& b, const xMat4x3& mat)
{
    xVec3 loc;
    xMat4x3ToLocal(&loc, &mat, &v);
    return xVecHitsBox(loc, b);
}

inline bool xBoxHitsBox(const xBox& a, const xBox& b)
{
    return a.upper.x >= b.lower.x && a.upper.y >= b.lower.y && a.upper.z >= b.lower.z &&
           a.lower.x <= b.upper.x && a.lower.y <= b.upper.y && a.lower.z <= b.upper.z;
}

#endif
