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

// RECONSTRUCTED (deadstripped): both functions are MAP UNUSED in all four builds. Bodies from RotU GC,
// where they are linked and match.
// Verified in Rat: sizes (Cylinder D 0x13C, R 0x108, MD/M 0x60; Bound D 0x2C0, R 0x1C0, MD/M 0x100),
// D DWARF params and locals (names, order, registers, slots) and References order.
// Unverified: the instruction bytes, the .line layout, the assert lines (__LINE__).
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
