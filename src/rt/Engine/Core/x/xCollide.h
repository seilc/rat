#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xModel.h"

struct xCollis
{
    struct tri_data
    {
        U32 index;
        F32 r;
        F32 d;

        // Not present in DWARF, so probably a compiler-generated function, but somehow also appears in its own .text section?
        // Needed to match xEntDrive.cpp.
        tri_data& operator=(const tri_data& tri)
        {
            index = tri.index;
            r = tri.r;
            d = tri.d;
            return *this;
        }
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

#define k_HIT_IT ((U32)(1 << 0))
#define k_HIT_CALC_HDNG ((U32)(1 << 12))
#define k_HIT_CALC_TRI ((U32)(1 << 13))

U32 xSphereHitsSphere(const xSphere* a, const xSphere* b, xCollis* coll);
U32 xSphereHitsBox(const xSphere* a, const xBox* b, xCollis* coll);
U32 xSphereHitsOBB_nu(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
U32 xBoxHitsSphere(const xBox* a, const xSphere* b, xCollis* coll);
U32 xObbHitsObb(const xBox* a, const xMat4x3* a_mat, const xBox* b, const xMat4x3* b_mat, xCollis* coll);
U32 xBoxHitsObb(const xBox* a, const xBox* b, const xMat4x3* mat, xCollis* coll);
xVec3 xCollisTriHit(const xCollis::tri_data& tri, const xModelInstance& model);
bool xSphereHitsBox(const xVec3& c, F32 r, const xBox& b);
bool xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat);
U32 xSphereHitsVCylinder(const xSphere* sph, const xCylinder* cyl, xCollis* coll);
U32 xCylinderHitsSphere(const xCylinder* cyl, const xSphere* sph, xCollis* coll);
S32 xSphereHitsModel3(const xSphere* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth);

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

// RESTORED: speculative
inline bool xVecHitsSphere(const xVec3& v, const xVec3& c, F32 r)
{
    F32 dx = v.x - c.x;
    F32 dy = v.y - c.y;
    F32 dz = v.z - c.z;
    return dx * dx + dy * dy + dz * dz <= r * r;
}

inline bool xVecHitsSphere(const xVec3& v, const xSphere& o)
{
    return xVecHitsSphere(v, o.center, o.r);
}

inline bool xVecHitsBox(const xVec3& v, const xBox& b)
{
    return v.x >= b.lower.x && v.y >= b.lower.y && v.z >= b.lower.z &&
           v.x <= b.upper.x && v.y <= b.upper.y && v.z <= b.upper.z;
}

inline bool xVecHitsOBB(const xVec3& v, const xBox& b, const xMat4x3& mat)
{
    xVec3 loc;
    xMat4x3ToLocal(&loc, &mat, &v);
    return xVecHitsBox(loc, b);
}
// END RESTORED

inline bool xBoxHitsBox(const xBox& a, const xBox& b)
{
    return a.upper.x >= b.lower.x && a.upper.y >= b.lower.y && a.upper.z >= b.lower.z &&
           a.lower.x <= b.upper.x && a.lower.y <= b.upper.y && a.lower.z <= b.upper.z;
}

#endif
