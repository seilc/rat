#ifndef ZGOO_H
#define ZGOO_H

#include "xEnt.h"

S32 zGooIs(xEnt* obj, F32& depth, U32 playerCheck);

inline S32 zGooIs(xEnt* obj)
{
    F32 dummy;
    return zGooIs(obj, dummy, FALSE);
}

#endif
