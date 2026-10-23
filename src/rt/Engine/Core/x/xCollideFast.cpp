#include "xCollideFast.h"

#include "xDebug.h"
#include "xBound.h"

U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s)
{
    xASSERT(26, r);
    xASSERT(27, s);

    xVec3 diff;
    xVec3Sub(&diff, &r->origin, &s->center);
    
#if DEBUG
    F32 a = xVec3Dot(&r->dir, &r->dir);
    xVALIDATE(33, xabs(a-1) < 1e-3f);
#endif
    
    F32 c = xVec3Dot(&diff, &diff) - s->r * s->r;
    if (c <= 0.0f) {
        return TRUE;
    }

    if (r->flags & XRAY3_USE_MAX) {
        if (c > (2.0f * s->r + r->max_t) * r->max_t) {
            return FALSE;
        }
    }

    F32 b = xVec3Dot(&diff, &r->dir);
    if (b >= 0.0f) {
        return FALSE;
    }

    return (b * b >= c);
}

U32 xRayHitsBoxFast(const xRay3* r, const xBox* b)
{
    xASSERT(63, r);
    xASSERT(64, b);

    xIsect isx;
    iBoxIsectRay(b, r, &isx);

    return (isx.penned <= 0.0f || isx.contained <= 0.0f);
}

// RECONSTRUCTED (deadstripped): from RotU GC, Incredibles GC
U32 xRayHitsCylinderFast(const xRay3* r, const xCylinder* c)
{
    xASSERT(__LINE__, r);
    xASSERT(__LINE__, c);

    xIsect isx;
    iCylinderIsectRay(c, r, &isx);

    return (isx.penned <= 0.0f || isx.contained <= 0.0f);
}

U32 xRayHitsBoundFast(const xRay3* r, const xBound* b)
{
    xASSERT(__LINE__, r);
    xASSERT(__LINE__, b);

    if (b->type == k_XBOUNDTYPE_SPHERE) {
        return xRayHitsSphereFast(r, &b->sph);
    } else if (b->type == k_XBOUNDTYPE_OBB) {
        xRay3 lr;
        xMat3x3 mn;
        F32 ms = xVec3Length(&b->mat->right);
        xVALIDATE(__LINE__, xeq(ms*ms,xVec3Length2(&b->mat->up),1e-3f));
        xVALIDATE(__LINE__, xeq(ms*ms,xVec3Length2(&b->mat->at),1e-3f));

        xMat3x3Normalize(&mn, b->mat);
        xMat4x3ToLocal(&lr.origin, b->mat, &r->origin);
        xMat3x3Tolocal(&lr.dir, &mn, &r->dir);
        lr.max_t = r->max_t / ms;
        lr.min_t = r->min_t / ms;
        lr.flags = r->flags;

        return xRayHitsBoxFast(&lr, &b->box.box);
    } else if (b->type == k_XBOUNDTYPE_BOX) {
        return xRayHitsBoxFast(r, &b->box.box);
    } else if (b->type == k_XBOUNDTYPE_CYLINDER) {
        return xRayHitsCylinderFast(r, &b->cyl);
    }

    xWARN("xRayHitsBoundFast: unsupported bound type %d\n", b->type);
    return FALSE;
}
// END RECONSTRUCTED
