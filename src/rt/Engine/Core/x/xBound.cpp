#include "xBound.h"

#include "xCollide.h"
#include "xDebug.h"
#include "xDraw.h"
#include "xMath3.h"
#include "xModel.h"
#include "xQuickCull.h"

extern S32 xent_entent;
#if DEBUG || RELEASE
extern S32 xent_nqcrejects;
#endif

// RESTORED: speculative
void xBoundMakeSphere(xBound& b, const xVec3& center, F32 radius)
{
    b.type = k_XBOUNDTYPE_SPHERE;
    b.sph.center = center;
    b.sph.r = radius;
    xQuickCullForSphere(&b.qcd, &b.sph);
}
// END RESTORED

void xBoundUpdate(xBound* b)
{
    if (b->type == k_XBOUNDTYPE_BOX) {
        xVec3Add(&b->box.center, &b->box.box.lower, &b->box.box.upper); xVec3SMul(&b->box.center, &b->box.center, 0.5f);
    }
    else if (b->type == k_XBOUNDTYPE_OBB) {
        xVec3Add(&b->box.center, &b->box.box.lower, &b->box.box.upper); xVec3SMul(&b->box.center, &b->box.center, 0.5f);
        xMat4x3ToWorld(&b->box.center, b->mat, &b->box.center);
    }

    if (b->type != k_XBOUNDTYPE_NONE) {
        xQuickCullForBound(&b->qcd, b);
#if DEBUG
        xQuickCullDebugRegister(&b->qcd);
#endif
    }
}

void xBoundGetBox(xBox& box, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE: {
        const xSphere& o = bound.sph;
        box.upper.assign(o.center.x + o.r, o.center.y + o.r, o.center.z + o.r);
        box.lower.assign(o.center.x - o.r, o.center.y - o.r, o.center.z - o.r);
    }
        break;
    case k_XBOUNDTYPE_BOX:
        box = bound.box.box;
        break;
    case k_XBOUNDTYPE_OBB:
        xBoxInitBoundOBB(&box, &bound.box.box, bound.mat);
        break;
    case k_XBOUNDTYPE_CYLINDER: {
        const xCylinder& c = bound.cyl;
        box.upper.assign(c.center.x + c.r, c.center.y + c.h, c.center.z + c.r);
        box.lower.assign(c.center.x - c.r, c.center.y - c.h, c.center.z - c.r);
    }
        break;
    default:
        xFAIL_FMT_L(81, "%s", "Unsupported bound type.");
    }
}

void xBoundGetCylinder(xCylinder& o, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE:
        o.center = bound.sph.center;
        o.h = bound.sph.r;
        o.r = bound.sph.r;
        break;
    case k_XBOUNDTYPE_BOX: {
        xVec3 diff;
        o.center = bound.box.center;
        diff.Sub(bound.box.box.upper, bound.box.center);
        o.h = diff.y;
        diff.y = 0.0f;
        o.r = diff.length();
        break;
    }
    case k_XBOUNDTYPE_OBB: {
        const xMat4x3& mat = *bound.mat;
        xVec3 v; v.ScaleDiff(bound.box.box.upper, bound.box.box.lower, 0.5f);
        F32 r2 = v.x * v.x * mat.right.length2() + v.z * v.z * mat.at.length2();
        o.r = xsqrt(r2);
        o.center = mat.pos;
        o.center.y += bound.box.box.upper.y - bound.box.box.lower.y;
        o.h = v.y;
    }
        break;
    case k_XBOUNDTYPE_CYLINDER:
        o = bound.cyl;
    default:
        xFAIL_FMT_L(120, "%s", "Unsupported bound type.");
    }
}

void xBoundGetSphere(xSphere& o, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE:
        o = bound.sph;
        break;
    case k_XBOUNDTYPE_BOX:
        o.center = bound.box.center;
        o.r = bound.box.box.upper.Distance(bound.box.center);
        break;
    case k_XBOUNDTYPE_OBB: {
        const xMat4x3& mat = *bound.mat;
        xVec3 v; v.ScaleDiff(bound.box.box.upper, bound.box.box.lower, 0.5f);
        F32 r2 = v.x * v.x * mat.right.length2() + v.y * v.y * mat.up.length2() + v.z * v.z * mat.at.length2();
        o.r = xsqrt(r2);
        o.center = bound.box.center;
        break;
    }
    default:
        xFAIL_FMT_L(149, "%s", "Unsupported bound type.");
    }
}

// RESTORED: speculative
void xBoundGetSphere2(xSphere& o, const xBound& bound)
{
    switch (bound.type) {
    case k_XBOUNDTYPE_SPHERE:
        o = bound.sph;
        o.r = o.r * o.r;
        break;
    case k_XBOUNDTYPE_BOX:
        o.center = bound.box.center;
        o.r = bound.box.box.upper.Distance2(bound.box.center);
        break;
    case k_XBOUNDTYPE_OBB: {
        const xMat4x3& mat = *bound.mat;
        xVec3 v; v.ScaleDiff(bound.box.box.upper, bound.box.box.lower, 0.5f);
        o.r = v.x * v.x * mat.right.length2() + v.y * v.y * mat.up.length2() + v.z * v.z * mat.at.length2();
        o.center = bound.box.center;
        break;
    }
    default:
        xFAIL_FMT("%s", "Unsupported bound type.");
    }
}
// END RESTORED

void xBoundSphereHitsOBB(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll)
{
    xSphereHitsOBB_nu(s, b, m, coll);
}

void xBoundHitsBound(const xBound* a, const xBound* b, xCollis* c)
{
    xASSERT_L(241, a);
    xASSERT_L(242, b);
    xASSERT_L(243, c);

    if (!xQuickCullIsects(&a->qcd, &b->qcd)) {
        c->flags &= ~k_HIT_IT;
#if DEBUG || RELEASE
        if (xent_entent) {
            xent_nqcrejects++;
        }
#endif
        return;
    }

    switch (a->type | (b->type << 3)) {
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_SPHERE << 3):
        xSphereHitsSphere(&a->sph, &b->sph, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_OBB << 3):
        xBoundSphereHitsOBB(&a->sph, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_BOX << 3):
        xSphereHitsBox(&a->sph, &b->box.box, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_SPHERE << 3):
        xBoxHitsSphere(&a->box.box, &b->sph, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_OBB << 3):
        xBoxHitsObb(&a->box.box, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_SPHERE << 3):
        xBoundSphereHitsOBB(&b->sph, &a->box.box, a->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_OBB << 3):
        xObbHitsObb(&a->box.box, a->mat, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_BOX << 3):
        xBoxHitsObb(&b->box.box, &a->box.box, a->mat, c);
        break;
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_CYLINDER << 3):
        xSphereHitsVCylinder(&a->sph, &b->cyl, c);
        break;
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_BOX << 3):
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_CYLINDER << 3):
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_CYLINDER << 3):
    case k_XBOUNDTYPE_CYLINDER | (k_XBOUNDTYPE_SPHERE << 3):
        xCylinderHitsSphere(&a->cyl, &b->sph, c);
        break;
    default:
        xDEBUG("xBoundHitsBound: unsupported collision type - bound %d vs [ANY]\n", a->type);
    }
}

void xBoundHitsModel(const xBound* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth)
{
    xASSERT_L(319, b);
    xASSERT_L(320, m);
    xASSERT_L(321, colls);

    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        xASSERT_L(326, !xModelIsPreinstanced(m));
        if (!xModelIsPreinstanced(m))
            xSphereHitsModel3(&b->sph, m, colls, ncolls, sth);
        break;
    case k_XBOUNDTYPE_BOX:
    case k_XBOUNDTYPE_OBB:
    case k_XBOUNDTYPE_CYLINDER:
    default:
        xDEBUG("xBoundHitsModel: unsupported collision type - bound %d vs [ANY]\n", b->type);
    }
}

static void xBoundOBBIsectRay(const xBox* b, const xMat4x3* m, const xRay3* r, xIsect* isect)
{
    xASSERT_L(345, b);
    xASSERT_L(346, m);
    xASSERT_L(347, r);
    xASSERT_L(348, isect);

    xRay3 xfr;
    xBox sbox;
    xVec3 scale;
    xMat4x3 mnormal;

    xVec3NormalizeMacro(&mnormal.right, &m->right, &scale.x);
    xVec3NormalizeMacro(&mnormal.up, &m->up, &scale.y);
    xVec3NormalizeMacro(&mnormal.at, &m->at, &scale.z);
    mnormal.pos = m->pos;

    sbox.upper.x = b->upper.x * scale.x, sbox.upper.y = b->upper.y * scale.y, sbox.upper.z = b->upper.z * scale.z;
    sbox.lower.x = b->lower.x * scale.x, sbox.lower.y = b->lower.y * scale.y, sbox.lower.z = b->lower.z * scale.z;

    xMat4x3ToLocal(&xfr.origin, &mnormal, &r->origin);
    xMat3x3Tolocal(&xfr.dir, &mnormal, &r->dir);
    xfr.min_t = r->min_t;
    xfr.max_t = r->max_t;
    xfr.flags = r->flags;

    iBoxIsectRay(&sbox, &xfr, isect);
}

void xRayHitsBound(const xRay3* r, const xBound* b, xCollis* c)
{
    xASSERT_L(409, r);
    xASSERT_L(410, b);
    xASSERT_L(411, c);

    xIsect isect = { 0 };

    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        iSphereIsectRay(&b->sph, r, &isect);
        break;
    case k_XBOUNDTYPE_OBB:
        xBoundOBBIsectRay(&b->box.box, b->mat, r, &isect);
        break;
    case k_XBOUNDTYPE_BOX:
        iBoxIsectRay(&b->box.box, r, &isect);
        break;
    case k_XBOUNDTYPE_CYLINDER:
        break;
    default:
        xDEBUG("xRayHitsBound: unsupported bound type %d\n", b->type);
    }

    if (isect.penned <= 0.0f) {
        c->flags |= k_HIT_IT;
        if (isect.contained <= 0.0f) {
            c->flags |= 0x10;
        }
        c->dist = isect.dist;
    } else {
        c->flags &= ~k_HIT_IT;
    }
}

void xSphereHitsBound(const xSphere* o, const xBound* b, xCollis* c)
{
    xASSERT_L(468, o != 0); xASSERT_L(468, b != 0); xASSERT_L(468, c != 0);
    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        xSphereHitsSphere(o, &b->sph, c);
        break;
    case k_XBOUNDTYPE_OBB:
        xBoundSphereHitsOBB(o, &b->box.box, b->mat, c);
        break;
    case k_XBOUNDTYPE_BOX:
        xSphereHitsBox(o, &b->box.box, c);
        break;
    default:
        xDEBUG("xBoundHitsBound: unsupported collision type - sphere bound vs %d\n", b->type);
    }
}

// RESTORED: speculative
void xSphereHitsBound(const xSphere* o, const xQCData* q, const xBound* b, xCollis* c)
{
    xASSERT(q != 0);
    xASSERT(b != 0);
    xASSERT(c != 0);

    if (xQuickCullIsects(q, &b->qcd)) {
        xSphereHitsBound(o, b, c);
    } else {
        c->flags &= ~k_HIT_IT;
    }
}
// END RESTORED

void xVecHitsBound(const xVec3* v, const xBound* b, xCollis* c)
{
    xASSERT_L(500, v);
    xASSERT_L(501, b);
    xASSERT_L(502, c);

    xIsect isect = { 0 };

    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        iSphereIsectVec(&b->sph, v, &isect);
        break;
    case k_XBOUNDTYPE_OBB: {
        xVec3 lv;
        xMat4x3ToLocal(&lv, b->mat, v);
        iBoxIsectVec(&b->box.box, &lv, &isect);
        break;
    }
    case k_XBOUNDTYPE_BOX:
        iBoxIsectVec(&b->box.box, v, &isect);
        break;
    case k_XBOUNDTYPE_CYLINDER:
        break;
    default:
        xDEBUG("xVecHitsBound: unsupported bound type %d\n", b->type);
    }

    if (isect.penned <= 0.0f) {
        c->flags |= k_HIT_IT;
    } else {
        c->flags &= ~k_HIT_IT;
    }
}

void xBoundDraw(const xBound* b)
{
    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        xDrawSphere2(&b->sph, 12);
        break;
    case k_XBOUNDTYPE_OBB:
        xDrawOBB(&b->box.box, b->mat);
        break;
    case k_XBOUNDTYPE_BOX:
        xDrawBox(&b->box.box);
        break;
    case k_XBOUNDTYPE_CYLINDER:
        break;
    default:
        xDEBUG("xBoundDraw: unsupported bound type %d\n", b->type);
    }
}

bool xBoundHitsBound(const xBound& a, const xBound& b)
{
    switch (a.type | (b.type << 3)) {
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsSphere(a.sph, b.sph);
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_OBB << 3):
        return xSphereHitsOBB(a.sph, b.box.box, *b.mat);
    case k_XBOUNDTYPE_SPHERE | (k_XBOUNDTYPE_BOX << 3):
        return xSphereHitsBox(a.sph, b.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsBox(b.sph, a.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_BOX << 3):
        return xBoxHitsBox(a.box.box, b.box.box);
    case k_XBOUNDTYPE_BOX | (k_XBOUNDTYPE_OBB << 3):
        return xOBBHitsOBB(a.box.box, g_I3, b.box.box, *b.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_SPHERE << 3):
        return xSphereHitsOBB(b.sph, a.box.box, *a.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_OBB << 3):
        return xOBBHitsOBB(a.box.box, *a.mat, b.box.box, *b.mat);
    case k_XBOUNDTYPE_OBB | (k_XBOUNDTYPE_BOX << 3):
        return xOBBHitsOBB(a.box.box, *a.mat, b.box.box, g_I3);
    default:
        xDEBUG("xBoundHitsBound: unsupported collision type - bound %d vs bound %d\n", a.type, b.type);
        return false;
    }
}

// RESTORED: from Incredibles GC
bool xRayHitsBound(const xRay3& r, const xBound& b)
{
    xIsect isect;
    isect.penned = HUGE;

    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        iSphereIsectRay(&b.sph, &r, &isect);
        break;
    case k_XBOUNDTYPE_BOX:
        iBoxIsectRay(&b.box.box, &r, &isect);
        break;
    case k_XBOUNDTYPE_OBB:
        xBoundOBBIsectRay(&b.box.box, b.mat, &r, &isect);
        break;
    case k_XBOUNDTYPE_CYLINDER:
        break;
    default:
        xDEBUG("xRayHitsBound: unsupported bound type %d\n", b.type);
    }

    return isect.penned <= 0.0f;
}
// END RESTORED

bool xSphereHitsBound(const xVec3& c, F32 r, const xBound& b)
{
    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        return xSphereHitsSphere(c, r, b.sph.center, b.sph.r);
    case k_XBOUNDTYPE_BOX:
        return xSphereHitsBox(c, r, b.box.box);
    case k_XBOUNDTYPE_OBB:
        return xSphereHitsOBB(c, r, b.box.box, *b.mat);
    default:
        xDEBUG("xSphereHitsBound: unsupported collision type - sphere bound vs %d\n", b.type);
        return false;
    }
}

// RESTORED: speculative
bool xVecHitsBound(const xVec3& v, const xBound& b)
{
    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        return xVecHitsSphere(v, b.sph);
    case k_XBOUNDTYPE_BOX:
        return xVecHitsBox(v, b.box.box);
    case k_XBOUNDTYPE_OBB:
        return xVecHitsOBB(v, b.box.box, *b.mat);
    default:
        xDEBUG("xVecHitsBound: unsupported collision type - sphere bound vs %d\n", b.type);
        return false;
    }
}
// END RESTORED

bool xOBBHitsBound(const xBox& box, const xMat4x3& mat, const xBound& b)
{
    switch (b.type) {
    case k_XBOUNDTYPE_SPHERE:
        return xSphereHitsOBB(b.sph.center, b.sph.r, box, mat);
    case k_XBOUNDTYPE_BOX:
        return xOBBHitsOBB(b.box.box, g_I3, box, mat);
    case k_XBOUNDTYPE_OBB:
        return xOBBHitsOBB(b.box.box, *b.mat, box, mat);
    default:
        return false;
    }
}

void xBoundGetCenter(const xBound* b, xVec3* center)
{
    switch (b->type) {
    case k_XBOUNDTYPE_SPHERE:
        *center = b->sph.center;
        break;
    case k_XBOUNDTYPE_BOX:
    case k_XBOUNDTYPE_OBB:
        *center = b->box.center;
        break;
    case k_XBOUNDTYPE_CYLINDER:
        *center = b->cyl.center;
        break;
    default:
        xFAIL_FMT_L(697, "%s", "Unsupported type");
    }
}

bool xOBBHitsTriangle(const xBox& box, const xMat4x3& mat, const xVec3* vertices)
{
    xVec3 dist;
    dist.Sub(box.upper, box.lower);
    dist.Scale(0.5f);

    xVec3 center = mat.pos;
    center.Add(box.lower);
    center.Add(dist);

    xVec3 v2MinusV1, v1MinusV0, v0MinusV2;
    v2MinusV1.Sub(vertices[2], vertices[1]);
    v1MinusV0.Sub(vertices[1], vertices[0]);
    v0MinusV2.Sub(vertices[0], vertices[2]);

    xVec3 seperationAxes[13];
    seperationAxes[0].cross(v2MinusV1, v1MinusV0);
    seperationAxes[1] = mat.right;
    seperationAxes[2] = mat.up;
    seperationAxes[3] = mat.at;
    seperationAxes[4].cross(mat.right, v1MinusV0);
    seperationAxes[5].cross(mat.right, v2MinusV1);
    seperationAxes[6].cross(mat.right, v0MinusV2);
    seperationAxes[7].cross(mat.up, v1MinusV0);
    seperationAxes[8].cross(mat.up, v2MinusV1);
    seperationAxes[9].cross(mat.up, v0MinusV2);
    seperationAxes[10].cross(mat.at, v1MinusV0);
    seperationAxes[11].cross(mat.at, v2MinusV1);
    seperationAxes[12].cross(mat.at, v0MinusV2);

    for (S32 i = 0; i < 13; i++) {
        xVec3& s = seperationAxes[i];

        F32 boxDist = dist.x * xabs(mat.right.dot(s)) + dist.y * xabs(mat.up.dot(s)) + dist.z * xabs(mat.at.dot(s));
        F32 centerDotS = center.dot(s);
        F32 minBoxPos = centerDotS - boxDist;
        F32 maxBoxPos = centerDotS + boxDist;

        F32 v0DotS = vertices[0].dot(s);
        F32 v1DotS = vertices[1].dot(s);
        F32 v2DotS = vertices[2].dot(s);
        F32 minTriPos = xmin(xmin(v0DotS, v1DotS), v2DotS);
        F32 maxTriPos = xmax(xmax(v0DotS, v1DotS), v2DotS);

        if (minBoxPos > maxTriPos || minTriPos > maxBoxPos) {
            return false;
        }
    }

    return true;
}
