#include "xCollideFast.h"

#include "xDebug.h"
#include "xBound.h"

U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s)
{
    xASSERT(26, r);
    xASSERT(27, s);

    xVec3 diff;
    xVec3Sub(&diff, &r->origin, &s->center);
    
#ifdef DEBUG
    F32 a = xVec3Dot(&r->dir, &r->dir);
    xVALIDATE(33, xabs(a-1) < 1e-3f);
#endif
    
    F32 c = xVec3Dot(&diff, &diff) - s->r * s->r;
    if (c <= 0.0f) {
        return TRUE;
    }

    if ((r->flags & XRAY3_USE_MAX) && c > (2.0f * s->r + r->max_t) * r->max_t) {
        return FALSE;
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

#ifndef NON_MATCHING
U32 xRayHitsCylinderFast(const xRay3* r, const xCylinder* c)
{
    xASSERT(0, c);
    return 0;
}

U32 xRayHitsBoundFast(const xRay3* r, const xBound* b)
{
    F32 ms = 0.0f;
    xVALIDATE(0, xeq(ms*ms,xVec3Length2(&b->mat->up),1e-3f));
    xVALIDATE(0, xeq(ms*ms,xVec3Length2(&b->mat->at),1e-3f));
    xWARN("xRayHitsBoundFast: unsupported bound type %d\n");
    return 0;
}
#endif
