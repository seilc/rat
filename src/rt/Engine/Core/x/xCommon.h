#ifndef XCOMMON_H
#define XCOMMON_H

#include "types.h"

template <class X, class Y>
static inline void FLAG_REMOVE(X& x, Y y)
{
    x = x & (X)~y;
}

#endif
