#include "xCommon.h"
#include "iMath3.h"

#include "xMath2.h"
#include "xMath3.h"
#include "xOutputMgr.h"

#include <string.h>

struct xTri3
{
    xVec3 p1;
    xVec3 p2;
    xVec3 p3;
};

void iMath3Init()
{
}

// UNUSED: needed to match (speculative code)
F32 iTri3GetArea(const xTri3* t)
{
    xASSERT(t);

    xVec3 X, Y, N;

    xVec3Sub(&X, &t->p2, &t->p1);
    xVec3Sub(&Y, &t->p3, &t->p1);
    xVec3Cross(&N, &X, &Y);
    return xVec3Length(&N);
}

void iSphereIsectVec(const xSphere* s, const xVec3* v, xIsect* isx)
{
    xASSERT_L(92, s);
    xASSERT_L(93, v);
    xASSERT_L(94, isx);

    xVec3Sub(&isx->norm, v, &s->center);
    isx->dist = xVec3Length(&isx->norm);
    isx->penned = isx->dist - s->r;
}

void iSphereIsectRay(const xSphere* s, const xRay3* r, xIsect* isx)
{
    xASSERT_L(102, s);
    xASSERT_L(103, r);
    xASSERT_L(104, isx);

    F32 t_in, t_out;
    U32 num;

#if DEBUG
    F32 a = xVec3Dot(&r->dir, &r->dir);
    xWARN_INVALID_L(111, "MATH", xabs(a - 1) < 1e-3f);
#endif

    if (!(r->flags & XRAY3_USE_MIN)) {
        ((xRay3*)r)->min_t = 0.0f;
    }
    if (!(r->flags & XRAY3_USE_MAX)) {
        ((xRay3*)r)->max_t = 1.0f;
    }

    xVec3Sub(&isx->norm, &r->origin, &s->center);
    num = xMathSolveQuadratic(xVec3Dot(&r->dir, &r->dir), 2.0f * xVec3Dot(&isx->norm, &r->dir),
                              xVec3Dot(&isx->norm, &isx->norm) - xsqr(s->r), &t_in, &t_out);

    if (num == 0) {
        isx->penned = 1.0f;
        isx->contained = 1.0f;
    } else if (num == 1) {
        if (t_in < r->min_t || t_in > r->max_t) {
            isx->dist = t_in;
            isx->penned = 1.0f;
            isx->contained = 1.0f;
        } else {
            isx->dist = t_in;
            isx->penned = -1.0f;
            isx->contained = 1.0f;
        }
    } else {
        if (t_in < r->min_t) {
            if (t_out < r->min_t) {
                isx->dist = t_out;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            } else {
                isx->dist = t_out;
                isx->penned = -1.0f;
                isx->contained = -1.0f;
            }
        } else {
            if (t_in <= r->max_t) {
                isx->dist = t_in;
                isx->penned = -1.0f;
                isx->contained = 1.0f;
            } else {
                isx->dist = t_in;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            }
        }
    }
}

void iSphereIsectSphere(const xSphere* s, const xSphere* p, xIsect* isx)
{
    xASSERT_L(237, s);
    xASSERT_L(238, p);
    xASSERT_L(239, isx);

    xVec3Sub(&isx->norm, &p->center, &s->center);
    isx->dist = xVec3Length(&isx->norm);
    isx->penned = isx->dist - p->r - s->r;
    isx->contained = isx->dist - s->r;
}

void iSphereInitBoundVec(xSphere* s, const xVec3* v)
{
    xASSERT_L(248, s);
    xASSERT_L(249, v);

    xVec3Copy(&s->center, v);
    s->r = 0.00001f;
}

void iSphereInitBoundOBB(xSphere* s, const xBox* b, const xMat4x3* m)
{
    xASSERT_L(278, s);
    xASSERT_L(279, b);
    xASSERT_L(280, m);

    xVec3 r;

    xVec3AddMacro(&s->center, &b->lower, &b->upper);
    xVec3SMulByMacro(&s->center, 0.5f);
    xVec3SubMacro(&r, &b->upper, &s->center);

    xMat4x3ToWorld(&s->center, m, &s->center);

    s->r = xsqrt(xsqr(r.x) * (xsqr(m->right.x) + xsqr(m->right.y) + xsqr(m->right.z)) +
                 xsqr(r.y) * (xsqr(m->up.x) + xsqr(m->up.y) + xsqr(m->up.z)) +
                 xsqr(r.z) * (xsqr(m->at.x) + xsqr(m->at.y) + xsqr(m->at.z)));
}

void iSphereBoundVec(xSphere* o, const xSphere* s, const xVec3* v)
{
    xASSERT_L(310, o);
    xASSERT_L(311, s);
    xASSERT_L(312, v);

    xIsect isx;
    F32 scale;
    xSphere temp;
    xSphere* tp;
    U32 usetemp = (o == s);

    iSphereIsectVec(s, v, &isx);
    if (isx.penned <= 0.0f) {
        if (!usetemp) {
            memcpy(o, s, sizeof(xSphere));
        }
        return;
    }

    if (usetemp) {
        tp = &temp;
    } else {
        tp = o;
    }

    xVec3Copy(&tp->center, &isx.norm);
    scale = (isx.dist - s->r) / (2.0f * isx.dist);
    xVec3SMul(&tp->center, &tp->center, scale);
    xVec3Add(&tp->center, &tp->center, &s->center);
    tp->r = 0.5f * (isx.dist + s->r);
    if (usetemp) {
        memcpy(o, tp, sizeof(xSphere));
    }
}

void iSphereBoundSphere(xSphere* o, const xSphere* a, const xSphere* b)
{
    xASSERT_L(398, o);
    xASSERT_L(399, a);
    xASSERT_L(400, b);

    xIsect isx;
    F32 scale;
    xSphere temp;
    xSphere* tp;
    U32 usetemp = (o == a);

    iSphereIsectSphere(a, b, &isx);
    if (isx.contained <= 0.0f) {
        if (!usetemp) {
            memcpy(o, a, sizeof(xSphere));
        }
        return;
    }

    if (isx.dist + a->r < b->r) {
        memcpy(o, b, sizeof(xSphere));
        return;
    }

    if (usetemp) {
        tp = &temp;
    } else {
        tp = o;
    }

    xVec3Copy(&tp->center, &isx.norm);
    scale = (b->r + (isx.dist - a->r)) / (2.0f * isx.dist);
    xVec3SMul(&tp->center, &tp->center, scale);
    xVec3Add(&tp->center, &tp->center, &a->center);
    tp->r = 0.5f * (b->r + (isx.dist + a->r));
    if (usetemp) {
        memcpy(o, tp, sizeof(xSphere));
    }
}

void iCylinderIsectVec(const xCylinder* c, const xVec3* v, xIsect* isx)
{
    xASSERT_L(448, c);
    xASSERT_L(449, v);
    xASSERT_L(450, isx);

    F32 a = c->center.y - c->h;
    F32 b = c->center.y + c->h;
    if (v->y >= a && v->y <= b && xVec2Dist(c->center.x, c->center.z, v->x, v->z) <= c->r) {
        isx->penned = -1.0f;
    } else {
        isx->penned = 1.0f;
    }
}

void iBoxVecDist(const xBox* box, const xVec3* v, xIsect* isx)
{
    if (v->x < box->lower.x) {
        if (v->y < box->lower.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (0 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (1 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (2 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            }
        } else if (v->y <= box->upper.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (3 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (4 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            } else {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (5 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            }
        } else {
            if (v->z < box->lower.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (6 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (7 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (8 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            }
        }
    } else if (v->x <= box->upper.x) {
        if (v->y < box->lower.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (9 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (10 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            } else {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (11 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            }
        } else if (v->y <= box->upper.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = 0.0f;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (12 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            } else if (v->z <= box->upper.z) {
            } else {
                isx->norm.x = 0.0f;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (14 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            }
        } else {
            if (v->z < box->lower.z) {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (15 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (16 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            } else {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (17 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            }
        }
    } else {
        if (v->y < box->lower.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (18 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (19 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (20 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            }
        } else if (v->y <= box->upper.y) {
            if (v->z < box->lower.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (21 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (22 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_FACE;
            } else {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (23 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            }
        } else {
            if (v->z < box->lower.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (24 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            } else if (v->z <= box->upper.z) {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (25 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_EDGE;
            } else {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = (isx->flags & ~XISECT_REGION_MASK) | (26 << XISECT_REGION_SHIFT);
                isx->flags |= XISECT_CORNER;
            }
        }
    }

    isx->dist = xVec3Length(&isx->norm);
}

void iBoxIsectVec(const xBox* b, const xVec3* v, xIsect* isx)
{
    xASSERT_L(772, b);
    xASSERT_L(773, v);
    xASSERT_L(774, isx);

    if (v->x >= b->lower.x && v->x <= b->upper.x &&
        v->y >= b->lower.y && v->y <= b->upper.y &&
        v->z >= b->lower.z && v->z <= b->upper.z) {
        isx->penned = -1.0f;
    } else {
        isx->penned = 1.0f;
    }
}

static U32 ClipPlane(F32 denom, F32 numer, F32* t_in, F32* t_out)
{
    if (denom > 0.0f) {
        if (numer > denom * *t_out) {
            return FALSE;
        }
        if (numer > denom * *t_in) {
            *t_in = numer / denom;
        }
        return TRUE;
    } else if (denom < 0.0f) {
        if (numer > denom * *t_in) {
            return FALSE;
        }
        if (numer > denom * *t_out) {
            *t_out = numer / denom;
        }
        return TRUE;
    } else {
        return numer <= 0.0f;
    }
}

static U32 ClipBox(const xVec3* E, const xVec3* P, const xVec3* D, F32* t_in, F32* t_out)
{
    return ClipPlane(D->x, -P->x - E->x, t_in, t_out) && ClipPlane(-D->x, P->x - E->x, t_in, t_out) &&
           ClipPlane(D->y, -P->y - E->y, t_in, t_out) && ClipPlane(-D->y, P->y - E->y, t_in, t_out) &&
           ClipPlane(D->z, -P->z - E->z, t_in, t_out) && ClipPlane(-D->z, P->z - E->z, t_in, t_out);
}

void iBoxIsectRay(const xBox* b, const xRay3* r, xIsect* isx)
{
    xASSERT_L(825, b);
    xASSERT_L(826, r);
    xASSERT_L(827, isx);

    F32 t_in, t_out;
    xVec3 E, P;

#if DEBUG
    F32 a = xVec3Dot(&r->dir, &r->dir);
    xWARN_INVALID_L(835, "MATH", xabs(a - 1) < 1e-3f);
#endif

    if (!(r->flags & XRAY3_USE_MIN)) {
        ((xRay3*)r)->min_t = 0.0f;
    }
    if (!(r->flags & XRAY3_USE_MAX)) {
        ((xRay3*)r)->max_t = 1.0f;
    }

    t_in = -HUGE;
    t_out = HUGE;

    xVec3SubMacro(&E, &b->upper, &b->lower);
    xVec3SMulByMacro(&E, 0.5f);
    xVec3AddMacro(&P, &b->lower, &b->upper);
    xVec3SMulByMacro(&P, -0.5f);
    xVec3AddToMacro(&P, &r->origin);

    if (ClipBox(&E, &P, &r->dir, &t_in, &t_out)) {
        if (t_in < r->min_t) {
            if (t_out < r->min_t) {
                isx->dist = t_out;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            } else {
                isx->dist = t_out;
                isx->penned = -1.0f;
                isx->contained = -1.0f;
            }
        } else {
            if (t_in <= r->max_t) {
                isx->dist = t_in;
                isx->penned = -1.0f;
                isx->contained = 1.0f;
            } else {
                isx->dist = t_in;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            }
        }
    } else {
        isx->penned = 1.0f;
        isx->contained = 1.0f;
    }
}

#define AXIS_CODE(lo, hi, bl, bu)               \
    ((lo) < (bl) ? ((hi) < (bl) ? 2 : ((hi) > (bu) ? 0 : 1)) : ((lo) > (bu) ? 5 : ((hi) > (bu) ? 4 : 3)))

void iBoxIsectSphere(const xBox* box, const xSphere* p, xIsect* isx)
{
    xVec3 a, b;
    U32 xcode, ycode, zcode, xcm, ycm, zcm;

    xASSERT_L(988, box);
    xASSERT_L(989, p);
    xASSERT_L(990, isx);

    a.x = p->center.x - p->r;
    b.x = p->center.x + p->r;
    xcode = AXIS_CODE(a.x, b.x, box->lower.x, box->upper.x);
    xcm = xcode % 3;
    if (xcm == 2) {
        isx->penned = 1.0f;
        return;
    }

    a.y = p->center.y - p->r;
    b.y = p->center.y + p->r;
    ycode = AXIS_CODE(a.y, b.y, box->lower.y, box->upper.y);
    ycm = ycode % 3;
    if (ycm == 2) {
        isx->penned = 1.0f;
        return;
    }

    a.z = p->center.z - p->r;
    b.z = p->center.z + p->r;
    zcode = AXIS_CODE(a.z, b.z, box->lower.z, box->upper.z);
    zcm = zcode % 3;
    if (zcm == 2) {
        isx->penned = 1.0f;
        return;
    }

    iBoxIsectVec(box, &p->center, isx);
    if (isx->penned < 0.0f) {
        xVec3 bc;

        xVec3Add(&bc, &box->lower, &box->upper);
        xVec3SMulBy(&bc, 0.5f);
        xVec3Sub(&isx->norm, &p->center, &bc);
        isx->dist = xVec3Length(&isx->norm);
        isx->contained = -1.0f;
        return;
    }

    isx->flags = xcode | (ycode << XISECT_YCODE_SHIFT) | (zcode << XISECT_ZCODE_SHIFT);
    iBoxVecDist(box, &p->center, isx);
    isx->penned = isx->dist - p->r;
    isx->contained = 1.0f;
}

void iBoxInitBoundVec(xBox* b, const xVec3* v)
{
    xASSERT_L(1077, b);
    xASSERT_L(1078, v);

    xVec3Copy(&b->lower, v);
    xVec3Copy(&b->upper, v);
}

void iBoxBoundVec(xBox* o, const xBox* b, const xVec3* v)
{
    xASSERT_L(1094, o);
    xASSERT_L(1095, b);
    xASSERT_L(1096, v);

    xVec3Init(&o->lower, MIN(v->x, b->lower.x), MIN(v->y, b->lower.y), MIN(v->z, b->lower.z));
    xVec3Init(&o->upper, MAX(v->x, b->upper.x), MAX(v->y, b->upper.y), MAX(v->z, b->upper.z));
}

// UNUSED: needed to match (speculative code)
void iBoxBoundSphere(xBox* o, const xBox* b, const xSphere* s)
{
    xASSERT(o);
    xASSERT(b);
    xASSERT(s);

    xVec3AddC(&o->lower, &s->center, -s->r, -s->r, -s->r);
    xVec3AddC(&o->upper, &s->center, s->r, s->r, s->r);
    iBoxBoundBox(o, b, o);
}

// UNUSED: needed to match (speculative code)
void iBoxBoundBox(xBox* o, const xBox* a, const xBox* b)
{
    xASSERT(o);
    xASSERT(a);
    xASSERT(b);

    xVec3Init(&o->lower, MIN(a->lower.x, b->lower.x), MIN(a->lower.y, b->lower.y), MIN(a->lower.z, b->lower.z));
    xVec3Init(&o->upper, MAX(a->upper.x, b->upper.x), MAX(a->upper.y, b->upper.y), MAX(a->upper.z, b->upper.z));
}
