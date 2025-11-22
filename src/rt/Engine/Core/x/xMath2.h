#ifndef XMATH2_H
#define XMATH2_H

#include "xMath.h"

struct xVec2
{
    F32 x, y;

    xVec2& assign(F32 x, F32 y);
    xVec2& operator/=(F32 f);
    F32 length() const;
};

template <class T>
struct basic_rect
{
    T x, y, w, h;

    static const basic_rect<F32> m_Null;
    static const basic_rect<F32> m_Unit;
};

extern const xVec2 g_O2;

inline F32 xMath2NearestPointOnLine(F32& Px, F32& Py, F32 Cx, F32 Cy, F32 Ax, F32 Ay, F32 Bx, F32 By)
{
    F32 ABx = Bx - Ax;
    F32 ABy = By - Ay;
    F32 ACx = Cx - Ax;
    F32 ACy = Cy - Ay;

    F32 AB_AC = ABx * ACx + ABy * ACy;
    if (AB_AC <= 0.0f) {
        Px = Ax;
        Py = Ay;
        return 0.0f;
    }

    F32 AB_AB = ABx * ABx + ABy * ABy;
    if (AB_AC >= AB_AB) {
        Px = Bx;
        Py = By;
        return 1.0f;
    }

    F32 t = AB_AC / AB_AB;
    Px = Ax + ABx * t;
    Py = Ay + ABy * t;
    return t;
}

#endif
