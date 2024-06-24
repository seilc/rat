#ifndef XMATH2_H
#define XMATH2_H

#include "xMath.h"

struct xVec2
{
    F32 x, y;
};

template <class T>
struct basic_rect
{
    T x, y, w, h;

    static const basic_rect<F32> m_Null;
    static const basic_rect<F32> m_Unit;
};

extern const xVec2 g_O2;

#endif